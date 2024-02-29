#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKOtsuMultipleThresholdsImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImageFilter(default)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][default]")
{
  DataStructure dataStructure;
  ITKOtsuMultipleThresholdsImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Short.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "a9c3b0c0971c5cbda12b29db916451c6");
}

TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImageFilter(two_on_float)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][two_on_float]")
{
  DataStructure dataStructure;
  ITKOtsuMultipleThresholdsImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/Ramp-Zero-One-Float.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<UInt8Parameter::ValueType>(2));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ReturnBinMidpoint_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "1ab20d3cd9a354b45ac07ec59c0413b3");
}

TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImageFilter(three_on)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][three_on]")
{
  DataStructure dataStructure;
  ITKOtsuMultipleThresholdsImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<UInt8Parameter::ValueType>(3));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfHistogramBins_Key, std::make_any<UInt32Parameter::ValueType>(256));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ReturnBinMidpoint_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "b61c3f4e063fcdd24dba76227129ae34");
}

TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImageFilter(valley_emphasis)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][valley_emphasis]")
{
  DataStructure dataStructure;
  ITKOtsuMultipleThresholdsImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<UInt8Parameter::ValueType>(3));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ValleyEmphasis_Key, std::make_any<BoolParameter::ValueType>(true));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ReturnBinMidpoint_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "fb65e730472c8001185f355fb626ca3f");
}
