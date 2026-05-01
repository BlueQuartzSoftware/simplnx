#include "OrientationAnalysis/Filters/ComputeFeatureFaceMisorientationFilter.hpp"
#include "OrientationAnalysis/Filters/ConvertOrientationsFilter.hpp"
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
constexpr StringLiteral k_FaceMisorientationColors("SurfaceMeshFaceMisorientationColors");
constexpr StringLiteral k_NXFaceMisorientationColors("NXFaceMisorientationColors");
constexpr StringLiteral k_AvgQuats("AvgQuats");

DataPath smallIn100Group({nx::core::Constants::k_SmallIN100});
DataPath featureDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_Grain_Data);
DataPath avgEulerAnglesPath = featureDataPath.createChildPath(nx::core::Constants::k_AvgEulerAngles);
DataPath featurePhasesPath = featureDataPath.createChildPath(nx::core::Constants::k_Phases);
DataPath crystalStructurePath = smallIn100Group.createChildPath(nx::core::Constants::k_Phase_Data).createChildPath(nx::core::Constants::k_CrystalStructures);
DataPath avgQuatsPath = featureDataPath.createChildPath(k_AvgQuats);

DataPath triangleDataContainerPath({nx::core::Constants::k_TriangleDataContainerName});
DataPath faceDataGroup = triangleDataContainerPath.createChildPath(nx::core::Constants::k_FaceData);

DataPath faceLabels = faceDataGroup.createChildPath(nx::core::Constants::k_FaceLabels);
DataPath faceNormals = faceDataGroup.createChildPath(nx::core::Constants::k_FaceNormals);
DataPath faceAreas = faceDataGroup.createChildPath(nx::core::Constants::k_FaceAreas);
} // namespace

TEST_CASE("OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Valid filter execution", "[OrientationAnalysis][ComputeFeatureFaceMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Convert the AvgEulerAngles array to AvgQuats for use in ComputeFeatureFaceMisorientationFilter input
  {
    // Instantiate the filter, and an Arguments Object
    ConvertOrientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<uint64>(2));
    args.insertOrAssign(ConvertOrientationsFilter::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
    args.insertOrAssign(ConvertOrientationsFilter::k_OutputOrientationArrayName_Key, std::make_any<std::string>(k_AvgQuats));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // ComputeFeatureFaceMisorientationFilter
  {
    // Instantiate the filter, and an Arguments Object
    ComputeFeatureFaceMisorientationFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_MisorientationArrayName_Key, std::make_any<std::string>(::k_NXFaceMisorientationColors));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // compare the resulting face IPF Colors array
  DataPath exemplarPath = faceDataGroup.createChildPath(::k_FaceMisorientationColors);
  DataPath generatedPath = faceDataGroup.createChildPath(::k_NXFaceMisorientationColors);
  CompareArrays<float32>(dataStructure, exemplarPath, generatedPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Invalid filter execution", "[OrientationAnalysis][ComputeFeatureFaceMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFeatureFaceMisorientationFilter filter;
  Arguments args;

  SECTION("Inconsistent cell data tuple dimensions")
  {
    // Convert the AvgEulerAngles array to AvgQuats for use in ComputeFeatureFaceMisorientationFilter input
    {
      // Instantiate the filter, and an Arguments Object
      ConvertOrientationsFilter convertOrientationsFilter;
      Arguments convertOrientationsArgs;

      // Create default Parameters for the filter.
      convertOrientationsArgs.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<uint64>(0));
      convertOrientationsArgs.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<uint64>(2));
      convertOrientationsArgs.insertOrAssign(ConvertOrientationsFilter::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
      convertOrientationsArgs.insertOrAssign(ConvertOrientationsFilter::k_OutputOrientationArrayName_Key, std::make_any<std::string>(k_AvgQuats));

      // Execute the filter and check the result
      auto convertOrientationsResult = convertOrientationsFilter.execute(dataStructure, convertOrientationsArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(convertOrientationsResult.result);
    }

    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_MisorientationArrayName_Key, std::make_any<std::string>(::k_NXFaceMisorientationColors));
  }

  SECTION("Missing input data path")
  {
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_MisorientationArrayName_Key, std::make_any<std::string>(::k_NXFaceMisorientationColors));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeFeatureFaceMisorientationFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureFaceMisorientationFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureFaceMisorientationFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureFaceMisorientationFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeatureFaceMisorientationFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureFaceMisorientationFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureFaceMisorientationFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureFaceMisorientationFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFeatureFaceMisorientationFilter::k_MisorientationArrayName_Key) == "TestName");
    }
  }
}
