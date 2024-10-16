#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageReaderFilter.hpp"
#include "ITKImageProcessing/Filters/ITKImportImageStackFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace fs = std::filesystem;

namespace
{
const std::string k_ImageStackDir = unit_test::k_DataDir.str() + "/ImageStack";
const DataPath k_ImageGeomPath = {{"ImageGeometry"}};
const DataPath k_ImageDataPath = k_ImageGeomPath.createChildPath(ImageGeom::k_CellDataName).createChildPath("ImageData");
const std::string k_FlippedImageStackDirName = "image_flip_test_images";
const DataPath k_XGeneratedImageGeomPath = DataPath({"xGeneratedImageGeom"});
const DataPath k_YGeneratedImageGeomPath = DataPath({"yGeneratedImageGeom"});
const DataPath k_XFlipImageGeomPath = DataPath({"xFlipImageGeom"});
const DataPath k_YFlipImageGeomPath = DataPath({"yFlipImageGeom"});
const std::string k_ImageDataName = "ImageData";
const ChoicesParameter::ValueType k_NoImageTransform = 0;
const ChoicesParameter::ValueType k_FlipAboutXAxis = 1;
const ChoicesParameter::ValueType k_FlipAboutYAxis = 2;
const fs::path k_ImageFlipStackDir = fs::path(fmt::format("{}/{}", unit_test::k_TestFilesDir, k_FlippedImageStackDirName));

// Exemplar Array Paths
const DataPath k_XFlippedImageDataPath = k_XFlipImageGeomPath.createChildPath(Constants::k_Cell_Data).createChildPath(::k_ImageDataName);
const DataPath k_YFlippedImageDataPath = k_YFlipImageGeomPath.createChildPath(Constants::k_Cell_Data).createChildPath(::k_ImageDataName);

// Make sure we can instantiate the ITK Import Image Stack Filter
// ITK Image Processing Plugin Uuid
constexpr AbstractPlugin::IdType k_ITKImageProcessingID = *Uuid::FromString("115b0d10-ab97-5a18-88e8-80d35056a28e");
const FilterHandle k_ImportImageStackFilterHandle(nx::core::FilterTraits<ITKImportImageStackFilter>::uuid, k_ITKImageProcessingID);

void ExecuteImportImageStackXY(DataStructure& dataStructure, const std::string& filePrefix)
{
  // Filter needs RotateSampleRefFrameFilter to run
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterListPtr = nx::core::Application::Instance()->getFilterList();
  REQUIRE(filterListPtr != nullptr);

  // Define Shared parameters
  std::vector<float32> k_Origin = {0.0f, 0.0f, 0.0f};
  std::vector<float32> k_Spacing = {1.0f, 1.0f, 1.0f};
  GeneratedFileListParameter::ValueType k_FileListInfo;

  // Set File list for reads
  {
    k_FileListInfo.inputPath = k_ImageFlipStackDir.string();
    k_FileListInfo.startIndex = 1;
    k_FileListInfo.endIndex = 1;
    k_FileListInfo.incrementIndex = 1;
    k_FileListInfo.fileExtension = ".tiff";
    k_FileListInfo.filePrefix = filePrefix;
    k_FileListInfo.fileSuffix = "";
    k_FileListInfo.paddingDigits = 1;
    k_FileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
  }

  // Run generated X flip
  {
    auto importImageStackFilter = filterListPtr->createFilter(::k_ImportImageStackFilterHandle);
    REQUIRE(nullptr != importImageStackFilter);

    Arguments args;

    args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
    args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
    args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
    args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_XGeneratedImageGeomPath));
    args.insertOrAssign(ITKImportImageStackFilter::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutXAxis));

    auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = importImageStackFilter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Run generated Y flip
  {
    auto importImageStackFilter = filterListPtr->createFilter(::k_ImportImageStackFilterHandle);
    REQUIRE(nullptr != importImageStackFilter);

    Arguments args;

    args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
    args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
    args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
    args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_YGeneratedImageGeomPath));
    args.insertOrAssign(ITKImportImageStackFilter::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutYAxis));

    auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = importImageStackFilter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
}

