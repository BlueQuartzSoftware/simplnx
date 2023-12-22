#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKInvertIntensityImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("ITKImageProcessing::ITKInvertIntensityImageFilter(3d)", "[ITKImageProcessing][ITKInvertIntensityImage][3d]")
{
  DataStructure dataStructure;
  ITKInvertIntensityImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataPath);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Short.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKInvertIntensityImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKInvertIntensityImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKInvertIntensityImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "76765a57f26a7979f33efc8ed9801a55");
}
