#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageReaderFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Read PNG", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / "test/data/PngTest.png";

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, inputFilePath);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_InputDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(inputGeometryPath);
  REQUIRE(imageGeom != nullptr);

  SizeVec3 imageDims = imageGeom->getDimensions();
  const SizeVec3 expectedImageDims = {64, 64, 1};
  REQUIRE(imageDims == expectedImageDims);

  FloatVec3 imageOrigin = imageGeom->getOrigin();
  const FloatVec3 expectedImageOrigin = {0.0f, 0.0f, 0.0f};
  REQUIRE(imageOrigin == expectedImageOrigin);

  FloatVec3 imageSpacing = imageGeom->getSpacing();
  const FloatVec3 expectedImageSpacing = {1.0f, 1.0f, 1.0f};
  REQUIRE(imageSpacing == expectedImageSpacing);

  const auto* dataArray = dataStructure.getDataAs<DataArray<uint8>>(inputDataPath);
  REQUIRE(dataArray != nullptr);

  const auto& dataStore = dataArray->getIDataStoreRefAs<DataStore<uint8>>();
  std::vector<usize> arrayDims = dataStore.getTupleShape();
  const std::vector<usize> expectedArrayDims = {1, 64, 64};
  REQUIRE(arrayDims == expectedArrayDims);

  std::vector<usize> arrayComponentDims = dataStore.getComponentShape();
  const std::vector<usize> expectedArrayComponentDims = {3};
  REQUIRE(arrayComponentDims == expectedArrayComponentDims);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Override Origin", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  bool k_ChangeOrigin = false;
  bool k_ChangeResolution = false;

  std::vector<float64> k_Origin{-32.0, -32.0, 0.0};
  std::vector<float64> k_Spacing{1.0, 1.0, 1.0};

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / "test/data/PngTest.png";
  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, inputFilePath);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_InputDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, true);

  args.insert(ITKImageReaderFilter::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(ITKImageReaderFilter::k_CenterOrigin_Key, std::make_any<bool>(false));
  args.insert(ITKImageReaderFilter::k_ChangeSpacing_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(ITKImageReaderFilter::k_Origin_Key, std::make_any<std::vector<float64>>(k_Origin));
  args.insert(ITKImageReaderFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(inputGeometryPath);
  REQUIRE(imageGeom != nullptr);

  SizeVec3 imageDims = imageGeom->getDimensions();
  const SizeVec3 expectedImageDims = {64, 64, 1};
  REQUIRE(imageDims == expectedImageDims);

  std::vector<float64> imageOrigin = imageGeom->getOrigin().toContainer<std::vector<float64>>();
  REQUIRE(imageOrigin == k_Origin);

  std::vector<float64> imageSpacing = imageGeom->getSpacing().toContainer<std::vector<float64>>();
  REQUIRE(imageSpacing == k_Spacing);
}

TEST_CASE("ITKImageProcessing::ITKImageReaderFilter: Centering Origin in Geometry", "[ITKImageProcessing][ITKImageReaderFilter]")
{
  ITKImageReaderFilter filter;
  DataStructure dataStructure;
  Arguments args;

  bool k_ChangeOrigin = true;
  bool k_ChangeResolution = false;

  std::vector<float64> k_Origin{0.0, 0.0, 0.0};
  std::vector<float64> k_Spacing{1.0, 1.0, 1.0};

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / "test/data/PngTest.png";
  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);

  args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, inputFilePath);
  args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_ImageCellDataName));
  args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(ITKTestBase::k_InputDataName));

  args.insert(ITKImageReaderFilter::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(ITKImageReaderFilter::k_CenterOrigin_Key, std::make_any<bool>(true));
  args.insert(ITKImageReaderFilter::k_ChangeSpacing_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(ITKImageReaderFilter::k_Origin_Key, std::make_any<std::vector<float64>>(k_Origin));
  args.insert(ITKImageReaderFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(inputGeometryPath);
  REQUIRE(imageGeom != nullptr);

  SizeVec3 imageDims = imageGeom->getDimensions();
  const SizeVec3 expectedImageDims = {64, 64, 1};
  REQUIRE(imageDims == expectedImageDims);

  std::vector<float64> imageOrigin = imageGeom->getOrigin().toContainer<std::vector<float64>>();
  REQUIRE(imageOrigin == std::vector<float64>{-32.0, -32.0, -0.5});

  std::vector<float64> imageSpacing = imageGeom->getSpacing().toContainer<std::vector<float64>>();
  REQUIRE(imageSpacing == k_Spacing);
}
