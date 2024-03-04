#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKMaskImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("ITKImageProcessing::ITKMaskImageFilter(2d)", "[ITKImageProcessing][ITKMaskImage][2d]")
{
  DataStructure dataStructure;
  ITKMaskImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  DataPath maskGeometryPath({ITKTestBase::k_MaskGeometryPath});
  DataPath maskCellDataPath = maskGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  DataPath maskDataPath = maskCellDataPath.createChildPath(ITKTestBase::k_MaskDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/STAPLE1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  fs::path maskInputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/STAPLE2.png";
  Result<> maskImageReadResult = ITKTestBase::ReadImage(dataStructure, maskInputFilePath, maskGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_MaskDataPath);
  SIMPLNX_RESULT_REQUIRE_VALID(maskImageReadResult);

  const auto& inputGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  const auto& maskGeom = dataStructure.getDataRefAs<ImageGeom>(maskGeometryPath);

  REQUIRE(inputGeom.getDimensions() == maskGeom.getDimensions());

  Arguments args;
  args.insertOrAssign(ITKMaskImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaskImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaskImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMaskImage::k_MaskImageDataPath_Key, std::make_any<DataPath>(maskDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "c57d7fda3e42374881c3c3181d15bf90");
}

TEST_CASE("ITKImageProcessing::ITKMaskImageFilter(cthead1)", "[ITKImageProcessing][ITKMaskImage][cthead1]")
{
  DataStructure dataStructure;
  ITKMaskImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  DataPath maskGeometryPath({ITKTestBase::k_MaskGeometryPath});
  DataPath maskCellDataPath = maskGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  DataPath maskDataPath = maskCellDataPath.createChildPath(ITKTestBase::k_MaskDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1-Float.mha";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  fs::path maskInputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1-mask.png";
  Result<> maskImageReadResult = ITKTestBase::ReadImage(dataStructure, maskInputFilePath, maskGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_MaskDataPath);
  SIMPLNX_RESULT_REQUIRE_VALID(maskImageReadResult);

  const auto& inputGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  const auto& maskGeom = dataStructure.getDataRefAs<ImageGeom>(maskGeometryPath);

  REQUIRE(inputGeom.getDimensions() == maskGeom.getDimensions());

  Arguments args;
  args.insertOrAssign(ITKMaskImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaskImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaskImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMaskImage::k_MaskImageDataPath_Key, std::make_any<DataPath>(maskDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "0ef8943803bb4a21b2015b53f0164f1c");
}

TEST_CASE("ITKImageProcessing::ITKMaskImageFilter(rgb)", "[ITKImageProcessing][ITKMaskImage][rgb]")
{
  DataStructure dataStructure;
  ITKMaskImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  DataPath maskGeometryPath({ITKTestBase::k_MaskGeometryPath});
  DataPath maskCellDataPath = maskGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  DataPath maskDataPath = maskCellDataPath.createChildPath(ITKTestBase::k_MaskDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/VM1111Shrink-RGB.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  fs::path maskInputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/VM1111Shrink-mask.png";
  Result<> maskImageReadResult = ITKTestBase::ReadImage(dataStructure, maskInputFilePath, maskGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_MaskDataPath);
  SIMPLNX_RESULT_REQUIRE_VALID(maskImageReadResult);

  const auto& inputGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  const auto& maskGeom = dataStructure.getDataRefAs<ImageGeom>(maskGeometryPath);

  REQUIRE(inputGeom.getDimensions() == maskGeom.getDimensions());

  Arguments args;
  args.insertOrAssign(ITKMaskImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaskImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaskImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKMaskImage::k_MaskImageDataPath_Key, std::make_any<DataPath>(maskDataPath));
  args.insertOrAssign(ITKMaskImage::k_OutsideValue_Key, std::make_any<Float64Parameter::ValueType>(10.0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "3dad4a416a7b6a198a4a916d65d7654f");
}

// Disabled this test because requires masking value which doesn't exist in the original
#if 0
TEST_CASE("ITKMaskImageFilter(cthead1_maskvalue)", "[ITKImageProcessing][ITKMaskImage][cthead1_maskvalue]")
{
  DataStructure dataStructure;
  ITKMaskImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataName);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  DataPath maskGeometryPath({ITKTestBase::k_MaskGeometryPath});
  DataPath maskDataPath = maskGeometryPath.createChildPath(ITKTestBase::k_MaskDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/cthead1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
  SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)

  fs::path maskInputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/2th_cthead1.mha";
  Result<> maskImageReadResult = ITKTestBase::ReadImage(dataStructure, maskInputFilePath, inputGeometryPath, maskDataPath);
  SIMPLNX_RESULT_REQUIRE_VALID(maskImageReadResult);

  const auto& inputGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  const auto& maskGeom = dataStructure.getDataRefAs<ImageGeom>(maskGeometryPath);

  REQUIRE(inputGeom.getDimensions() == maskGeom.getDimensions());

  Arguments args;
  args.insertOrAssign(ITKMaskImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKMaskImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKMaskImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKMaskImage::k_MaskImageDataPath_Key, std::make_any<DataPath>(maskDataPath));
  args.insertOrAssign(ITKMaskImage::k_MaskingValue_Key, std::make_any<Float64Parameter::ValueType>(100.0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));
  REQUIRE(md5Hash == "3eb703113d03f38e7b8db4b180079a39");
}
#endif
