#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKApproximateSignedDistanceMapImage.hpp"

#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKApproximateSignedDistanceMapImageFilter(default)", "[ITKImageProcessing][ITKApproximateSignedDistanceMapImage][default]")
{
  DataStructure ds;
  ITKApproximateSignedDistanceMapImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  { // Start Image Comparison Scope
    fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/2th_cthead1.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
    COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKApproximateSignedDistanceMapImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKApproximateSignedDistanceMapImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKApproximateSignedDistanceMapImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  fs::path baselineFilePath = fs::path(complex::unit_test::k_DataDir.view()) / "JSONFilters/Baseline/BasicFilters_ApproximateSignedDistanceMapImageFilter_default.nrrd";
  DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
  DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
  Result<> readBaselineResult = ITKTestBase::ReadImage(ds, baselineFilePath, baselineGeometryPath, baseLineCellDataPath, baselineDataPath);
  Result<> compareResult = ITKTestBase::CompareImages(ds, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, 0.01);
  COMPLEX_RESULT_REQUIRE_VALID(compareResult);
}

TEST_CASE("ITKApproximateSignedDistanceMapImageFilter(modified_parms)", "[ITKImageProcessing][ITKApproximateSignedDistanceMapImage][modified_parms]")
{
  DataStructure ds;
  ITKApproximateSignedDistanceMapImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  { // Start Image Comparison Scope
    fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/2th_cthead1.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
    COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKApproximateSignedDistanceMapImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKApproximateSignedDistanceMapImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKApproximateSignedDistanceMapImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKApproximateSignedDistanceMapImage::k_InsideValue_Key, std::make_any<Float64Parameter::ValueType>(100));
  args.insertOrAssign(ITKApproximateSignedDistanceMapImage::k_OutsideValue_Key, std::make_any<Float64Parameter::ValueType>(0));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  fs::path baselineFilePath =
      fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.view() / "JSONFilters/Baseline/BasicFilters_ApproximateSignedDistanceMapImageFilter_modified_parms.nrrd";
  DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
  DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
  Result<> readBaselineResult = ITKTestBase::ReadImage(ds, baselineFilePath, baselineGeometryPath, baseLineCellDataPath, baselineDataPath);
  Result<> compareResult = ITKTestBase::CompareImages(ds, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, 0.01);
  COMPLEX_RESULT_REQUIRE_VALID(compareResult);
}
