#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ComputeBiasedFeaturesFilter.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_ComputedBiasedFeaturesName = "ComputedBiasedFeatures";
const std::string k_ExemplarBiasedFeaturesName = "BiasedFeatures";
const DataPath k_GeometryPath({Constants::k_SmallIN100});
const DataPath k_Geometry2DPath({Constants::k_SmallIN1002});
const DataPath k_CentroidsPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Centroids});
const DataPath k_SurfaceFeaturesPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_SurfaceFeatures});
const DataPath k_PhasesPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Phases});
const DataPath k_Centroids2DPath({Constants::k_SmallIN1002, Constants::k_Grain_Data, Constants::k_Centroids});
const DataPath k_SurfaceFeatures2DPath({Constants::k_SmallIN1002, Constants::k_Grain_Data, Constants::k_SurfaceFeatures});
const DataPath k_Phases2DPath({Constants::k_SmallIN1002, Constants::k_Grain_Data, Constants::k_Phases});
const DataPath k_ExemplarBiasedFeaturesPath({Constants::k_SmallIN100, Constants::k_Grain_Data, k_ExemplarBiasedFeaturesName});
const DataPath k_ComputedBiasedFeaturesPath({Constants::k_SmallIN100, Constants::k_Grain_Data, k_ComputedBiasedFeaturesName});
const DataPath k_ExemplarBiasedFeatures2DPath({Constants::k_SmallIN1002, Constants::k_Grain_Data, k_ExemplarBiasedFeaturesName});
const DataPath k_ComputedBiasedFeatures2DPath({Constants::k_SmallIN1002, Constants::k_Grain_Data, k_ComputedBiasedFeaturesName});
const DataPath k_WrongSurfaceFeaturePath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});
} // namespace

TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Valid filter execution", "[ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_biased_features.tar.gz", "6_6_find_biased_features.dream3d");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_find_biased_features.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeBiasedFeaturesFilter filter;
  Arguments args;

  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CalcByPhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key, std::make_any<std::string>(k_ComputedBiasedFeaturesName));

  SECTION("3D")
  {
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsPath));
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_SurfaceFeaturesPath));
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    UnitTest::CompareArrays<bool>(dataStructure, k_ExemplarBiasedFeaturesPath, k_ComputedBiasedFeaturesPath);
  }
  SECTION("2D")
  {
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_Geometry2DPath));
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_Centroids2DPath));
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_SurfaceFeatures2DPath));
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_Phases2DPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    UnitTest::CompareArrays<bool>(dataStructure, k_ExemplarBiasedFeatures2DPath, k_ComputedBiasedFeatures2DPath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Invalid filter execution", "[ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_biased_features.tar.gz", "6_6_find_biased_features.dream3d");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_find_biased_features.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeBiasedFeaturesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CalcByPhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_WrongSurfaceFeaturePath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key, std::make_any<std::string>(k_ComputedBiasedFeaturesName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeBiasedFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeBiasedFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeBiasedFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeBiasedFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeBiasedFeaturesFilter::k_CalcByPhase_Key) == true);
      CHECK(args.value<DataPath>(ComputeBiasedFeaturesFilter::k_GeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeBiasedFeaturesFilter::k_CentroidsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBiasedFeaturesFilter::k_PhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key) == "TestName");
    }
  }
}

namespace
{
void CheckPlanarBias(usize plane, bool spacingFixture)
{
  const std::array<std::array<usize, 2>, 3> activeAxes = {{{0, 1}, {1, 2}, {0, 2}}};
  const auto axes = activeAxes[plane];
  SizeVec3 dims = {1, 1, 1};
  dims[axes[0]] = 2;
  dims[axes[1]] = 3;
  FloatVec3 spacing = {8, 8, 8};
  FloatVec3 origin = {0, 0, 0};
  std::array<float32, 3> centroid = {4, 4, 4};
  if(spacingFixture)
  {
    spacing = {2, 4, 8};
    const std::array<FloatVec3, 3> origins = {FloatVec3{18, 10, 16}, FloatVec3{19, 14, 0}, FloatVec3{18, 18, 0}};
    origin = origins[plane];
    centroid = {20, 20, 20};
  }
  else
  {
    spacing[axes[0]] = 2;
    spacing[axes[1]] = 4;
    origin[axes[0]] = 10;
    origin[axes[1]] = 20;
    centroid[axes[0]] = 12;
    centroid[axes[1]] = 22;
  }
  DataStructure dataStructure;
  const DataPath geomPath({"Planar"});
  const DataPath featurePath = geomPath.createChildPath("Features");
  auto* geom = ImageGeom::Create(dataStructure, "Planar");
  geom->setDimensions(dims);
  geom->setSpacing(spacing);
  geom->setOrigin(origin);
  auto* features = AttributeMatrix::Create(dataStructure, "Features", {2}, geom->getId());
  auto* centroids = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Centroids", {2}, {3}, features->getId());
  auto* surface = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Surface", {2}, {1}, features->getId());
  auto* phases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Phases", {2}, {1}, features->getId());
  surface->fill(false);
  phases->fill(1);
  centroids->fill(0);
  for(usize axis = 0; axis < 3; axis++)
  {
    (*centroids)[3 + axis] = centroid[axis];
  }
  ComputeBiasedFeaturesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CalcByPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_GeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(featurePath.createChildPath("Centroids")));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(featurePath.createChildPath("Surface")));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(featurePath.createChildPath("Phases")));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key, std::make_any<std::string>("Biased"));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* biased = dataStructure.getDataAs<BoolArray>(featurePath.createChildPath("Biased"));
  REQUIRE(biased != nullptr);
  // Active-axis bounds are [10,14] x [20,32]; (12,22) is strictly inside.
  // Spacing fixtures: XY [18,22] x [10,22], YZ [14,22] x [0,24], XZ [18,22] x [0,24].
  // The centroid (20,20) is strictly inside each spacing fixture. No surface feature shrinks the bounds.
  CHECK_FALSE((*biased)[1]);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Planar bias classification uses active centroid axes", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  const usize plane = GENERATE(0, 1, 2);
  CAPTURE(plane);
  CheckPlanarBias(plane, false);
}

TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Planar bias bounds use active spacing", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  const usize plane = GENERATE(0, 1, 2);
  CAPTURE(plane);
  CheckPlanarBias(plane, true);
}