void ReadInFlippedXYExemplars(DataStructure& dataStructure, const std::string& filePrefix)
{
  {
    ITKImageReaderFilter filter;
    Arguments args;

    fs::path filePath = k_ImageFlipStackDir / (filePrefix + "flip_x.tiff");
    args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, filePath);
    args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, ::k_XFlipImageGeomPath);
    args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, ::k_ImageDataName);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  {
    ITKImageReaderFilter filter;
    Arguments args;

    fs::path filePath = k_ImageFlipStackDir / (filePrefix + "flip_y.tiff");
    args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, filePath);
    args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, ::k_YFlipImageGeomPath);
    args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, ::k_ImageDataName);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
}

void CompareXYFlippedGeometries(DataStructure& dataStructure)
{
  UnitTest::CompareImageGeometry(dataStructure, ::k_XFlipImageGeomPath, k_XGeneratedImageGeomPath);
  UnitTest::CompareImageGeometry(dataStructure, ::k_YFlipImageGeomPath, k_YGeneratedImageGeomPath);

  // Processed
  DataPath k_XGeneratedImageDataPath = k_XGeneratedImageGeomPath.createChildPath(Constants::k_Cell_Data).createChildPath(::k_ImageDataName);
  DataPath k_YGeneratedImageDataPath = k_YGeneratedImageGeomPath.createChildPath(Constants::k_Cell_Data).createChildPath(::k_ImageDataName);
  const auto& xGeneratedImageData = dataStructure.getDataRefAs<UInt8Array>(k_XGeneratedImageDataPath);
  const auto& yGeneratedImageData = dataStructure.getDataRefAs<UInt8Array>(k_YGeneratedImageDataPath);

  // Exemplar
  const auto& xFlippedImageData = dataStructure.getDataRefAs<UInt8Array>(k_XFlippedImageDataPath);
  const auto& yFlippedImageData = dataStructure.getDataRefAs<UInt8Array>(k_YFlippedImageDataPath);

  UnitTest::CompareDataArrays<uint8>(xGeneratedImageData, xFlippedImageData);
  UnitTest::CompareDataArrays<uint8>(yGeneratedImageData, yFlippedImageData);
}
} // namespace

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: NoInput", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  ITKImportImageStackFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: NoImageGeometry", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  ITKImportImageStackFilter filter;
  DataStructure dataStructure;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;

  fileListInfo.inputPath = k_ImageStackDir;

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: NoFiles", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  ITKImportImageStackFilter filter;
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

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: FileDoesNotExist", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  ITKImportImageStackFilter filter;
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

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: CompareImage", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(unit_test::k_BuildDir.view());

  ITKImportImageStackFilter filter;
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

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeomPtr != nullptr);

  SizeVec3 imageDims = imageGeomPtr->getDimensions();
  FloatVec3 imageOrigin = imageGeomPtr->getOrigin();
  FloatVec3 imageSpacing = imageGeomPtr->getSpacing();

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

  const auto* imageDataPtr = dataStructure.getDataAs<UInt8Array>(k_ImageDataPath);
  REQUIRE(imageDataPtr != nullptr);

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, k_ImageDataPath);
  REQUIRE(md5Hash == "2620b39f0dcaa866602c2591353116a4");
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Flipped Image Even-Even X/Y", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  const std::string k_FilePrefix = "image_flip_even_even_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ITKImportImageStackFilter
  ::ExecuteImportImageStackXY(dataStructure, k_FilePrefix);

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, k_FilePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/even_even_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Flipped Image Even-Odd X/Y", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  const std::string k_FilePrefix = "image_flip_even_odd_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ITKImportImageStackFilter
  ::ExecuteImportImageStackXY(dataStructure, k_FilePrefix);

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, k_FilePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/even_odd_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Flipped Image Odd-Even X/Y", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  const std::string k_FilePrefix = "image_flip_odd_even_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ITKImportImageStackFilter
  ::ExecuteImportImageStackXY(dataStructure, k_FilePrefix);

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, k_FilePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/odd_even_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Flipped Image Odd-Odd X/Y", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  const std::string k_FilePrefix = "image_flip_odd_odd_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ITKImportImageStackFilter
  ::ExecuteImportImageStackXY(dataStructure, k_FilePrefix);

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, k_FilePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/odd_odd_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: RGB_To_Grayscale", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(unit_test::k_BuildDir.view());

  ITKImportImageStackFilter filter;
  DataStructure dataStructure;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;
  fileListInfo.inputPath = k_ImageStackDir;
  fileListInfo.startIndex = 0;
  fileListInfo.endIndex = 2;
  fileListInfo.incrementIndex = 1;
  fileListInfo.fileExtension = ".png";
  fileListInfo.filePrefix = "rgb_";
  fileListInfo.fileSuffix = "";
  fileListInfo.paddingDigits = 1;
  fileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

  std::vector<float32> origin = {1.0f, 4.0f, 8.0f};
  std::vector<float32> spacing = {0.3f, 0.2f, 0.9f};

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImportImageStackFilter::k_ConvertToGrayScale_Key, std::make_any<BoolParameter::ValueType>(true));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeomPtr != nullptr);

  SizeVec3 imageDims = imageGeomPtr->getDimensions();
  FloatVec3 imageOrigin = imageGeomPtr->getOrigin();
  FloatVec3 imageSpacing = imageGeomPtr->getSpacing();

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

  const auto* imageDataPtr = dataStructure.getDataAs<UInt8Array>(k_ImageDataPath);
  REQUIRE(imageDataPtr != nullptr);

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, k_ImageDataPath);
  REQUIRE(md5Hash == "2620b39f0dcaa866602c2591353116a4");
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: RGB", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(unit_test::k_BuildDir.view());

  ITKImportImageStackFilter filter;
  DataStructure dataStructure;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;
  fileListInfo.inputPath = k_ImageStackDir;
  fileListInfo.startIndex = 0;
  fileListInfo.endIndex = 2;
  fileListInfo.incrementIndex = 1;
  fileListInfo.fileExtension = ".png";
  fileListInfo.filePrefix = "rgb_";
  fileListInfo.fileSuffix = "";
  fileListInfo.paddingDigits = 1;
  fileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

  std::vector<float32> origin = {1.0f, 4.0f, 8.0f};
  std::vector<float32> spacing = {0.3f, 0.2f, 0.9f};

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImportImageStackFilter::k_ConvertToGrayScale_Key, std::make_any<BoolParameter::ValueType>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeomPtr != nullptr);

  SizeVec3 imageDims = imageGeomPtr->getDimensions();
  FloatVec3 imageOrigin = imageGeomPtr->getOrigin();
  FloatVec3 imageSpacing = imageGeomPtr->getSpacing();

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

  const auto* imageDataPtr = dataStructure.getDataAs<UInt8Array>(k_ImageDataPath);
  REQUIRE(imageDataPtr != nullptr);

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, k_ImageDataPath);
  REQUIRE(md5Hash == "8b0b0393d6779156c88544bc4d75d3fc");
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Resampled Scaled", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(unit_test::k_BuildDir.view());

  ITKImportImageStackFilter filter;
  DataStructure dataStructure;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;
  fileListInfo.inputPath = k_ImageStackDir;
  fileListInfo.startIndex = 0;
  fileListInfo.endIndex = 2;
  fileListInfo.incrementIndex = 1;
  fileListInfo.fileExtension = ".png";
  fileListInfo.filePrefix = "rgb_";
  fileListInfo.fileSuffix = "";
  fileListInfo.paddingDigits = 1;
  fileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

  std::vector<float32> origin = {1.0f, 4.0f, 8.0f};
  std::vector<float32> spacing = {0.3f, 0.2f, 0.9f};

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImportImageStackFilter::k_ConvertToGrayScale_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(ITKImportImageStackFilter::k_ResampleImagesChoice_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(ITKImportImageStackFilter::k_Scaling_Key, std::make_any<Float32Parameter::ValueType>(50.0f));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeomPtr != nullptr);

  SizeVec3 imageDims = imageGeomPtr->getDimensions();
  FloatVec3 imageOrigin = imageGeomPtr->getOrigin();
  FloatVec3 imageSpacing = imageGeomPtr->getSpacing();

  std::array<usize, 3> dims = {262, 195, 3};
  std::vector<float32> new_spacing = {0.6f, 0.4f, 0.9f};

  REQUIRE(imageDims[0] == dims[0]);
  REQUIRE(imageDims[1] == dims[1]);
  REQUIRE(imageDims[2] == dims[2]);

  REQUIRE(imageOrigin[0] == Approx(origin[0]));
  REQUIRE(imageOrigin[1] == Approx(origin[1]));
  REQUIRE(imageOrigin[2] == Approx(origin[2]));

  REQUIRE(imageSpacing[0] == Approx(new_spacing[0]));
  REQUIRE(imageSpacing[1] == Approx(new_spacing[1]));
  REQUIRE(imageSpacing[2] == Approx(new_spacing[2]));

  const auto* imageDataPtr = dataStructure.getDataAs<UInt8Array>(k_ImageDataPath);
  REQUIRE(imageDataPtr != nullptr);

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, k_ImageDataPath);
  REQUIRE(md5Hash == "5969f0ae7507bfae14de3cb470d53e60");
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Resampled Exact Dims", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(unit_test::k_BuildDir.view());

  ITKImportImageStackFilter filter;
  DataStructure dataStructure;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;
  fileListInfo.inputPath = k_ImageStackDir;
  fileListInfo.startIndex = 0;
  fileListInfo.endIndex = 2;
  fileListInfo.incrementIndex = 1;
  fileListInfo.fileExtension = ".png";
  fileListInfo.filePrefix = "rgb_";
  fileListInfo.fileSuffix = "";
  fileListInfo.paddingDigits = 1;
  fileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

  std::vector<float32> origin = {1.0f, 4.0f, 8.0f};
  std::vector<float32> spacing = {0.3f, 0.2f, 0.9f};

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImportImageStackFilter::k_ConvertToGrayScale_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(ITKImportImageStackFilter::k_ResampleImagesChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(ITKImportImageStackFilter::k_ExactXYDimensions_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{100, 100}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeomPtr != nullptr);

  SizeVec3 imageDims = imageGeomPtr->getDimensions();
  FloatVec3 imageOrigin = imageGeomPtr->getOrigin();
  FloatVec3 imageSpacing = imageGeomPtr->getSpacing();

  std::array<usize, 3> dims = {100, 100, 3};
  std::vector<float32> new_spacing = {1.572f, 0.78f, 0.9f};

  REQUIRE(imageDims[0] == dims[0]);
  REQUIRE(imageDims[1] == dims[1]);
  REQUIRE(imageDims[2] == dims[2]);

  REQUIRE(imageOrigin[0] == Approx(origin[0]));
  REQUIRE(imageOrigin[1] == Approx(origin[1]));
  REQUIRE(imageOrigin[2] == Approx(origin[2]));

  REQUIRE(imageSpacing[0] == Approx(new_spacing[0]));
  REQUIRE(imageSpacing[1] == Approx(new_spacing[1]));
  REQUIRE(imageSpacing[2] == Approx(new_spacing[2]));

  const auto* imageDataPtr = dataStructure.getDataAs<UInt8Array>(k_ImageDataPath);
  REQUIRE(imageDataPtr != nullptr);

  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, k_ImageDataPath);
  REQUIRE(md5Hash == "e1e892c7e11eb55a57919053eee66f22");
}
