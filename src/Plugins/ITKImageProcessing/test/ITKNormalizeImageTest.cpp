#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKNormalizeImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKImageProcessing::ITKNormalizeImageFilter(defaults)", "[ITKImageProcessing][ITKNormalizeImage][defaults]")
{
  DataStructure dataStructure;
  ITKNormalizeImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataPath);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/Ramp-Up-Short.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult)

  Arguments args;
  args.insertOrAssign(ITKNormalizeImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKNormalizeImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKNormalizeImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  fs::path baselineFilePath = fs::path(complex::unit_test::k_DataDir.view()) / "JSONFilters/Baseline/BasicFilters_NormalizeImageFilter_defaults.nrrd";
  DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
  DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
  Result<> readBaselineResult = ITKTestBase::ReadImage(dataStructure, baselineFilePath, baselineGeometryPath, baseLineCellDataPath, baselineDataPath);
  Result<> compareResult = ITKTestBase::CompareImages(dataStructure, baselineGeometryPath, baselineDataPath, inputGeometryPath, cellDataPath.createChildPath(outputArrayName), 0.0001);

  COMPLEX_RESULT_REQUIRE_VALID(compareResult)
}

// This test fails because the filter needs a scalar image.
// SimpleITK does an extra conversion step automatically that we don't do.
// In the original ITKImageProcessing, there were no test cases at all.
TEST_CASE("ITKImageProcessing::ITKNormalizeImageFilter(vector)", "[.][ITKImageProcessing][ITKNormalizeImage][vector]")
{
  DataStructure dataStructure;
  ITKNormalizeImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataPath);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/VM1111Shrink-RGB.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult)

  Arguments args;
  args.insertOrAssign(ITKNormalizeImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKNormalizeImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKNormalizeImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  fs::path baselineFilePath = fs::path(complex::unit_test::k_DataDir.view()) / "JSONFilters/Baseline/BasicFilters_NormalizeImageFilter_vector.nrrd";
  DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
  DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
  Result<> readBaselineResult = ITKTestBase::ReadImage(dataStructure, baselineFilePath, baselineGeometryPath, baseLineCellDataPath, baselineDataPath);
  Result<> compareResult = ITKTestBase::CompareImages(dataStructure, baselineGeometryPath, baselineDataPath, inputGeometryPath, cellDataPath.createChildPath(outputArrayName), 0.0001);
  COMPLEX_RESULT_REQUIRE_VALID(compareResult)
}
