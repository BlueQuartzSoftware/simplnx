#include <catch2/catch.hpp>

#include "ITKImageProcessing/Common/sitkCommon.hpp"
#include "ITKImageProcessing/Filters/ITKMaximumProjectionImageFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("ITKImageProcessing::ITKMaximumProjectionImageFilter: Default Test", "[ITKImageProcessing][ITKMaximumProjectionImage][defaults]")
{
  DataStructure dataStructure;
  const ITKMaximumProjectionImageFilter filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Float.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "fb78c55635b17fc9ff38ef0ef14f0948");
}

TEST_CASE("ITKImageProcessing::ITKMaximumProjectionImageFilter: New Geometry Default Test", "[ITKImageProcessing][ITKMaximumProjectionImage][defaults]")
{
  DataStructure dataStructure;
  const ITKMaximumProjectionImageFilter filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;
  const DataObjectNameParameter::ValueType outputImageName = "New Image Geometry";
  const DataPath outputDataPath = DataPath({outputImageName, ITKTestBase::k_ImageCellDataName, outputArrayName});

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Float.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputImageName));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, outputDataPath);
  REQUIRE(md5Hash == "fb78c55635b17fc9ff38ef0ef14f0948");
}

TEST_CASE("ITKImageProcessing::ITKMaximumProjectionImageFilter: Dimensional Test", "[ITKImageProcessing][ITKMaximumProjectionImage][defaults]")
{
  DataStructure dataStructure;
  const ITKMaximumProjectionImageFilter filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Float.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(2));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "f3f0d97c83c6b0d92df10c28e2481520");
}

TEST_CASE("ITKImageProcessing::ITKMaximumProjectionImageFilter: Image Short Test", "[ITKImageProcessing][ITKMaximumProjectionImage][defaults]")
{
  DataStructure dataStructure;
  const ITKMaximumProjectionImageFilter filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/Ramp-Up-Short.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(1));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "5390344262c91e83bc9208b0991a2fc9");
}

/**
 * TODO: Review RGB/ARGB Image Functionality with Filter
 * In the current implementation RGB/ARGB has been disabled due to the inability to validate test case. The below test
 * case is an adaptation from SIMPL, however, the MD5 doesn't match. In old SIMPL, when you attempt to replicate pipeline
 * an error states it doesn't allow RGB images, however the old test case is passing. For the time being the functionality
 * has been disabled.
 */
// TEST_CASE("ITKImageProcessing::ITKMaximumProjectionImageFilter: Image RGB Test", "[ITKImageProcessing][ITKMaximumProjectionImage][defaults]")
//{
//   DataStructure dataStructure;
//   const ITKMaximumProjectionImageFilter filter;
//
//   const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
//   const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
//   const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
//   const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;
//
//   { // Start Image Comparison Scope
//     const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/VM1111Shrink-RGB.png";
//     Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
//     SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
//   } // End Image Comparison Scope
//
//   Arguments args;
//   args.insertOrAssign(ITKMaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
//   args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
//   args.insertOrAssign(ITKMaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
//   args.insertOrAssign(ITKMaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
//   args.insertOrAssign(ITKMaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(1));
//
//   auto preflightResult = filter.preflight(dataStructure, args);
//   SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
//
//   auto executeResult = filter.execute(dataStructure, args);
//   SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
//
//   const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
//   REQUIRE(md5Hash == "92134e0dd38fccdf054ff23e67a72e75");
// }
