#include "SimplnxCore/Filters/ComputeNeighborhoodsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <filesystem>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace fs = std::filesystem;

namespace
{
// Helper: build a minimal DataStructure with an Image Geometry and a Cell Feature Attribute Matrix holding
// Centroids (and optionally EquivalentDiameters). Returns the created DataStructure.
struct SyntheticFeatureData
{
  DataStructure dataStructure;
  DataPath imageGeomPath;
  DataPath featureAMPath;
  DataPath centroidsPath;
  DataPath eqDiamPath;
};

SyntheticFeatureData BuildSyntheticFeatures(usize numFeatures, const std::vector<std::array<float32, 3>>& centroidValues, const std::vector<float32>& eqDiamValues, SizeVec3 dims, FloatVec3 spacing)
{
  SyntheticFeatureData out;
  out.imageGeomPath = DataPath({"Data Container"});
  out.featureAMPath = out.imageGeomPath.createChildPath("Cell Feature Data");
  out.centroidsPath = out.featureAMPath.createChildPath("Centroids");
  out.eqDiamPath = out.featureAMPath.createChildPath("EquivalentDiameters");

  auto* imageGeom = ImageGeom::Create(out.dataStructure, out.imageGeomPath.getTargetName());
  imageGeom->setDimensions(dims);
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
  imageGeom->setSpacing(spacing);

  auto* featureAM = AttributeMatrix::Create(out.dataStructure, out.featureAMPath.getTargetName(), std::vector<usize>{numFeatures}, imageGeom->getId());

  auto* centroids = Float32Array::CreateWithStore<Float32DataStore>(out.dataStructure, out.centroidsPath.getTargetName(), {numFeatures}, {3}, featureAM->getId());
  for(usize i = 0; i < numFeatures; i++)
  {
    (*centroids)[3 * i + 0] = centroidValues[i][0];
    (*centroids)[3 * i + 1] = centroidValues[i][1];
    (*centroids)[3 * i + 2] = centroidValues[i][2];
  }

  if(!eqDiamValues.empty())
  {
    auto* eqDiams = Float32Array::CreateWithStore<Float32DataStore>(out.dataStructure, out.eqDiamPath.getTargetName(), {numFeatures}, {1}, featureAM->getId());
    for(usize i = 0; i < numFeatures; i++)
    {
      (*eqDiams)[i] = eqDiamValues[i];
    }
  }

  return out;
}
} // namespace

