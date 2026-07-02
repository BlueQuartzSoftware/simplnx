#include "SimplnxCore/Filters/ComputeNeighborhoodsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace fs = std::filesystem;

namespace
{
const std::string k_Volumes("Volumes");
const std::string k_EquivalentDiameters("EquivalentDiameters");

const std::string k_Neighborhoods_1("Neighborhoods_1");
const std::string k_NeighborhoodList_1("NeighborhoodList_1");

const std::string k_Neighborhoods_3("Neighborhoods_3");
const std::string k_NeighborhoodList_3("NeighborhoodList_3");

const std::string k_NeighborhoodsNX("computed neighborhood");
const std::string k_NeighborhoodListNX("computed neighborhood list");
} // namespace

TEST_CASE("SimplnxCore::ComputeNeighborhoods_1", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_feature_neighborhoods.tar.gz", "compute_feature_neighborhoods");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_feature_neighborhoods/compute_feature_neighborhoods.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath(k_CellFeatureData);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeNeighborhoodsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insert(ComputeNeighborhoodsFilter::k_MultiplesOfAverage_Key, std::make_any<float32>(1.0F));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_EquivalentDiameters)));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Centroids)));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsNX));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the k_Neighborhoods output array with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_Neighborhoods_1});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodsNX});
    UnitTest::CompareArrays<int32>(dataStructure, exemplarPath, calculatedPath);
  }

  // Compare the k_NeighborhoodList output neighbor list with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodList_1});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodListNX});
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplarPath, calculatedPath);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighborhoods.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeNeighborhoods_3", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_feature_neighborhoods.tar.gz", "compute_feature_neighborhoods");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_feature_neighborhoods/compute_feature_neighborhoods.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath(k_CellFeatureData);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeNeighborhoodsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insert(ComputeNeighborhoodsFilter::k_MultiplesOfAverage_Key, std::make_any<float32>(3.0F));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_EquivalentDiameters)));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Centroids)));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsNX));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the k_Neighborhoods output array with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_Neighborhoods_3});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodsNX});
    UnitTest::CompareArrays<int32>(dataStructure, exemplarPath, calculatedPath);
  }

  // Compare the k_NeighborhoodList output neighbor list with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodList_3});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodListNX});
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplarPath, calculatedPath);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighborhoods.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Exercises the "Search Radius (microns)" mode. When the absolute search radius is set to
// (avgDiameter * multiples / 2), the microns mode must reproduce the "Multiples of Average Diameter"
// mode exactly. Here we target the multiples=1.0 exemplar (Neighborhoods_1 / NeighborhoodList_1).
TEST_CASE("SimplnxCore::ComputeNeighborhoods_SearchRadiusMicrons", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_feature_neighborhoods.tar.gz", "compute_feature_neighborhoods");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_feature_neighborhoods/compute_feature_neighborhoods.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath(k_CellFeatureData);
  const DataPath k_EquivalentDiametersPath = k_CellFeatureDataAM.createChildPath(k_EquivalentDiameters);

  // Compute the average equivalent diameter exactly as the algorithm does: sum features 1..N-1,
  // then divide by the total number of features (feature 0 is background).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath));
  const auto& equivalentDiametersRef = dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath).getDataStoreRef();
  const usize totalFeatures = equivalentDiametersRef.getNumberOfTuples();
  float32 avgDiameter = 0.0F;
  for(usize i = 1; i < totalFeatures; i++)
  {
    avgDiameter += equivalentDiametersRef[i];
  }
  avgDiameter /= static_cast<float32>(totalFeatures);

  // Search radius equivalent to the multiples==1.0 case
  const float32 searchRadius = avgDiameter * 1.0F / 2.0F;

  {
    ComputeNeighborhoodsFilter filter;
    Arguments args;

    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Search Radius (microns)
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadius_Key, std::make_any<float32>(searchRadius));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_EquivalentDiametersPath));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Centroids)));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsNX));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // The microns-mode result must match the multiples==1.0 exemplar
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_Neighborhoods_1});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodsNX});
    UnitTest::CompareArrays<int32>(dataStructure, exemplarPath, calculatedPath);
  }
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodList_1});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodListNX});
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplarPath, calculatedPath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Search Radius must be greater than zero when the "Search Radius (microns)" mode is active.
TEST_CASE("SimplnxCore::ComputeNeighborhoods_InvalidSearchRadius", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_feature_neighborhoods.tar.gz", "compute_feature_neighborhoods");

  auto baseDataFilePath = fs::path(fmt::format("{}/compute_feature_neighborhoods/compute_feature_neighborhoods.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath(k_CellFeatureData);

  ComputeNeighborhoodsFilter filter;
  Arguments args;

  args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Search Radius (microns)
  args.insert(ComputeNeighborhoodsFilter::k_SearchRadius_Key, std::make_any<float32>(0.0F));                         // invalid
  args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insert(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_EquivalentDiameters)));
  args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Centroids)));
  args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsNX));
  args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListNX));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
}

