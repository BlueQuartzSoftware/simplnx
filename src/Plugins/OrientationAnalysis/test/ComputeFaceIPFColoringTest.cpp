#include "OrientationAnalysis/Filters/ComputeFaceIPFColoringFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
constexpr StringLiteral k_FaceIPFColors("SurfaceMeshFaceIPFColors");
constexpr StringLiteral k_FirstNXFaceIPFColors("NXFaceIPFColors 0");
constexpr StringLiteral k_SecondNXFaceIPFColors("NXFaceIPFColors 1");

DataPath smallIn100Group({nx::core::Constants::k_SmallIN100});
DataPath featureDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_Grain_Data);
DataPath avgEulerAnglesPath = featureDataPath.createChildPath(nx::core::Constants::k_AvgEulerAngles);
DataPath featurePhasesPath = featureDataPath.createChildPath(nx::core::Constants::k_Phases);
DataPath crystalStructurePath = smallIn100Group.createChildPath(nx::core::Constants::k_Phase_Data).createChildPath(nx::core::Constants::k_CrystalStructures);
DataPath avgQuatsPath = featureDataPath.createChildPath("AvgQuats");

DataPath triangleDataContainerPath({nx::core::Constants::k_TriangleDataContainerName});
DataPath faceDataGroup = triangleDataContainerPath.createChildPath(nx::core::Constants::k_FaceData);

DataPath faceLabels = faceDataGroup.createChildPath(nx::core::Constants::k_FaceLabels);
DataPath faceNormals = faceDataGroup.createChildPath(nx::core::Constants::k_FaceNormals);
DataPath faceAreas = faceDataGroup.createChildPath(nx::core::Constants::k_FaceAreas);
} // namespace

TEST_CASE("OrientationAnalysis::ComputeFaceIPFColoringFilter: Valid filter execution", "[OrientationAnalysis][ComputeFaceIPFColoringFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFaceIPFColoringFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FirstFaceIPFColorsArrayName_Key, std::make_any<std::string>(::k_FirstNXFaceIPFColors));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SecondFaceIPFColorsArrayName_Key, std::make_any<std::string>(::k_SecondNXFaceIPFColors));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // compare the resulting face IPF Colors array
  DataPath exemplarPath = faceDataGroup.createChildPath(::k_FaceIPFColors);
  const auto& exemplarArray = dataStructure.getDataRefAs<UInt8Array>(exemplarPath);
  DataPath firstGeneratedPath = faceDataGroup.createChildPath(::k_FirstNXFaceIPFColors);
  const auto& firstArray = dataStructure.getDataRefAs<UInt8Array>(firstGeneratedPath);
  DataPath secondGeneratedPath = faceDataGroup.createChildPath(::k_SecondNXFaceIPFColors);
  const auto& secondArray = dataStructure.getDataRefAs<UInt8Array>(secondGeneratedPath);

  // Done this way because original output was segmented into separate arrays for vis
  for(usize i = 0; i < exemplarArray.getNumberOfTuples(); i++)
  {
    REQUIRE(exemplarArray[(i * 6) + 0] == firstArray[(i * 3) + 0]);
    REQUIRE(exemplarArray[(i * 6) + 1] == firstArray[(i * 3) + 1]);
    REQUIRE(exemplarArray[(i * 6) + 2] == firstArray[(i * 3) + 2]);

    REQUIRE(exemplarArray[(i * 6) + 3] == secondArray[(i * 3) + 0]);
    REQUIRE(exemplarArray[(i * 6) + 4] == secondArray[(i * 3) + 1]);
    REQUIRE(exemplarArray[(i * 6) + 5] == secondArray[(i * 3) + 2]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFaceIPFColoringFilter: Invalid filter execution", "[OrientationAnalysis][ComputeFaceIPFColoringFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFaceIPFColoringFilter filter;
  Arguments args;

  SECTION("Inconsistent face data tuple dimensions")
  {
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
  }
  SECTION("Inconsistent cell data tuple dimensions")
  {
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFaceIPFColoringFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeFaceIPFColoringFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFaceIPFColoringFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFaceIPFColoringFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFaceIPFColoringFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFaceIPFColoringFilter::k_FirstFaceIPFColorsArrayName_Key) == "TestName");
    }
  }
}
