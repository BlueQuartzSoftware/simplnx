#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"
#include "ITKImageProcessing/Filters/ITKImportImageStack.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
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
const DataPath k_XGeneratedImageGeomPath = DataPath({"xGeneratedImageGeom"});
const DataPath k_YGeneratedImageGeomPath = DataPath({"yGeneratedImageGeom"});
const DataPath k_XFlipImageGeomPath = DataPath({"xFlipImageGeom"});
const DataPath k_YFlipImageGeomPath = DataPath({"yFlipImageGeom"});
const std::string k_ImageDataName = "ImageData";
const ChoicesParameter::ValueType k_NoImageTransform = 0;
const ChoicesParameter::ValueType k_FlipAboutXAxis = 1;
const ChoicesParameter::ValueType k_FlipAboutYAxis = 2;

// Exemplar Array Paths
const DataPath k_XFlippedImageDataPath = k_XFlipImageGeomPath.createChildPath(Constants::k_Cell_Data).createChildPath(::k_ImageDataName);
const DataPath k_YFlippedImageDataPath = k_YFlipImageGeomPath.createChildPath(Constants::k_Cell_Data).createChildPath(::k_ImageDataName);

// Make sure we can instantiate the ITK Import Image Stack Filter
// ITK Image Processing Plugin Uuid
constexpr AbstractPlugin::IdType k_ITKImageProcessingID = *Uuid::FromString("115b0d10-ab97-5a18-88e8-80d35056a28e");
const FilterHandle k_ImportImageStackFilterHandle(nx::core::FilterTraits<ITKImportImageStack>::uuid, k_ITKImageProcessingID);

void ReadInFlippedXYExemplars(DataStructure& dataStructure, const std::string& filePrefix, const fs::path& imageFlipStackDir)
{
  {
    ITKImageReader filter;
    Arguments args;

    fs::path filePath = imageFlipStackDir / (filePrefix + "flip_x.png");
    args.insertOrAssign(ITKImageReader::k_FileName_Key, filePath);
    args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, ::k_XFlipImageGeomPath);
    args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, ::k_XFlippedImageDataPath);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  {
    ITKImageReader filter;
    Arguments args;

    fs::path filePath = imageFlipStackDir / (filePrefix + "flip_y.png");
    args.insertOrAssign(ITKImageReader::k_FileName_Key, filePath);
    args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, ::k_YFlipImageGeomPath);
    args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, ::k_YFlippedImageDataPath);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
}

