#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKNormalizeImage.hpp"

#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKNormalizeImageFilter(defaults)", "[ITKImageProcessing][ITKNormalizeImage][defaults]")
{
  DataStructure ds;
  ITKNormalizeImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/Ramp-Up-Short.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKNormalizeImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKNormalizeImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKNormalizeImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.view() / "JSONFilters/Baseline/BasicFilters_NormalizeImageFilter_defaults.nrrd";
  DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
  DataPath baselineDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_BaselineDataPath);
  Result<> readBaselineResult = ITKTestBase::ReadImage(ds, baselineFilePath, baselineGeometryPath, baselineDataPath);
  Result<> compareResult = ITKTestBase::CompareImages(ds, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, 0.0001);
  COMPLEX_RESULT_REQUIRE_VALID(compareResult);
}

// This test fails because the filter needs a scalar image.
// SimpleITK does an extra conversion step automatically that we don't do.
// In the original ITKImageProcessing, there were no test cases at all.
TEST_CASE("ITKNormalizeImageFilter(vector)", "[.][ITKImageProcessing][ITKNormalizeImage][vector][!mayfail]")
{
  DataStructure ds;
  ITKNormalizeImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/VM1111Shrink-RGB.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKNormalizeImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKNormalizeImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKNormalizeImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.view() / "JSONFilters/Baseline/BasicFilters_NormalizeImageFilter_vector.nrrd";
  DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
  DataPath baselineDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_BaselineDataPath);
  Result<> readBaselineResult = ITKTestBase::ReadImage(ds, baselineFilePath, baselineGeometryPath, baselineDataPath);
  Result<> compareResult = ITKTestBase::CompareImages(ds, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, 0.0001);
  COMPLEX_RESULT_REQUIRE_VALID(compareResult);
}
