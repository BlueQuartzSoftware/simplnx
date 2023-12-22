#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKBinaryThresholdImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("ITKImageProcessing::ITKBinaryThresholdImageFilter(default)", "[ITKImageProcessing][ITKBinaryThresholdImage][default]")
{
  DataStructure dataStructure;
  ITKBinaryThresholdImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataPath);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Short.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  Arguments args;
  args.insertOrAssign(ITKBinaryThresholdImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKBinaryThresholdImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKBinaryThresholdImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "dbd0ea7d6f16bb93e9c688cb0f1bfd85");
}

TEST_CASE("ITKImageProcessing::ITKBinaryThresholdImageFilter(NarrowThreshold)", "[ITKImageProcessing][ITKBinaryThresholdImage][NarrowThreshold]")
{
  DataStructure dataStructure;
  ITKBinaryThresholdImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataPath);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Short.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  Arguments args;
  args.insertOrAssign(ITKBinaryThresholdImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKBinaryThresholdImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKBinaryThresholdImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKBinaryThresholdImage::k_LowerThreshold_Key, std::make_any<Float64Parameter::ValueType>(10));
  args.insertOrAssign(ITKBinaryThresholdImage::k_UpperThreshold_Key, std::make_any<Float64Parameter::ValueType>(100));
  args.insertOrAssign(ITKBinaryThresholdImage::k_InsideValue_Key, std::make_any<UInt8Parameter::ValueType>(255));
  args.insertOrAssign(ITKBinaryThresholdImage::k_OutsideValue_Key, std::make_any<UInt8Parameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "fc4ce029c088096a69d033ccc5bc1ae2");
}
