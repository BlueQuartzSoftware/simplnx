#include <catch2/catch.hpp>

#include "ITKImageProcessing/Common/sitkCommon.hpp"
#include "ITKImageProcessing/Filters/ITKRelabelComponentImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("ITKImageProcessing::ITKRelabelComponentImageFilter(default)", "[ITKImageProcessing][ITKRelabelComponentImage][default]")
{
  DataStructure dataStructure;
  const ITKRelabelComponentImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/2th_cthead1.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKRelabelComponentImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKRelabelComponentImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKRelabelComponentImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "58af064e929f08f9d5bacc8be44ed92e");
}

TEST_CASE("ITKImageProcessing::ITKRelabelComponentImageFilter(no_sorting)", "[ITKImageProcessing][ITKRelabelComponentImage][no_sorting]")
{
  DataStructure dataStructure;
  const ITKRelabelComponentImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/simple-label-d.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKRelabelComponentImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKRelabelComponentImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKRelabelComponentImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKRelabelComponentImage::k_SortByObjectSize_Key, std::make_any<BoolParameter::ValueType>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "0231da8387aa665ddff9c2645e71f213");
}

TEST_CASE("ITKImageProcessing::ITKRelabelComponentImageFilter(no_sorting2)", "[ITKImageProcessing][ITKRelabelComponentImage][no_sorting2]")
{
  DataStructure dataStructure;
  const ITKRelabelComponentImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/simple-label-d.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKRelabelComponentImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKRelabelComponentImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKRelabelComponentImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKRelabelComponentImage::k_SortByObjectSize_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(ITKRelabelComponentImage::k_MinimumObjectSize_Key, std::make_any<UInt64Parameter::ValueType>(140));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "4c81464832a0270041334abd744b94b0");
}
