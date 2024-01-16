#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImportImageStack.hpp"
#include "ITKImageProcessing/Filters/ITKImageReader.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
const std::string k_ImageStackDir = unit_test::k_DataDir.str() + "/ImageStack";
const DataPath k_ImageGeomPath = {{"ImageGeometry"}};
const DataPath k_ImageDataPath = k_ImageGeomPath.createChildPath(ImageGeom::k_CellDataName).createChildPath("ImageData");
const std::string k_FlippedImageStackDirName = "image_flip_test_images";

void FillFlippedArgs(Arguments& args)
{
  std::vector<float32> origin = {1.0f, 4.0f, 8.0f};
  std::vector<float32> spacing = {0.3f, 0.2f, 0.9f};

  args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
  args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
  args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
}

void CompareFlippedGeometries(const DataStructure& dataStructure, const ImageGeom* baseGeom, const ImageGeom* xFlipGeom, const ImageGeom* yFlipGeom)
{

  REQUIRE(baseGeom != nullptr);
  REQUIRE(xFlipGeom != nullptr);
  REQUIRE(yFlipGeom != nullptr);

  SizeVec3 imageDims = baseGeom->getDimensions();
  FloatVec3 imageOrigin = baseGeom->getOrigin();
  FloatVec3 imageSpacing = baseGeom->getSpacing();

  SizeVec3 dims = xFlipGeom->getDimensions();

  REQUIRE(imageDims[0] == dims[0]);
  REQUIRE(imageDims[1] == dims[1]);
  REQUIRE(imageDims[2] == dims[2]);

  FloatVec3 origin = xFlipGeom->getOrigin();

  REQUIRE(imageOrigin[0] == Approx(origin[0]));
  REQUIRE(imageOrigin[1] == Approx(origin[1]));
  REQUIRE(imageOrigin[2] == Approx(origin[2]));

  FloatVec3 spacing = xFlipGeom->getSpacing();

  REQUIRE(imageSpacing[0] == Approx(spacing[0]));
  REQUIRE(imageSpacing[1] == Approx(spacing[1]));
  REQUIRE(imageSpacing[2] == Approx(spacing[2]));

  dims = yFlipGeom->getDimensions();

  REQUIRE(imageDims[0] == dims[0]);
  REQUIRE(imageDims[1] == dims[1]);
  REQUIRE(imageDims[2] == dims[2]);

  origin = yFlipGeom->getOrigin();

  REQUIRE(imageOrigin[0] == Approx(origin[0]));
  REQUIRE(imageOrigin[1] == Approx(origin[1]));
  REQUIRE(imageOrigin[2] == Approx(origin[2]));

  spacing = yFlipGeom->getSpacing();

  REQUIRE(imageSpacing[0] == Approx(spacing[0]));
  REQUIRE(imageSpacing[1] == Approx(spacing[1]));
  REQUIRE(imageSpacing[2] == Approx(spacing[2]));

  //const auto* imageData = dataStructure.getDataAs<UInt8Array>(k_ImageDataPath);
  //REQUIRE(imageData != nullptr);
}
} // namespace

TEST_CASE("ITKImageProcessing::ITKImportImageStack: NoInput", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure dataStructure;
  Arguments args;

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: NoImageGeometry", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure dataStructure;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;

  fileListInfo.inputPath = k_ImageStackDir;

  args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: NoFiles", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure dataStructure;
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

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: FileDoesNotExist", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure dataStructure;
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

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: CompareImage", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure dataStructure;
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

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
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

  const auto* imageData = dataStructure.getDataAs<UInt8Array>(k_ImageDataPath);
  REQUIRE(imageData != nullptr);

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, k_ImageDataPath);
  REQUIRE(md5Hash == "2620b39f0dcaa866602c2591353116a4");
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: Flipped Image Even-Even X/Y", "[ITKImageProcessing][ITKImportImageStack]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  fs::path k_ImageFlipStackDir = fs::path(fmt::format("{}/{}", unit_test::k_TestFilesDir, k_FlippedImageStackDirName));
  const std::string k_FilePrefix = "image_flip_even_even_";

  DataStructure dataStructure;
  {
    ITKImportImageStack filter;
    Arguments args;

    GeneratedFileListParameter::ValueType fileListInfo;
    fileListInfo.inputPath = k_ImageFlipStackDir;
    fileListInfo.startIndex = 1;
    fileListInfo.endIndex = 1;
    fileListInfo.incrementIndex = 1;
    fileListInfo.fileExtension = ".png";
    fileListInfo.filePrefix = k_FilePrefix;
    fileListInfo.fileSuffix = "";
    fileListInfo.paddingDigits = 1;
    fileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

    args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
    ::FillFlippedArgs(args);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  {
    ITKImageReader filter;
    Arguments args;

    fs::path filePath = k_ImageFlipStackDir / (k_FilePrefix + "flip_x.png");
    DataPath arrayPath{{"ImageGeom", "ImageArray"}};
    DataPath imagePath = arrayPath.getParent();
    args.insertOrAssign(ITKImageReader::k_FileName_Key, filePath);
    args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, imagePath);
    args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, arrayPath);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  {
    ITKImageReader filter;
    Arguments args;

    fs::path filePath = k_ImageFlipStackDir / (k_FilePrefix + "flip_y.png");
    DataPath arrayPath{{"ImageGeom", "ImageArray"}};
    DataPath imagePath = arrayPath.getParent();
    args.insertOrAssign(ITKImageReader::k_FileName_Key, filePath);
    args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, imagePath);
    args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, arrayPath);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

//  auto compareResult =
//      ::CompareFlippedGeometries(dataStructure, dataStructure.getDataAs<ImageGeom>(k_BaseGeomPath), dataStructure.getDataAs<ImageGeom>(k_XFlippedGeomPath), dataStructure.getDataAs<ImageGeom>(k_YFlippedGeomPath));
//  SIMPLNX_RESULT_REQUIRE_VALID(compareResult.result)
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: Flipped Image Even-Odd X/Y", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure dataStructure;
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

  args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  ::FillFlippedArgs(args);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

//  auto compareResult =
//      ::CompareFlippedGeometries(dataStructure, dataStructure.getDataAs<ImageGeom>(k_BaseGeomPath), dataStructure.getDataAs<ImageGeom>(k_XFlippedGeomPath), dataStructure.getDataAs<ImageGeom>(k_YFlippedGeomPath));
//  SIMPLNX_RESULT_REQUIRE_VALID(compareResult.result)
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: Flipped Image Odd-Even X/Y", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure dataStructure;
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

  args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  ::FillFlippedArgs(args);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

//  auto compareResult =
//      ::CompareFlippedGeometries(dataStructure, dataStructure.getDataAs<ImageGeom>(k_BaseGeomPath), dataStructure.getDataAs<ImageGeom>(k_XFlippedGeomPath), dataStructure.getDataAs<ImageGeom>(k_YFlippedGeomPath));
//  SIMPLNX_RESULT_REQUIRE_VALID(compareResult.result)
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: Flipped Image Odd-Odd X/Y", "[ITKImageProcessing][ITKImportImageStack]")
{
  ITKImportImageStack filter;
  DataStructure dataStructure;
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

  args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  ::FillFlippedArgs(args);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

//  auto compareResult =
//      ::CompareFlippedGeometries(dataStructure, dataStructure.getDataAs<ImageGeom>(k_BaseGeomPath), dataStructure.getDataAs<ImageGeom>(k_XFlippedGeomPath), dataStructure.getDataAs<ImageGeom>(k_YFlippedGeomPath));
//  SIMPLNX_RESULT_REQUIRE_VALID(compareResult.result)
}
