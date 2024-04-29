#include <catch2/catch.hpp>

#include "ITKImageProcessing/Common/sitkCommon.hpp"
#include "ITKImageProcessing/Filters/ITKGrayscaleGrindPeakImageFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("ITKImageProcessing::ITKGrayscaleGrindPeakImageFilter(GrayscaleGrindPeak1)", "[ITKImageProcessing][ITKGrayscaleGrindPeakImage][GrayscaleGrindPeak1]")
{
  DataStructure dataStructure;
  const ITKGrayscaleGrindPeakImageFilter filter;

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
  args.insertOrAssign(ITKGrayscaleGrindPeakImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKGrayscaleGrindPeakImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKGrayscaleGrindPeakImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "084cdd1d64664ebfab26c2e0ed382e14");
}

TEST_CASE("ITKImageProcessing::ITKGrayscaleGrindPeakImageFilter(GrayscaleGrindPeak2)", "[ITKImageProcessing][ITKGrayscaleGrindPeakImage][GrayscaleGrindPeak2]")
{
  DataStructure dataStructure;
  const ITKGrayscaleGrindPeakImageFilter filter;

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
  args.insertOrAssign(ITKGrayscaleGrindPeakImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKGrayscaleGrindPeakImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKGrayscaleGrindPeakImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "b18d75cccb9361c65b40bb5c0d3c6e0d");
}
