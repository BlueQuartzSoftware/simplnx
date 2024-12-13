#include <catch2/catch.hpp>

#include "ITKImageProcessing/Common/sitkCommon.hpp"
#include "ITKImageProcessing/Filters/ITKMinimumProjectionImageFilter.hpp"
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

TEST_CASE("ITKImageProcessing::ITKMinimumProjectionImageFilter: Default Test", "[ITKImageProcessing][ITKMinimumProjectionImage][defaults]")
{
  DataStructure dataStructure;
  const ITKMinimumProjectionImageFilter filter;

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
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "5591e0307db733396e8cc8143e7f29f7");
}

TEST_CASE("ITKImageProcessing::ITKMinimumProjectionImageFilter: New Geometry Default Test", "[ITKImageProcessing][ITKMinimumProjectionImage][defaults]")
{
  DataStructure dataStructure;
  const ITKMinimumProjectionImageFilter filter;

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
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputImageName));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, outputDataPath);
  REQUIRE(md5Hash == "5591e0307db733396e8cc8143e7f29f7");
}

TEST_CASE("ITKImageProcessing::ITKMinimumProjectionImageFilter: Dimensional Test", "[ITKImageProcessing][ITKMinimumProjectionImage][defaults]")
{
  DataStructure dataStructure;
  const ITKMinimumProjectionImageFilter filter;

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
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(2));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "6c16b87a823ca190294ac8b678ba4300");
}

TEST_CASE("ITKImageProcessing::ITKMinimumProjectionImageFilter: Image Short Test", "[ITKImageProcessing][ITKMinimumProjectionImage][defaults]")
{
  DataStructure dataStructure;
  const ITKMinimumProjectionImageFilter filter;

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
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMinimumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(1));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "c4d83f61ffd5cc3a163155bb5d6a0698");
}

/**
 * TODO: Review RGB/ARGB Image Functionality with Filter
 * In the current implementation RGB/ARGB has been disabled due to the inability to validate test case. The below test
 * case is an adaptation from SIMPL, however, the MD5 doesn't match. In old SIMPL, when you attempt to replicate pipeline
 * an error states it doesn't allow RGB images, however the old test case is passing. For the time being the functionality
 * has been disabled.
 */
// TEST_CASE("ITKImageProcessing::ITKMinimumProjectionImageFilter: Image RGB Test", "[ITKImageProcessing][ITKMinimumProjectionImage][defaults]")
//{
//   DataStructure dataStructure;
//   const ITKMinimumProjectionImageFilter filter;
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
//   args.insertOrAssign(ITKMinimumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
//   args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
//   args.insertOrAssign(ITKMinimumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
//   args.insertOrAssign(ITKMinimumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
//   args.insertOrAssign(ITKMinimumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<UInt32Parameter::ValueType>(1));
//
//   auto preflightResult = filter.preflight(dataStructure, args);
//   SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
//
//   auto executeResult = filter.execute(dataStructure, args);
//   SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
//
//   const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
//   REQUIRE(md5Hash == "344c2d7cf14b5e8b30b266b77a0548c2");
// }
