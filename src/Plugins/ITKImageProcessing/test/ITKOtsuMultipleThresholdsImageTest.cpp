#include <catch2/catch.hpp>

#include "ITKImageProcessing/Common/sitkCommon.hpp"
#include "ITKImageProcessing/Filters/ITKOtsuMultipleThresholdsImageFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImageFilter(default)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][default]")
{
  DataStructure dataStructure;
  const ITKOtsuMultipleThresholdsImageFilter filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Short.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // md5 hash only works on in-memory DataStore<T>
  // if(ITKTestBase::IsArrayInMemory(dataStructure, cellDataPath.createChildPath(outputArrayName)))
  {
    const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
    REQUIRE(md5Hash == "a9c3b0c0971c5cbda12b29db916451c6");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImageFilter(two_on_float)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][two_on_float]")
{
  DataStructure dataStructure;
  const ITKOtsuMultipleThresholdsImageFilter filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/Ramp-Zero-One-Float.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<UInt8Parameter::ValueType>(2));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // md5 hash only works on in-memory DataStore<T>
  // if(ITKTestBase::IsArrayInMemory(dataStructure, cellDataPath.createChildPath(outputArrayName)))
  {
    const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
    REQUIRE(md5Hash == "1ab20d3cd9a354b45ac07ec59c0413b3");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImageFilter(three_on)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][three_on]")
{
  DataStructure dataStructure;
  const ITKOtsuMultipleThresholdsImageFilter filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<UInt8Parameter::ValueType>(3));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<UInt32Parameter::ValueType>(256));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // md5 hash only works on in-memory DataStore<T>
  // if(ITKTestBase::IsArrayInMemory(dataStructure, cellDataPath.createChildPath(outputArrayName)))
  {
    const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
    REQUIRE(md5Hash == "b61c3f4e063fcdd24dba76227129ae34");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImageFilter(valley_emphasis)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][valley_emphasis]")
{
  DataStructure dataStructure;
  const ITKOtsuMultipleThresholdsImageFilter filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<UInt8Parameter::ValueType>(3));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<BoolParameter::ValueType>(true));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // md5 hash only works on in-memory DataStore<T>
  // if(ITKTestBase::IsArrayInMemory(dataStructure, cellDataPath.createChildPath(outputArrayName)))
  {
    const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
    REQUIRE(md5Hash == "fb65e730472c8001185f355fb626ca3f");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImageFilter: SIMPL Backwards Compatibility", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImageFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ITKOtsuMultipleThresholdsImageFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ITKOtsuMultipleThresholdsImageFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ITKOtsuMultipleThresholdsImageFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<uint8>(ITKOtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key) == 5);
      CHECK(args.value<uint8>(ITKOtsuMultipleThresholdsImageFilter::k_LabelOffset_Key) == 5);
      CHECK(args.value<uint32>(ITKOtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key) == 5);
      CHECK(args.value<bool>(ITKOtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key) == true);
      CHECK(args.value<DataPath>(ITKOtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ITKOtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ITKOtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key) == "TestName");
    }
  }
}
