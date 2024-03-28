#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKMorphologicalWatershedImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("ITKImageProcessing::ITKMorphologicalWatershedImageFilter(defaults)", "[ITKImageProcessing][ITKMorphologicalWatershedImage][defaults]")
{
  DataStructure dataStructure;
  ITKMorphologicalWatershedImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1-grad-mag.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKMorphologicalWatershedImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMorphologicalWatershedImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMorphologicalWatershedImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "406079d7904d4e9ab0b5f29f7a3a1ea8");
}

TEST_CASE("ITKImageProcessing::ITKMorphologicalWatershedImageFilter(level_1)", "[ITKImageProcessing][ITKMorphologicalWatershedImage][level_1]")
{
  DataStructure dataStructure;
  ITKMorphologicalWatershedImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1-grad-mag.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKMorphologicalWatershedImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMorphologicalWatershedImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMorphologicalWatershedImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMorphologicalWatershedImage::k_Level_Key, std::make_any<Float64Parameter::ValueType>(1.0));
  args.insertOrAssign(ITKMorphologicalWatershedImage::k_MarkWatershedLine_Key, std::make_any<BoolParameter::ValueType>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "a204ce7cf8ec4e7bc6538f0515a8910e");
}