// Independent, hand-verified oracle for the "Search Radius (microns)" mode.
// A tiny synthetic dataset with known centroids and a chosen absolute radius lets us assert exact
// neighbor counts computed by hand, rather than relying on the pre-generated exemplar.
//
//   Feature | Centroid       | Neighbors within radius 3.5 | Count
//   --------|----------------|-----------------------------|------
//     0     | (0,0,0) bkgnd  | (ignored)                   |  -
//     1     | (0,0,0)        | {2}          (d(1,2)=3.0)   |  1
//     2     | (3,0,0)        | {1,3}        (d=3.0, 3.0)   |  2
//     3     | (6,0,0)        | {2}          (d(2,3)=3.0)   |  1
//     4     | (0,8,0)        | {}           (nearest d=8)  |  0
//     5     | (100,100,0)    | {}           (isolated)     |  0
TEST_CASE("SimplnxCore::ComputeNeighborhoods_SyntheticOracle", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const DataPath k_ImageGeomPath({"Data Container"});
  const DataPath k_FeatureAMPath = k_ImageGeomPath.createChildPath("Cell Feature Data");
  const DataPath k_CentroidsPath = k_FeatureAMPath.createChildPath("Centroids");
  const std::string k_NeighborhoodsName("Neighborhoods");
  const std::string k_NeighborhoodListName("NeighborhoodList");

  const usize k_NumFeatures = 6;

  // Build a minimal DataStructure: an Image Geometry with a Cell Feature Attribute Matrix.
  // Note there is intentionally NO Equivalent Diameters array: the "Search Radius (microns)" mode must
  // not require it.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomPath.getTargetName());
  imageGeom->setDimensions({200, 200, 1});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});

  auto* featureAM = AttributeMatrix::Create(dataStructure, k_FeatureAMPath.getTargetName(), std::vector<usize>{k_NumFeatures}, imageGeom->getId());

  auto* centroids = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_CentroidsPath.getTargetName(), {k_NumFeatures}, {3}, featureAM->getId());

  const std::array<std::array<float32, 3>, k_NumFeatures> centroidValues = {{{0.0F, 0.0F, 0.0F},
                                                                             {0.0F, 0.0F, 0.0F},
                                                                             {3.0F, 0.0F, 0.0F},
                                                                             {6.0F, 0.0F, 0.0F},
                                                                             {0.0F, 8.0F, 0.0F},
                                                                             {100.0F, 100.0F, 0.0F}}};
  for(usize i = 0; i < k_NumFeatures; i++)
  {
    (*centroids)[3 * i + 0] = centroidValues[i][0];
    (*centroids)[3 * i + 1] = centroidValues[i][1];
    (*centroids)[3 * i + 2] = centroidValues[i][2];
  }

  {
    ComputeNeighborhoodsFilter filter;
    Arguments args;

    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Search Radius (microns)
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadius_Key, std::make_any<float32>(3.5F));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsPath));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsName));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Verify exact neighbor counts against the hand-computed oracle (features 1..5)
  const DataPath neighborhoodsPath = k_FeatureAMPath.createChildPath(k_NeighborhoodsName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(neighborhoodsPath));
  const auto& neighborhoods = dataStructure.getDataRefAs<Int32Array>(neighborhoodsPath);
  const std::array<int32, k_NumFeatures> expectedCounts = {0, 1, 2, 1, 0, 0};
  for(usize i = 1; i < k_NumFeatures; i++)
  {
    INFO(fmt::format("Feature {} neighborhood count", i));
    REQUIRE(neighborhoods[i] == expectedCounts[i]);
  }

  // Verify the neighbor list lengths match the counts (contents are order-independent)
  const DataPath neighborListPath = k_FeatureAMPath.createChildPath(k_NeighborhoodListName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int32>>(neighborListPath));
  const auto& neighborList = dataStructure.getDataRefAs<NeighborList<int32>>(neighborListPath);
  for(usize i = 1; i < k_NumFeatures; i++)
  {
    INFO(fmt::format("Feature {} neighbor list size", i));
    REQUIRE(neighborList.getListSize(static_cast<int32>(i)) == expectedCounts[i]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// The "Search Radius (microns)" mode reports the input Image Geometry info as a preflight value and warns
// when the radius is sub-voxel or larger than the geometry, to help the user pick a sensible value.
TEST_CASE("SimplnxCore::ComputeNeighborhoods_SearchRadiusPreflightInfo", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const DataPath k_ImageGeomPath({"Data Container"});
  const DataPath k_FeatureAMPath = k_ImageGeomPath.createChildPath("Cell Feature Data");
  const DataPath k_CentroidsPath = k_FeatureAMPath.createChildPath("Centroids");
  const usize k_NumFeatures = 3;

  // Geometry with voxel edge 2.0 microns and physical extents 20 x 20 x 20 microns
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomPath.getTargetName());
  imageGeom->setDimensions({10, 10, 10});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
  imageGeom->setSpacing({2.0F, 2.0F, 2.0F});
  auto* featureAM = AttributeMatrix::Create(dataStructure, k_FeatureAMPath.getTargetName(), std::vector<usize>{k_NumFeatures}, imageGeom->getId());
  Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_CentroidsPath.getTargetName(), {k_NumFeatures}, {3}, featureAM->getId());

  auto makeArgs = [&](float32 searchRadius) {
    Arguments args;
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadiusType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Search Radius (microns)
    args.insert(ComputeNeighborhoodsFilter::k_SearchRadius_Key, std::make_any<float32>(searchRadius));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsPath));
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
    auto preflightResult = filter.preflight(dataStructure, makeArgs(5.0F));
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(hasGeometryInfo(preflightResult));
    REQUIRE(preflightResult.outputActions.warnings().empty());
  }

  SECTION("sub-voxel radius: warning emitted")
  {
    auto preflightResult = filter.preflight(dataStructure, makeArgs(0.5F)); // < voxel edge 2.0
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(hasGeometryInfo(preflightResult));
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == -5734);
  }

  SECTION("oversized radius: warning emitted")
  {
    auto preflightResult = filter.preflight(dataStructure, makeArgs(1000.0F)); // > extent 20.0
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
