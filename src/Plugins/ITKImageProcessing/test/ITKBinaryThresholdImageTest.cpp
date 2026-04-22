#include <catch2/catch.hpp>

#include "ITKImageProcessing/Common/sitkCommon.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryThresholdImageFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Core/Application.hpp"
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

TEST_CASE("ITKImageProcessing::ITKBinaryThresholdImageFilter(default)", "[ITKImageProcessing][ITKBinaryThresholdImage][default]")
{
  DataStructure dataStructure;
  const ITKBinaryThresholdImageFilter filter;

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
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // md5 hash only works on in-memory DataStore<T>
  // if(ITKTestBase::IsArrayInMemory(dataStructure, cellDataPath.createChildPath(outputArrayName)))
  {
    const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
    REQUIRE(md5Hash == "dbd0ea7d6f16bb93e9c688cb0f1bfd85");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKBinaryThresholdImageFilter(NarrowThreshold)", "[ITKImageProcessing][ITKBinaryThresholdImage][NarrowThreshold]")
{
  DataStructure dataStructure;
  const ITKBinaryThresholdImageFilter filter;

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
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_LowerThreshold_Key, std::make_any<Float64Parameter::ValueType>(10));
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_UpperThreshold_Key, std::make_any<Float64Parameter::ValueType>(100));
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_InsideValue_Key, std::make_any<UInt8Parameter::ValueType>(255));
  args.insertOrAssign(ITKBinaryThresholdImageFilter::k_OutsideValue_Key, std::make_any<UInt8Parameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // md5 hash only works on in-memory DataStore<T>
  // if(ITKTestBase::IsArrayInMemory(dataStructure, cellDataPath.createChildPath(outputArrayName)))
  {
    const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
    REQUIRE(md5Hash == "fc4ce029c088096a69d033ccc5bc1ae2");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKBinaryThresholdImageFilter: SIMPL Backwards Compatibility", "[ITKImageProcessing][ITKBinaryThresholdImageFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ITKBinaryThresholdImageFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ITKBinaryThresholdImageFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ITKBinaryThresholdImageFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<float64>(ITKBinaryThresholdImageFilter::k_LowerThreshold_Key) == 2.5);
      CHECK(args.value<float64>(ITKBinaryThresholdImageFilter::k_UpperThreshold_Key) == 2.5);
      CHECK(args.value<uint8>(ITKBinaryThresholdImageFilter::k_InsideValue_Key) == 5);
      CHECK(args.value<uint8>(ITKBinaryThresholdImageFilter::k_OutsideValue_Key) == 5);
      CHECK(args.value<DataPath>(ITKBinaryThresholdImageFilter::k_InputImageGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ITKBinaryThresholdImageFilter::k_InputImageDataPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ITKBinaryThresholdImageFilter::k_OutputImageArrayName_Key) == "TestName");
    }
  }
}
