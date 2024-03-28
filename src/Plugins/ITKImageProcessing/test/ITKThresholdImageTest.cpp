#include <catch2/catch.hpp>

#include "ITKImageProcessing/Common/sitkCommon.hpp"
#include "ITKImageProcessing/Filters/ITKThresholdImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("ITKImageProcessing::ITKThresholdImageFilter(Default)", "[ITKImageProcessing][ITKThresholdImage][Default]")
{
  DataStructure dataStructure;
  const ITKThresholdImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Short.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKThresholdImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKThresholdImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKThresholdImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "59071590099d21dd439896592338bf95");
}

TEST_CASE("ITKImageProcessing::ITKThresholdImageFilter(Threshold1)", "[ITKImageProcessing][ITKThresholdImage][Threshold1]")
{
  DataStructure dataStructure;
  const ITKThresholdImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Slice-Short.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKThresholdImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKThresholdImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKThresholdImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKThresholdImage::k_Lower_Key, std::make_any<Float64Parameter::ValueType>(25000));
  args.insertOrAssign(ITKThresholdImage::k_Upper_Key, std::make_any<Float64Parameter::ValueType>(65535));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "f70a31938657e0163b82521af4f8e3b0");
}

TEST_CASE("ITKImageProcessing::ITKThresholdImageFilter(Threshold2)", "[ITKImageProcessing][ITKThresholdImage][Threshold2]")
{
  DataStructure dataStructure;
  const ITKThresholdImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Slice-Short.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKThresholdImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKThresholdImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKThresholdImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKThresholdImage::k_Lower_Key, std::make_any<Float64Parameter::ValueType>(0));
  args.insertOrAssign(ITKThresholdImage::k_Upper_Key, std::make_any<Float64Parameter::ValueType>(25000));
  args.insertOrAssign(ITKThresholdImage::k_OutsideValue_Key, std::make_any<Float64Parameter::ValueType>(25000));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "898743e4ec2e75c0169d025fb29b7ead");
}