// Class 1 (Analytical) oracle for the "Search Radius (microns)" mode. A tiny synthetic dataset with known
// centroids and a chosen absolute radius lets us assert exact neighbor counts computed by hand, independently
// of any exemplar or legacy output. No Equivalent Diameters array is provided, proving the microns mode does
// not require it.
//
//   Feature | Centroid       | Neighbors within radius 3.5 | Count
//   --------|----------------|-----------------------------|------
//     0     | (0,0,0) bkgnd  | (excluded)                  |  0
//     1     | (0,0,0)        | {2}          (d(1,2)=3.0)   |  1
//     2     | (3,0,0)        | {1,3}        (d=3.0, 3.0)   |  2
//     3     | (6,0,0)        | {2}          (d(2,3)=3.0)   |  1
//     4     | (0,8,0)        | {6}          (d(4,6)=3.5)   |  1
//     5     | (100,100,0)    | {}           (isolated)     |  0
//     6     | (3.5,8,0)      | {4}          (d(4,6)=3.5)   |  1
//
// Features 4 and 6 are separated by EXACTLY the search radius (3.5, exactly representable in float32, so
// distSq == radiusSq == 12.25 with no rounding): this pins the inclusive `distSq <= radiusSq` boundary
// semantics (a centroid exactly on the sphere surface IS a neighbor; legacy 6.5.171 used a strict `<` on
// bin differences — see deviation D1).
TEST_CASE("SimplnxCore::ComputeNeighborhoods_SyntheticOracle", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const usize k_NumFeatures = 7;
  const std::vector<std::array<float32, 3>> centroids = {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F},     {3.0F, 0.0F, 0.0F}, {6.0F, 0.0F, 0.0F},
                                                         {0.0F, 8.0F, 0.0F}, {100.0F, 100.0F, 0.0F}, {3.5F, 8.0F, 0.0F}};
  auto data = BuildSyntheticFeatures(k_NumFeatures, centroids, {}, {200, 200, 1}, {1.0F, 1.0F, 1.0F});

  const std::string k_NeighborhoodsName("Neighborhoods");
  const std::string k_NeighborhoodListName("NeighborhoodList");
  {
    ComputeNeighborhoodsFilter filter;
    Arguments args;
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Search Radius (microns)
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadius_Key, std::make_any<float32>(3.5F));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(data.imageGeomPath));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(data.centroidsPath));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsName));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListName));

    auto preflightResult = filter.preflight(data.dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(data.dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  const DataPath neighborhoodsPath = data.featureAMPath.createChildPath(k_NeighborhoodsName);
  REQUIRE_NOTHROW(data.dataStructure.getDataRefAs<Int32Array>(neighborhoodsPath));
  const auto& neighborhoods = data.dataStructure.getDataRefAs<Int32Array>(neighborhoodsPath);
  const std::array<int32, k_NumFeatures> expectedCounts = {0, 1, 2, 1, 1, 0, 1};
  // Index 0 is asserted too: the background feature must never search nor be counted, so its count is 0.
  for(usize i = 0; i < k_NumFeatures; i++)
  {
    INFO(fmt::format("Feature {} neighborhood count", i));
    REQUIRE(neighborhoods[i] == expectedCounts[i]);
  }

  const DataPath neighborListPath = data.featureAMPath.createChildPath(k_NeighborhoodListName);
  REQUIRE_NOTHROW(data.dataStructure.getDataRefAs<NeighborList<int32>>(neighborListPath));
  const auto& neighborList = data.dataStructure.getDataRefAs<NeighborList<int32>>(neighborListPath);

  // Class 4 invariant: count == list size for every feature, including the background feature 0
  for(usize i = 0; i < k_NumFeatures; i++)
  {
    INFO(fmt::format("Feature {} count-vs-list-size invariant", i));
    REQUIRE(neighborList.getListSize(static_cast<int32>(i)) == expectedCounts[i]);
  }

  // Boundary inclusion: features 4 and 6 are exactly 3.5 apart (== the search radius) and must list each other.
  {
    const auto list4 = neighborList.getList(4);
    const auto list6 = neighborList.getList(6);
    REQUIRE(std::find(list4.begin(), list4.end(), 6) != list4.end());
    REQUIRE(std::find(list6.begin(), list6.end(), 4) != list6.end());
  }

  // Class 4 invariant: in microns mode every feature uses the SAME radius, so the neighbor relation is
  // symmetric (j in list(i)  <=>  i in list(j)).
  for(usize i = 1; i < k_NumFeatures; i++)
  {
    const auto listI = neighborList.getList(static_cast<int32>(i));
    for(const int32 j : listI)
    {
      const auto listJ = neighborList.getList(j);
      INFO(fmt::format("Symmetry: feature {} lists {}, so {} must list {}", i, j, j, i));
      REQUIRE(std::find(listJ.begin(), listJ.end(), static_cast<int32>(i)) != listJ.end());
    }
  }

  UnitTest::CheckArraysInheritTupleDims(data.dataStructure);
}

// Class 1 (Analytical) oracle for the "Multiples of Equivalent Diameter" mode. Each feature searches within a
// radius equal to its OWN Equivalent Diameter times the multiplier (radius_i = eqDiam[i] * mult), so the
// neighbor relation is per-feature and can be asymmetric: a large feature reaches a small one that does not
// reach back. This fixture is designed to exercise that asymmetry.
//
//   eqDiam: f1=6, f2=f3=f4=f5=2 ; mult=1  ->  r1=6, r2=r3=r4=r5=2
//   Feature | Centroid   | within r_i of         | Count
//   --------|------------|-----------------------|------
//     1     | (0,0,0)    | 2(d4),3(d3),4(d1) <=6 |  3   {2,3,4}
//     2     | (4,0,0)    | none <= 2             |  0   {}
//     3     | (0,3,0)    | none <= 2             |  0   {}
//     4     | (1,0,0)    | 1(d1) <= 2            |  1   {1}
//     5     | (50,0,0)   | isolated             |  0   {}
// Asymmetry: f1 lists f2 (d=4 <= r1=6) but f2 does NOT list f1 (d=4 > r2=2).
TEST_CASE("SimplnxCore::ComputeNeighborhoods_MultiplesAnalyticalOracle", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const usize k_NumFeatures = 6;
  const std::vector<float32> eqDiam = {0.0F, 6.0F, 2.0F, 2.0F, 2.0F, 2.0F};
  const std::vector<std::array<float32, 3>> centroids = {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}, {4.0F, 0.0F, 0.0F}, {0.0F, 3.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {50.0F, 0.0F, 0.0F}};
  auto data = BuildSyntheticFeatures(k_NumFeatures, centroids, eqDiam, {100, 100, 1}, {1.0F, 1.0F, 1.0F});

  const std::string k_NeighborhoodsName("Neighborhoods");
  const std::string k_NeighborhoodListName("NeighborhoodList");
  {
    ComputeNeighborhoodsFilter filter;
    Arguments args;
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL)); // Multiples of Equivalent Diameter
    args.insert(ComputeNeighborhoodsFilter::k_MultiplesOfAverage_Key, std::make_any<float32>(1.0F));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(data.imageGeomPath));
    args.insert(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(data.eqDiamPath));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(data.centroidsPath));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsName));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListName));

    auto preflightResult = filter.preflight(data.dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(data.dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Class 1: exact hand-derived neighbor counts
  const DataPath neighborhoodsPath = data.featureAMPath.createChildPath(k_NeighborhoodsName);
  REQUIRE_NOTHROW(data.dataStructure.getDataRefAs<Int32Array>(neighborhoodsPath));
  const auto& neighborhoods = data.dataStructure.getDataRefAs<Int32Array>(neighborhoodsPath);
  const std::array<int32, k_NumFeatures> expectedCounts = {0, 3, 0, 0, 1, 0};
  // Index 0 is asserted too. Feature 0's centroid coincides with feature 1's, so if the background feature
  // were ever allowed to act as a search source (or be counted as a candidate) this assertion would fail.
  for(usize i = 0; i < k_NumFeatures; i++)
  {
    INFO(fmt::format("Feature {} neighborhood count", i));
    REQUIRE(neighborhoods[i] == expectedCounts[i]);
  }

  const DataPath neighborListPath = data.featureAMPath.createChildPath(k_NeighborhoodListName);
  REQUIRE_NOTHROW(data.dataStructure.getDataRefAs<NeighborList<int32>>(neighborListPath));
  const auto& neighborList = data.dataStructure.getDataRefAs<NeighborList<int32>>(neighborListPath);

  // Class 4 invariant: count == list size for every feature, including the background feature 0
  for(usize i = 0; i < k_NumFeatures; i++)
  {
    INFO(fmt::format("Feature {} count-vs-list-size invariant", i));
    REQUIRE(neighborList.getListSize(static_cast<int32>(i)) == neighborhoods[i]);
  }

  // Per-feature asymmetry: f1 (large radius) lists f2, but f2 (small radius) does NOT list f1.
  {
    const auto list1 = neighborList.getList(1);
    const auto list2 = neighborList.getList(2);
    REQUIRE(std::find(list1.begin(), list1.end(), 2) != list1.end());
    REQUIRE(std::find(list2.begin(), list2.end(), 1) == list2.end());
  }
  // Symmetric pair: f1 and f4 are within each other's radius, so each lists the other.
  {
    const auto list1 = neighborList.getList(1);
    const auto list4 = neighborList.getList(4);
    REQUIRE(std::find(list1.begin(), list1.end(), 4) != list1.end());
    REQUIRE(std::find(list4.begin(), list4.end(), 1) != list4.end());
  }

  UnitTest::CheckArraysInheritTupleDims(data.dataStructure);
}

// The radius parameter that is active for the selected Search Radius Type must be greater than zero:
// "Search Radius (microns)" rejects a non-positive radius with -5733 and "Multiples of Equivalent Diameter"
// rejects a non-positive multiplier with -5732 (legacy 6.5.171 accepted these silently — see deviation D3).
TEST_CASE("SimplnxCore::ComputeNeighborhoods_InvalidSearchRadius", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const std::vector<std::array<float32, 3>> centroids = {{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F}};
  const std::vector<float32> eqDiams = {0.0F, 1.0F, 1.0F};
  auto data = BuildSyntheticFeatures(3, centroids, eqDiams, {10, 10, 10}, {1.0F, 1.0F, 1.0F});

  ComputeNeighborhoodsFilter filter;
  Arguments args;
  args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(data.imageGeomPath));
  args.insert(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(data.eqDiamPath));
  args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(data.centroidsPath));
  args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>("Neighborhoods"));
  args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>("NeighborhoodList"));

  SECTION("microns mode: non-positive search radius fails preflight with -5733")
  {
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Search Radius (microns)
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadius_Key, std::make_any<float32>(0.0F));                         // invalid

    auto preflightResult = filter.preflight(data.dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5733);
  }

  SECTION("multiples mode: non-positive multiplier fails preflight with -5732")
  {
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL)); // Multiples of Equivalent Diameter
    args.insert(ComputeNeighborhoodsFilter::k_MultiplesOfAverage_Key, std::make_any<float32>(0.0F));                   // invalid

    auto preflightResult = filter.preflight(data.dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5732);
  }

  SECTION("multiples mode: eqDiam/centroids tuple-count mismatch fails preflight with -5730")
  {
    auto* imageGeom = data.dataStructure.getDataAs<ImageGeom>(data.imageGeomPath);
    Float32Array::CreateWithStore<Float32DataStore>(data.dataStructure, "WrongEqDiams", {4}, {1}, imageGeom->getId());

    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL)); // Multiples of Equivalent Diameter
    args.insert(ComputeNeighborhoodsFilter::k_MultiplesOfAverage_Key, std::make_any<float32>(1.0F));
    args.insertOrAssign(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(data.imageGeomPath.createChildPath("WrongEqDiams")));

    auto preflightResult = filter.preflight(data.dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5730);
  }

  SECTION("centroids not inside an Attribute Matrix fails preflight with -5731")
  {
    auto* imageGeom = data.dataStructure.getDataAs<ImageGeom>(data.imageGeomPath);
    auto* looseCentroids = Float32Array::CreateWithStore<Float32DataStore>(data.dataStructure, "LooseCentroids", {3}, {3}, imageGeom->getId());
    looseCentroids->fill(0.0F);

    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Search Radius (microns)
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadius_Key, std::make_any<float32>(1.0F));
    args.insertOrAssign(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(data.imageGeomPath.createChildPath("LooseCentroids")));

    auto preflightResult = filter.preflight(data.dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -5731);
  }
}

