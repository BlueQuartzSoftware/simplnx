#include "OrientationAnalysis/Filters/ComputeShapesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <algorithm>
#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("OrientationAnalysis::ComputeShapesFilter", "[SimplnxCore][ComputeShapesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  const std::string k_Omega3sArrayName("Omega3s");
  const std::string k_AxisLengthsArrayName("AxisLengths");
  const std::string k_AxisEulerAnglesArrayName("AxisEulerAngles");
  const std::string k_AspectRatiosArrayName("AspectRatios");
  const std::string k_VolumesArrayName("Shape Volumes");
  const std::string k_Omega3sArrayNameNX("Omega3sNX");
  const std::string k_AxisLengthsArrayNameNX("AxisLengthsNX");
  const std::string k_AxisEulerAnglesArrayNameNX("AxisEulerAnglesNX");
  const std::string k_AspectRatiosArrayNameNX("AspectRatiosNX");
  const std::string k_VolumesArrayNameNX("Shape VolumesNX");

  // Instantiate ComputeShapesFilter
  {
    ComputeShapesFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_CellData, k_FeatureIds});
    const DataPath k_CellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
    const DataPath k_CentroidsArrayPath({k_DataContainer, k_CellFeatureData, k_Centroids});

    const DataPath k_Omega3sArrayPath({k_DataContainer, k_CellFeatureData, k_Omega3sArrayNameNX});
    const DataPath k_AxisLengthsArrayPath({k_DataContainer, k_CellFeatureData, k_AxisLengthsArrayNameNX});
    const DataPath k_AxisEulerAnglesArrayPath({k_DataContainer, k_CellFeatureData, k_AxisEulerAnglesArrayNameNX});
    const DataPath k_AspectRatiosArrayPath({k_DataContainer, k_CellFeatureData, k_AspectRatiosArrayNameNX});
    const DataPath k_VolumesArrayPath({k_DataContainer, k_CellFeatureData, k_VolumesArrayNameNX});
    const DataPath k_SelectedGeometryPath({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeShapesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(ComputeShapesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsArrayPath));
    args.insertOrAssign(ComputeShapesFilter::k_Omega3sArrayName_Key, std::make_any<std::string>(k_Omega3sArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_AxisLengthsArrayName_Key, std::make_any<std::string>(k_AxisLengthsArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_AxisEulerAnglesArrayName_Key, std::make_any<std::string>(k_AxisEulerAnglesArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_AspectRatiosArrayName_Key, std::make_any<std::string>(k_AspectRatiosArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_VolumesArrayName_Key, std::make_any<std::string>(k_VolumesArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedGeometryPath));
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    std::vector<std::string> comparisonNames = {k_Omega3sArrayName, k_AxisLengthsArrayName, k_AxisEulerAnglesArrayName, k_AspectRatiosArrayName, k_VolumesArrayName};
    for(const auto& comparisonName : comparisonNames)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, comparisonName});
      const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, comparisonName + "NX"});
      const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
      UnitTest::CompareDataArrays<float>(exemplarData, calculatedData);
    }
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_shapes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeShapesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeShapesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeShapesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeShapesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeShapesFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeShapesFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeShapesFilter::k_CentroidsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeShapesFilter::k_Omega3sArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeShapesFilter::k_AxisLengthsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeShapesFilter::k_AxisEulerAnglesArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeShapesFilter::k_AspectRatiosArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeShapesFilter::k_VolumesArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: Geometry cell tuple count", "[OrientationAnalysis][ComputeShapesFilter][TupleCount]")
{
  using FilterType = ComputeShapesFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Grid");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 3, 1});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1, 3, 4}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // Equal counts remain valid in another Attribute Matrix with a different tuple shape.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 0), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Grid"})));
  args.insertOrAssign(FilterType::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(inputPath));
  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{2}, imageGeomPtr->getId());
  REQUIRE(featureAMPtr != nullptr);
  REQUIRE(DataArray<float32>::Create(dataStructure, "Centroids", std::make_shared<DataStore<float32>>(ShapeType{2}, ShapeType{3}, 0), featureAMPtr->getId()) != nullptr);
  args.insertOrAssign(FilterType::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath({"Grid", "Feature Data", "Centroids"})));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -12806);
    REQUIRE(errorRef.message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", tuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Grid' has 12 cells") != std::string::npos);
  }
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: Cell Feature Ids requires 1 component", "[OrientationAnalysis][ComputeShapesFilter][ComponentShape]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({2, 1, 1});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", {1, 1, 2}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Ids", {1, 1, 2}, {2}, cellData->getId());
  (*featureIds)[0] = 1;
  (*featureIds)[1] = 0;
  (*featureIds)[2] = 2;
  (*featureIds)[3] = 0;
  auto* featureData = AttributeMatrix::Create(dataStructure, "Feature Data", {3}, imageGeom->getId());
  REQUIRE(featureData != nullptr);
  auto* centroids = UnitTest::CreateTestDataArray<float32>(dataStructure, "Centroids", {3}, {3}, featureData->getId());
  (*centroids)[3] = 0.5F;
  (*centroids)[4] = 0.5F;
  (*centroids)[5] = 0.5F;
  (*centroids)[6] = 1.5F;
  (*centroids)[7] = 0.5F;
  (*centroids)[8] = 0.5F;

  ComputeShapesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeShapesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(ComputeShapesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "Feature Ids"})));
  args.insertOrAssign(ComputeShapesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Feature Data", "Centroids"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(std::any_of(preflightResult.outputActions.errors().begin(), preflightResult.outputActions.errors().end(), [](const Error& error) { return error.code == -208; }));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
