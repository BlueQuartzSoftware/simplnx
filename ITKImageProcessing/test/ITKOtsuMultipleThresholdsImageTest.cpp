#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKOtsuMultipleThresholdsImage.hpp"

#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKOtsuMultipleThresholdsImageFilter(default)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][default]")
{
  DataStructure ds;
  ITKOtsuMultipleThresholdsImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Short.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "a9c3b0c0971c5cbda12b29db916451c6");
}

TEST_CASE("ITKOtsuMultipleThresholdsImageFilter(two_on_float)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][two_on_float]")
{
  DataStructure ds;
  ITKOtsuMultipleThresholdsImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/Ramp-Zero-One-Float.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<UInt8Parameter::ValueType>(2));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ReturnBinMidpoint_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "1ab20d3cd9a354b45ac07ec59c0413b3");
}

TEST_CASE("ITKOtsuMultipleThresholdsImageFilter(three_on)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][three_on]")
{
  DataStructure ds;
  ITKOtsuMultipleThresholdsImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<UInt8Parameter::ValueType>(3));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfHistogramBins_Key, std::make_any<UInt32Parameter::ValueType>(256));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ReturnBinMidpoint_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "b61c3f4e063fcdd24dba76227129ae34");
}

TEST_CASE("ITKOtsuMultipleThresholdsImageFilter(valley_emphasis)", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage][valley_emphasis]")
{
  DataStructure ds;
  ITKOtsuMultipleThresholdsImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<UInt8Parameter::ValueType>(3));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ValleyEmphasis_Key, std::make_any<BoolParameter::ValueType>(true));
  args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ReturnBinMidpoint_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "fb65e730472c8001185f355fb626ca3f");
}
