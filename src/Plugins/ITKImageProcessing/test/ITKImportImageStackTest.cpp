#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageReaderFilter.hpp"
#include "ITKImageProcessing/Filters/ITKImportImageStackFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace fs = std::filesystem;

namespace
{
const std::string k_ImageStackDir = unit_test::k_DataDir.str() + "/ImageStack";
const DataPath k_ImageGeomPath = {{"ImageGeometry"}};
const DataPath k_ImageDataPath = k_ImageGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath("ImageData");
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
  UnitTest::LoadPlugins();
  auto* filterListPtr = nx::core::Application::Instance()->getFilterList();
  REQUIRE(filterListPtr != nullptr);

  // Define Shared parameters
  std::vector<float64> k_Origin = {0.0f, 0.0f, 0.0f};
  std::vector<float64> k_Spacing = {1.0f, 1.0f, 1.0f};
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

    args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float64>>(k_Origin));
    args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));
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

    args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float64>>(k_Origin));
    args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));
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

// Test data paths
const std::string k_TestDataDirName = "import_image_stack_test";
const fs::path k_TestDataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_TestDataDirName;
const fs::path k_InputImagesDir = k_TestDataDir / "input_images";
const fs::path k_ExemplarFile = k_TestDataDir / "import_image_stack_test.dream3d";

// Standard test parameters
const std::string k_FilePrefix = "200x200_";
const std::string k_FileExtension = ".tif";

// Cropping boundaries (crop to the colored square: 50:150 in X/Y, all Z)
const IntVec2Type k_VoxelCropX = {50, 150};
const IntVec2Type k_VoxelCropY = {50, 150};
const IntVec2Type k_VoxelCropZ = {0, 1};

const FloatVec2Type k_PhysicalCropX = {50.0f, 150.0f};
const FloatVec2Type k_PhysicalCropY = {50.0f, 150.0f};
const FloatVec2Type k_PhysicalCropZ = {0.0f, 1.0f};

// Resampling/flip/timing constants
const ChoicesParameter::ValueType k_NoResample = 0;
const ChoicesParameter::ValueType k_ScalingFactor = 1;
const ChoicesParameter::ValueType k_ExactDimensions = 2;
const ChoicesParameter::ValueType k_NoFlip = 0;
const ChoicesParameter::ValueType k_FlipX = 1;
const ChoicesParameter::ValueType k_FlipY = 2;
const ChoicesParameter::ValueType k_Preprocessed = 0;
const ChoicesParameter::ValueType k_Postprocessed = 1;

/**
 * @brief Helper to create standard file list for test images
 */
GeneratedFileListParameter::ValueType CreateStandardFileList()
{
  GeneratedFileListParameter::ValueType fileList;
  fileList.inputPath = k_InputImagesDir.string();
  fileList.filePrefix = k_FilePrefix;
  fileList.fileSuffix = "";
  fileList.fileExtension = k_FileExtension;
  fileList.startIndex = 0;
  fileList.endIndex = 2;
  fileList.incrementIndex = 1;
  fileList.paddingDigits = 1;
  fileList.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
  return fileList;
}

/**
 * @brief Helper to create cropping options
 */
