#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImportImageStack.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

#include "ITKTestBase.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

using namespace complex;

namespace fs = std::filesystem;

namespace
{
const std::string k_ImageStackDir = unit_test::k_DataDir.str() + "/ImageStack";
const DataPath k_ImageGeomPath = {{"ImageGeometry"}};
const DataPath k_ImageDataPath = k_ImageGeomPath.createChildPath("ImageData");
} // namespace

TEST_CASE("ITKImageProcessing::ITKImportImageStack: NoInput", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure ds;
  Arguments args;

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: NoImageGeometry", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure ds;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;

  fileListInfo.inputPath = k_ImageStackDir;

  args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: NoFiles", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure ds;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;
  fileListInfo.inputPath = "doesNotExist.ghost";
  fileListInfo.startIndex = 75;
  fileListInfo.endIndex = 77;
  fileListInfo.fileExtension = "dcm";
  fileListInfo.filePrefix = "Image";
  fileListInfo.fileSuffix = "";
  fileListInfo.paddingDigits = 4;

  args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImportImageStack::k_ImageDataArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: FileDoesNotExist", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure ds;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;
  fileListInfo.inputPath = k_ImageStackDir;
  fileListInfo.startIndex = 75;
  fileListInfo.endIndex = 79;
  fileListInfo.fileExtension = "dcm";
  fileListInfo.filePrefix = "Image";
  fileListInfo.fileSuffix = "";
  fileListInfo.paddingDigits = 4;

  args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImportImageStack::k_ImageDataArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: CompareImage", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure ds;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;
  fileListInfo.inputPath = k_ImageStackDir;
  fileListInfo.startIndex = 11;
  fileListInfo.endIndex = 13;
  fileListInfo.incrementIndex = 1;
  fileListInfo.fileExtension = ".tif";
  fileListInfo.filePrefix = "slice_";
  fileListInfo.fileSuffix = "";
  fileListInfo.paddingDigits = 2;
  fileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

  std::vector<float32> origin = {1.0f, 4.0f, 8.0f};
  std::vector<float32> spacing = {0.3f, 0.2f, 0.9f};

  args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
  args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
  args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImportImageStack::k_ImageDataArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto* imageGeom = ds.getDataAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeom != nullptr);

  SizeVec3 imageDims = imageGeom->getDimensions();
  FloatVec3 imageOrigin = imageGeom->getOrigin();
  FloatVec3 imageSpacing = imageGeom->getSpacing();

  std::array<usize, 3> dims = {524, 390, 3};

  REQUIRE(imageDims[0] == dims[0]);
  REQUIRE(imageDims[1] == dims[1]);
  REQUIRE(imageDims[2] == dims[2]);

  REQUIRE(imageOrigin[0] == Approx(origin[0]));
  REQUIRE(imageOrigin[1] == Approx(origin[1]));
  REQUIRE(imageOrigin[2] == Approx(origin[2]));

  REQUIRE(imageSpacing[0] == Approx(spacing[0]));
  REQUIRE(imageSpacing[1] == Approx(spacing[1]));
  REQUIRE(imageSpacing[2] == Approx(spacing[2]));

  const auto* imageData = ds.getDataAs<UInt8Array>(k_ImageDataPath);
  REQUIRE(imageData != nullptr);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, k_ImageDataPath);
  REQUIRE(md5Hash == "2620b39f0dcaa866602c2591353116a4");
}
