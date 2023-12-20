#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("ITKImageProcessing::ITKImageReader: Read PNG", "[ITKImageProcessing][ITKImageReader]")
{
  ITKImageReader filter;
  DataStructure dataStructure;
  Arguments args;

  fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / "test/data/PngTest.png";
  DataPath arrayPath{{"ImageGeom", "ImageArray"}};
  DataPath imagePath = arrayPath.getParent();
  args.insertOrAssign(ITKImageReader::k_FileName_Key, filePath);
  args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, imagePath);
  args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, arrayPath);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(imagePath);
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

  const auto* dataArray = dataStructure.getDataAs<DataArray<uint8>>(arrayPath);
  REQUIRE(dataArray != nullptr);

  const auto& dataStore = dataArray->getIDataStoreRefAs<DataStore<uint8>>();
  std::vector<usize> arrayDims = dataStore.getTupleShape();
  const std::vector<usize> expectedArrayDims = {1, 64, 64};
  REQUIRE(arrayDims == expectedArrayDims);

  std::vector<usize> arrayComponentDims = dataStore.getComponentShape();
  const std::vector<usize> expectedArrayComponentDims = {3};
  REQUIRE(arrayComponentDims == expectedArrayComponentDims);
}