CropGeometryParameter::ValueType CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum type, bool cropX, bool cropY, bool cropZ)
{
  CropGeometryParameter::ValueType crop;
  crop.type = type;
  crop.cropX = cropX;
  crop.cropY = cropY;
  crop.cropZ = cropZ;

  if(type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
  {
    crop.xBoundVoxels = k_VoxelCropX;
    crop.yBoundVoxels = k_VoxelCropY;
    crop.zBoundVoxels = k_VoxelCropZ;
  }
  else if(type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume)
  {
    crop.xBoundPhysical = k_PhysicalCropX;
    crop.yBoundPhysical = k_PhysicalCropY;
    crop.zBoundPhysical = k_PhysicalCropZ;
  }

  return crop;
}

/**
 * @brief Execute filter with standard parameters + custom overrides
 */
Result<> ExecuteImportImageStack(DataStructure& dataStructure, const DataPath& outputGeomPath, const CropGeometryParameter::ValueType& cropOptions = {},
                                 ChoicesParameter::ValueType resampleMode = k_NoResample, float32 scalingFactor = 100.0f, const VectorUInt64Parameter::ValueType& exactDims = {200, 200},
                                 ChoicesParameter::ValueType flipMode = k_NoFlip, bool convertToGrayscale = false, bool changeOrigin = false, const std::vector<float64>& origin = {0.0, 0.0, 0.0},
                                 bool changeSpacing = false, const std::vector<float64>& spacing = {1.0, 1.0, 1.0}, ChoicesParameter::ValueType originSpacingTiming = k_Postprocessed)
{
  ITKImportImageStackFilter filter;
  Arguments args;

  auto fileList = CreateStandardFileList();

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, fileList);
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, outputGeomPath);
  args.insertOrAssign(ITKImportImageStackFilter::k_CroppingOptions_Key, cropOptions);
  args.insertOrAssign(ITKImportImageStackFilter::k_ResampleImagesChoice_Key, resampleMode);
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageTransformChoice_Key, flipMode);
  args.insertOrAssign(ITKImportImageStackFilter::k_ConvertToGrayScale_Key, convertToGrayscale);
  args.insertOrAssign(ITKImportImageStackFilter::k_ChangeOrigin_Key, changeOrigin);
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, origin);
  args.insertOrAssign(ITKImportImageStackFilter::k_ChangeSpacing_Key, changeSpacing);
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, spacing);
  args.insertOrAssign(ITKImportImageStackFilter::k_OriginSpacingProcessing_Key, originSpacingTiming);

  if(resampleMode == k_ScalingFactor)
  {
    args.insertOrAssign(ITKImportImageStackFilter::k_Scaling_Key, scalingFactor);
  }
  else if(resampleMode == k_ExactDimensions)
  {
    args.insertOrAssign(ITKImportImageStackFilter::k_ExactXYDimensions_Key, exactDims);
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  if(preflightResult.outputActions.invalid())
  {
    return ConvertResult(std::move(preflightResult.outputActions));
  }

  auto executeResult = filter.execute(dataStructure, args);
  return executeResult.result;
}

/**
 * @brief Verify expected geometry dimensions
 */
void VerifyGeometryDimensions(const DataStructure& ds, const DataPath& geomPath, usize expectedX, usize expectedY, usize expectedZ)
{
  const auto* geom = ds.getDataAs<ImageGeom>(geomPath);
  REQUIRE(geom != nullptr);

  SizeVec3 dims = geom->getDimensions();
  REQUIRE(dims[0] == expectedX);
  REQUIRE(dims[1] == expectedY);
  REQUIRE(dims[2] == expectedZ);
}

/**
 * @brief Verify origin and spacing
 */
