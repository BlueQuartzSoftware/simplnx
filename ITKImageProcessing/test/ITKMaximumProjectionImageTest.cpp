#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKMaximumProjectionImage.hpp"

#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKMaximumProjectionImageFilter(defaults)", "[ITKImageProcessing][ITKMaximumProjectionImage][defaults]")
{
  DataStructure ds;
  ITKMaximumProjectionImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Float.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKMaximumProjectionImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "fb78c55635b17fc9ff38ef0ef14f0948");
}

TEST_CASE("ITKMaximumProjectionImageFilter(another_dimension)", "[ITKImageProcessing][ITKMaximumProjectionImage][another_dimension]")
{
  DataStructure ds;
  ITKMaximumProjectionImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Float.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKMaximumProjectionImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(2));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "f3f0d97c83c6b0d92df10c28e2481520");
}

TEST_CASE("ITKMaximumProjectionImageFilter(short_image)", "[ITKImageProcessing][ITKMaximumProjectionImage][short_image]")
{
  DataStructure ds;
  ITKMaximumProjectionImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/Ramp-Up-Short.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKMaximumProjectionImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(1));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "5390344262c91e83bc9208b0991a2fc9");
}

TEST_CASE("ITKMaximumProjectionImageFilter(rgb_image)", "[.][ITKImageProcessing][ITKMaximumProjectionImage][rgb_image][!mayfail]")
{
  DataStructure ds;
  ITKMaximumProjectionImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/VM1111Shrink-RGB.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKMaximumProjectionImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "92134e0dd38fccdf054ff23e67a72e75");
}