void CompareXYFlippedGeometries(DataStructure& dataStructure)
{
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

  // Filter needs RotateSampleRefFrameFilter to run
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = nx::core::Application::Instance()->getFilterList();
  REQUIRE(filterList != nullptr);

  DataStructure dataStructure;
  {
    // Define Shared parameters
    std::vector<float32> k_Origin = {0.0f, 0.0f, 0.0f};
    std::vector<float32> k_Spacing = {1.0f, 1.0f, 1.0f};
    GeneratedFileListParameter::ValueType k_FileListInfo;
    // Set File list for reads
    {
      k_FileListInfo.inputPath = k_ImageFlipStackDir;
      k_FileListInfo.startIndex = 1;
      k_FileListInfo.endIndex = 1;
      k_FileListInfo.incrementIndex = 1;
      k_FileListInfo.fileExtension = ".png";
      k_FileListInfo.filePrefix = k_FilePrefix;
      k_FileListInfo.fileSuffix = "";
      k_FileListInfo.paddingDigits = 1;
      k_FileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
    }

    // Run generated X flip
    {
      auto importImageStackFilter = filterList->createFilter(::k_ImportImageStackFilterHandle);
      REQUIRE(nullptr != importImageStackFilter);

      Arguments args;

      args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
      args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
      args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
      args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_XGeneratedImageGeomPath));
      args.insertOrAssign(ITKImportImageStack::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutXAxis));

      auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = importImageStackFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

    // Run generated Y flip
    {
      auto importImageStackFilter = filterList->createFilter(::k_ImportImageStackFilterHandle);
      REQUIRE(nullptr != importImageStackFilter);

      Arguments args;

      args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
      args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
      args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
      args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_YGeneratedImageGeomPath));
      args.insertOrAssign(ITKImportImageStack::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutYAxis));

      auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = importImageStackFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
  }

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, k_FilePrefix, k_ImageFlipStackDir);

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: Flipped Image Even-Odd X/Y", "[ITKImageProcessing][ITKImportImageStack]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  fs::path k_ImageFlipStackDir = fs::path(fmt::format("{}/{}", unit_test::k_TestFilesDir, k_FlippedImageStackDirName));
  const std::string k_FilePrefix = "image_flip_even_odd_";

  // Filter needs RotateSampleRefFrameFilter to run
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = nx::core::Application::Instance()->getFilterList();
  REQUIRE(filterList != nullptr);

  DataStructure dataStructure;
  {
    // Define Shared parameters
    std::vector<float32> k_Origin = {0.0f, 0.0f, 0.0f};
    std::vector<float32> k_Spacing = {1.0f, 1.0f, 1.0f};
    GeneratedFileListParameter::ValueType k_FileListInfo;
    // Set File list for reads
    {
      k_FileListInfo.inputPath = k_ImageFlipStackDir;
      k_FileListInfo.startIndex = 1;
      k_FileListInfo.endIndex = 1;
      k_FileListInfo.incrementIndex = 1;
      k_FileListInfo.fileExtension = ".png";
      k_FileListInfo.filePrefix = k_FilePrefix;
      k_FileListInfo.fileSuffix = "";
      k_FileListInfo.paddingDigits = 1;
      k_FileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
    }

    // Run generated X flip
    {
      auto importImageStackFilter = filterList->createFilter(::k_ImportImageStackFilterHandle);
      REQUIRE(nullptr != importImageStackFilter);

      Arguments args;

      args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
      args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
      args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
      args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_XGeneratedImageGeomPath));
      args.insertOrAssign(ITKImportImageStack::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutXAxis));

      auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = importImageStackFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

    // Run generated Y flip
    {
      auto importImageStackFilter = filterList->createFilter(::k_ImportImageStackFilterHandle);
      REQUIRE(nullptr != importImageStackFilter);

      Arguments args;

      args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
      args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
      args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
      args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_YGeneratedImageGeomPath));
      args.insertOrAssign(ITKImportImageStack::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutYAxis));

      auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = importImageStackFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
  }

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, k_FilePrefix, k_ImageFlipStackDir);

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: Flipped Image Odd-Even X/Y", "[ITKImageProcessing][ITKImportImageStack]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  fs::path k_ImageFlipStackDir = fs::path(fmt::format("{}/{}", unit_test::k_TestFilesDir, k_FlippedImageStackDirName));
  const std::string k_FilePrefix = "image_flip_odd_even_";

  // Filter needs RotateSampleRefFrameFilter to run
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = nx::core::Application::Instance()->getFilterList();
  REQUIRE(filterList != nullptr);

  DataStructure dataStructure;
  {
    // Define Shared parameters
    std::vector<float32> k_Origin = {0.0f, 0.0f, 0.0f};
    std::vector<float32> k_Spacing = {1.0f, 1.0f, 1.0f};
    GeneratedFileListParameter::ValueType k_FileListInfo;
    // Set File list for reads
    {
      k_FileListInfo.inputPath = k_ImageFlipStackDir;
      k_FileListInfo.startIndex = 1;
      k_FileListInfo.endIndex = 1;
      k_FileListInfo.incrementIndex = 1;
      k_FileListInfo.fileExtension = ".png";
      k_FileListInfo.filePrefix = k_FilePrefix;
      k_FileListInfo.fileSuffix = "";
      k_FileListInfo.paddingDigits = 1;
      k_FileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
    }

    // Run generated X flip
    {
      auto importImageStackFilter = filterList->createFilter(::k_ImportImageStackFilterHandle);
      REQUIRE(nullptr != importImageStackFilter);

      Arguments args;

      args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
      args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
      args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
      args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_XGeneratedImageGeomPath));
      args.insertOrAssign(ITKImportImageStack::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutXAxis));

      auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = importImageStackFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

    // Run generated Y flip
    {
      auto importImageStackFilter = filterList->createFilter(::k_ImportImageStackFilterHandle);
      REQUIRE(nullptr != importImageStackFilter);

      Arguments args;

      args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
      args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
      args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
      args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_YGeneratedImageGeomPath));
      args.insertOrAssign(ITKImportImageStack::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutYAxis));

      auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = importImageStackFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
  }

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, k_FilePrefix, k_ImageFlipStackDir);

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStack: Flipped Image Odd-Odd X/Y", "[ITKImageProcessing][ITKImportImageStack]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  fs::path k_ImageFlipStackDir = fs::path(fmt::format("{}/{}", unit_test::k_TestFilesDir, k_FlippedImageStackDirName));
  const std::string k_FilePrefix = "image_flip_odd_odd_";

  // Filter needs RotateSampleRefFrameFilter to run
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = nx::core::Application::Instance()->getFilterList();
  REQUIRE(filterList != nullptr);

  DataStructure dataStructure;
  {
    // Define Shared parameters
    std::vector<float32> k_Origin = {0.0f, 0.0f, 0.0f};
    std::vector<float32> k_Spacing = {1.0f, 1.0f, 1.0f};
    GeneratedFileListParameter::ValueType k_FileListInfo;
    // Set File list for reads
    {
      k_FileListInfo.inputPath = k_ImageFlipStackDir;
      k_FileListInfo.startIndex = 1;
      k_FileListInfo.endIndex = 1;
      k_FileListInfo.incrementIndex = 1;
      k_FileListInfo.fileExtension = ".png";
      k_FileListInfo.filePrefix = k_FilePrefix;
      k_FileListInfo.fileSuffix = "";
      k_FileListInfo.paddingDigits = 1;
      k_FileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
    }

    // Run generated X flip
    {
      auto importImageStackFilter = filterList->createFilter(::k_ImportImageStackFilterHandle);
      REQUIRE(nullptr != importImageStackFilter);

      Arguments args;

      args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
      args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
      args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
      args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_XGeneratedImageGeomPath));
      args.insertOrAssign(ITKImportImageStack::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutXAxis));

      auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = importImageStackFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

    // Run generated Y flip
    {
      auto importImageStackFilter = filterList->createFilter(::k_ImportImageStackFilterHandle);
      REQUIRE(nullptr != importImageStackFilter);

      Arguments args;

      args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
      args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
      args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_FileListInfo));
      args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_YGeneratedImageGeomPath));
      args.insertOrAssign(ITKImportImageStack::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutYAxis));

      auto preflightResult = importImageStackFilter->preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = importImageStackFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
  }

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, k_FilePrefix, k_ImageFlipStackDir);

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);
}