void VerifyOriginSpacing(const DataStructure& ds, const DataPath& geomPath, const FloatVec3& expectedOrigin, const FloatVec3& expectedSpacing)
{
  const auto* geom = ds.getDataAs<ImageGeom>(geomPath);
  REQUIRE(geom != nullptr);

  FloatVec3 origin = geom->getOrigin();
  FloatVec3 spacing = geom->getSpacing();

  REQUIRE(origin[0] == Approx(expectedOrigin[0]));
  REQUIRE(origin[1] == Approx(expectedOrigin[1]));
  REQUIRE(origin[2] == Approx(expectedOrigin[2]));

  REQUIRE(spacing[0] == Approx(expectedSpacing[0]));
  REQUIRE(spacing[1] == Approx(expectedSpacing[1]));
  REQUIRE(spacing[2] == Approx(expectedSpacing[2]));
}

} // namespace

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: NoInput", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  ITKImportImageStackFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float64>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float64>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: CompareImage", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  UnitTest::LoadPlugins();

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

  std::vector<float64> origin = {1.0f, 4.0f, 8.0f};
  std::vector<float64> spacing = {0.3f, 0.2f, 0.9f};

  args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ITKImportImageStackFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float64>>(origin));
  args.insertOrAssign(ITKImportImageStackFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(spacing));
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

  // md5 hash only works on in-memory DataStore<T>
  // if(ITKTestBase::IsArrayInMemory(dataStructure, k_ImageDataPath))
  {
    const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, k_ImageDataPath);
    REQUIRE(md5Hash == "2620b39f0dcaa866602c2591353116a4");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Flipped Image Even-Even X/Y", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  const std::string filePrefix = "image_flip_even_even_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ITKImportImageStackFilter
  ::ExecuteImportImageStackXY(dataStructure, filePrefix);

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, filePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/even_even_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Flipped Image Even-Odd X/Y", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  const std::string filePrefix = "image_flip_even_odd_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ITKImportImageStackFilter
  ::ExecuteImportImageStackXY(dataStructure, filePrefix);

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, filePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/even_odd_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Flipped Image Odd-Even X/Y", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  const std::string filePrefix = "image_flip_odd_even_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ITKImportImageStackFilter
  ::ExecuteImportImageStackXY(dataStructure, filePrefix);

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, filePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/odd_even_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: Flipped Image Odd-Odd X/Y", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "image_flip_test_images.tar.gz", k_FlippedImageStackDirName);

  const std::string filePrefix = "image_flip_odd_odd_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ITKImportImageStackFilter
  ::ExecuteImportImageStackXY(dataStructure, filePrefix);

  // Read in exemplars
  ::ReadInFlippedXYExemplars(dataStructure, filePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/odd_odd_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare against exemplars
  ::CompareXYFlippedGeometries(dataStructure);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImportImageStack::Baseline_NoProcessing", "[ITKImageProcessing][ITKImportImageStackFilter][Baseline]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Baseline_Geometry"});
  auto result = ExecuteImportImageStack(ds, geomPath);

  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 3);
  VerifyOriginSpacing(ds, geomPath, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Baseline_Geometry"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Baseline_Geometry", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

// =============================================================================
// CROPPING TESTS
// =============================================================================

TEST_CASE("ITKImportImageStack::Crop_Voxel_XOnly", "[ITKImageProcessing][ITKImportImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_X"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Cropped X: 50-150 = 101 voxels, Y and Z unchanged
  VerifyGeometryDimensions(ds, geomPath, 101, 200, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_Voxel_X"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_Voxel_X", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Crop_Voxel_YOnly", "[ITKImageProcessing][ITKImportImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_Y"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, false, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Cropped Y: 50-150 = 101 voxels, X and Z unchanged
  VerifyGeometryDimensions(ds, geomPath, 200, 101, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_Voxel_Y"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_Voxel_Y", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Crop_Voxel_ZOnly", "[ITKImageProcessing][ITKImportImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_Z"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, false, false, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Cropped Z: 0-1 inclusive = 2 slices
  VerifyGeometryDimensions(ds, geomPath, 200, 200, 2);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_Voxel_Z"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_Voxel_Z", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Crop_Voxel_XY", "[ITKImageProcessing][ITKImportImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_XY"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Cropped to colored square: 101x101x3
  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_Voxel_XY"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_Voxel_XY", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Crop_Voxel_XYZ", "[ITKImageProcessing][ITKImportImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_XYZ"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Full crop: 101x101x2 (X: 50-150, Y: 50-150, Z: 0-1)
  VerifyGeometryDimensions(ds, geomPath, 101, 101, 2);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_Voxel_XYZ"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_Voxel_XYZ", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Crop_Physical_XY", "[ITKImageProcessing][ITKImportImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Physical_XY"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Physical crop with default origin/spacing should match voxel crop
  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_Physical_XY"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_Physical_XY", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Crop_Physical_Z", "[ITKImageProcessing][ITKImportImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Physical_Z"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume, false, false, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Physical Z crop: 0.0-1.0 with default spacing 1.0 = 2 slices
  VerifyGeometryDimensions(ds, geomPath, 200, 200, 2);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_Physical_Z"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_Physical_Z", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

// =============================================================================
// RESAMPLING TESTS
// =============================================================================

TEST_CASE("ITKImportImageStack::Resample_ScalingFactor", "[ITKImageProcessing][ITKImportImageStackFilter][Resampling]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Resample_Scaling50"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  // 50% scaling -> 200x200 becomes 100x100
  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_ScalingFactor, 50.0f);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 100, 100, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Resample_Scaling_50"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Resample_Scaling_50", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Resample_ExactDimensions", "[ITKImageProcessing][ITKImportImageStackFilter][Resampling]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Resample_Exact128x128"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  // Exact dimensions: 128x128
  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_ExactDimensions, 100.0f, {128, 128});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 128, 128, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Resample_Exact_128x128"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Resample_Exact_128x128", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

// =============================================================================
// GRAYSCALE TESTS
// =============================================================================

TEST_CASE("ITKImportImageStack::Grayscale_Conversion", "[ITKImageProcessing][ITKImportImageStackFilter][Grayscale]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Grayscale"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_NoResample, 100.0f, {200, 200}, k_NoFlip, true); // convertToGrayscale=true
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Verify geometry created
  VerifyGeometryDimensions(ds, geomPath, 200, 200, 3);

  // Verify grayscale array was created
  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<AbstractDataArray>(grayscalePath));

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Grayscale_Conversion"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Grayscale_Conversion", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

// =============================================================================
// FLIP TESTS
// =============================================================================

TEST_CASE("ITKImportImageStack::FlipY", "[ITKImageProcessing][ITKImportImageStackFilter][Flip]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"FlipY_Test"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_NoResample, 100.0f, {200, 200}, k_FlipY);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"FlipY_Test"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"FlipY_Test", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

// =============================================================================
// ORIGIN/SPACING TESTS
// =============================================================================

TEST_CASE("ITKImportImageStack::OriginSpacing_Preprocessed", "[ITKImageProcessing][ITKImportImageStackFilter][OriginSpacing]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"OriginSpacing_Preprocessed"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, false, true, {10.0, 20.0, 30.0}, true, {2.0, 2.0, 2.0}, k_Preprocessed);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);
  // Preprocessed: origin/spacing applied before crop, so final origin = [10 + 50*2, 20 + 50*2, 30] = [110, 120, 30]
  VerifyOriginSpacing(ds, geomPath, {110.0f, 120.0f, 30.0f}, {2.0f, 2.0f, 2.0f});

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"OriginSpacing_Preprocessed"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"OriginSpacing_Preprocessed", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::OriginSpacing_Postprocessed", "[ITKImageProcessing][ITKImportImageStackFilter][OriginSpacing]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"OriginSpacing_Postprocessed"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, false, true, {10.0, 20.0, 30.0}, true, {2.0, 2.0, 2.0}, k_Postprocessed);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);
  VerifyOriginSpacing(ds, geomPath, {10.0f, 20.0f, 30.0f}, {2.0f, 2.0f, 2.0f});

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"OriginSpacing_Postprocessed"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"OriginSpacing_Postprocessed", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::OriginSpacing_Preprocessed_WithZCrop", "[ITKImageProcessing][ITKImportImageStackFilter][OriginSpacing]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"OriginSpacing_Preprocessed_WithZCrop"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, false, false, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, false, true, {10.0, 20.0, 30.0}, true, {2.0, 2.0, 2.0}, k_Preprocessed);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 2);
  VerifyOriginSpacing(ds, geomPath, {10.0f, 20.0f, 30.0f}, {2.0f, 2.0f, 2.0f});

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"OriginSpacing_Preprocessed_WithZCrop"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"OriginSpacing_Preprocessed_WithZCrop", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::OriginSpacing_Postprocessed_WithZCrop", "[ITKImageProcessing][ITKImportImageStackFilter][OriginSpacing]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"OriginSpacing_Postprocessed_WithZCrop"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, false, false, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, false, true, {10.0, 20.0, 30.0}, true, {2.0, 2.0, 2.0}, k_Postprocessed);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 2);
  VerifyOriginSpacing(ds, geomPath, {10.0f, 20.0f, 30.0f}, {2.0f, 2.0f, 2.0f});

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"OriginSpacing_Postprocessed_WithZCrop"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"OriginSpacing_Postprocessed_WithZCrop", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

// =============================================================================
// INTERACTION TESTS
// =============================================================================

TEST_CASE("ITKImportImageStack::Interaction_Crop_Resample", "[ITKImageProcessing][ITKImportImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Then_Resample"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  // Crop to 101x101, then resample to 64x64
  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_ExactDimensions, 100, {64, 64});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 64, 64, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_And_Resample"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_And_Resample", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Interaction_Crop_Flip", "[ITKImageProcessing][ITKImportImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Then_FlipX"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_FlipX);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Crop to 101x101x3 then flip along the X axis
  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_And_FlipX"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_And_FlipX", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Interaction_Resample_Flip", "[ITKImageProcessing][ITKImportImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Resample_Then_FlipX"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  // Resample to 128x128x3 then flip along the X axis
  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_ExactDimensions, 100.0f, {128, 128}, k_FlipX);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 128, 128, 3);

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Resample_And_FlipX"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Resample_And_FlipX", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Interaction_Crop_Grayscale", "[ITKImageProcessing][ITKImportImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Then_Grayscale"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);

  // Verify grayscale array exists
  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<AbstractDataArray>(grayscalePath));

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Crop_And_Grayscale"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Crop_And_Grayscale", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Interaction_Resample_Grayscale", "[ITKImageProcessing][ITKImportImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Resample_Then_Grayscale"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  // Resample to 128x128 then convert to grayscale
  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_ExactDimensions, 100.0f, {128, 128}, k_NoFlip, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 128, 128, 3);

  // Verify grayscale array exists
  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<AbstractDataArray>(grayscalePath));

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Resample_And_Grayscale"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Resample_And_Grayscale", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Interaction_Grayscale_Flip", "[ITKImageProcessing][ITKImportImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Grayscale_Then_FlipX"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_NoResample, 100.0f, {200, 200}, k_FlipX, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 3);

  // Verify grayscale array exists
  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<AbstractDataArray>(grayscalePath));

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Grayscale_And_FlipX"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Grayscale_And_FlipX", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}

TEST_CASE("ITKImportImageStack::Interaction_FullPipeline", "[ITKImageProcessing][ITKImportImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v2.tar.gz", k_TestDataDirName);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Full_Pipeline_Calculated"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  // Crop XY [50,150] -> Resample to 100x100 -> Scale 50% -> Grayscale -> Flip X
  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_ScalingFactor, 50.0f, {100, 100}, k_FlipX, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // 100 * 0.5 = 50
  VerifyGeometryDimensions(ds, geomPath, 50, 50, 3);

  // Verify grayscale array exists
  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<AbstractDataArray>(grayscalePath));

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  const auto* generatedGeom = ds.getDataAs<ImageGeom>(geomPath);
  const auto* exemplarGeom = exemplarDS.getDataAs<ImageGeom>(DataPath({"Full_Pipeline"}));
  UnitTest::CompareImageGeometry(exemplarGeom, generatedGeom);

  DataPath generatedDataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Full_Pipeline", Constants::k_Cell_Data, k_ImageDataName});
  const auto& generatedArray = ds.getDataRefAs<UInt8Array>(generatedDataPath);
  const auto& exemplarArray = exemplarDS.getDataRefAs<UInt8Array>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
}