// The "Search Radius (microns)" mode reports the input Image Geometry info as a preflight value and warns
// when the radius is sub-voxel or larger than the geometry, to help the user pick a sensible value.
TEST_CASE("SimplnxCore::ComputeNeighborhoods_SearchRadiusPreflightInfo", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  // Geometry with voxel edge 2.0 microns and physical extents 20 x 20 x 20 microns
  const std::vector<std::array<float32, 3>> centroids = {{0.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F}, {4.0F, 0.0F, 0.0F}};
  auto data = BuildSyntheticFeatures(3, centroids, {}, {10, 10, 10}, {2.0F, 2.0F, 2.0F});

  auto makeArgs = [&](float32 searchRadius) {
    Arguments args;
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Search Radius (microns)
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadius_Key, std::make_any<float32>(searchRadius));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(data.imageGeomPath));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(data.centroidsPath));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>("Neighborhoods"));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>("NeighborhoodList"));
    return args;
  };

  auto hasGeometryInfo = [](const IFilter::PreflightResult& result) {
    return std::any_of(result.outputValues.cbegin(), result.outputValues.cend(), [](const IFilter::PreflightValue& v) { return v.name == "Input Image Geometry Info"; });
  };

  ComputeNeighborhoodsFilter filter;

  SECTION("reasonable radius: geometry info reported, no radius warnings")
  {
    auto preflightResult = filter.preflight(data.dataStructure, makeArgs(5.0F));
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(hasGeometryInfo(preflightResult));
    REQUIRE(preflightResult.outputActions.warnings().empty());
  }

  SECTION("sub-voxel radius: warning emitted")
  {
    auto preflightResult = filter.preflight(data.dataStructure, makeArgs(0.5F)); // < voxel edge 2.0
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(hasGeometryInfo(preflightResult));
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == -5734);
  }

  SECTION("oversized radius: warning emitted")
  {
    auto preflightResult = filter.preflight(data.dataStructure, makeArgs(1000.0F)); // > extent 20.0
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(hasGeometryInfo(preflightResult));
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == -5735);
  }
}

TEST_CASE("SimplnxCore::ComputeNeighborhoodsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeNeighborhoodsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeNeighborhoodsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeNeighborhoodsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ComputeNeighborhoodsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<float32>(ComputeNeighborhoodsFilter::k_MultiplesOfAverage_Key) == 2.5f);
      CHECK(args.value<DataPath>(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key) == "TestName");
    }
  }
}
