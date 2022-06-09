#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKGrayscaleDilateImage.hpp"

#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKGrayscaleDilateImageFilter(GrayscaleDilate)", "[ITKImageProcessing][ITKGrayscaleDilateImage][GrayscaleDilate]")
{
  DataStructure ds;
  ITKGrayscaleDilateImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  { // Start Image Comparison Scope
    fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/STAPLE1.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
    COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKGrayscaleDilateImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKGrayscaleDilateImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKGrayscaleDilateImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKGrayscaleDilateImage::k_KernelRadius_Key, std::make_any<VectorParameter<uint32>::ValueType>(std::vector<uint32>{1, 1, 1}));
  args.insertOrAssign(ITKGrayscaleDilateImage::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(itk::simple::sitkBall));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "cb692559f1eb21e4c932f6bbb3850ad3");
}
