#include <catch2/catch.hpp>

#include "ImageProcessing/Filters/ReadImageFilter.hpp"
#include "ImageProcessing/Filters/ReadImageStackFilter.hpp"
#include "ImageProcessing/ImageProcessing_test_dirs.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <tiffio.h>

#include <algorithm>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace fs = std::filesystem;

namespace
{
// The read-image-stack test re-uses the ImageStack data from the ITKImageProcessing plugin source
// tree. See the note in WriteImageTest.cpp for why we reach across plugin boundaries here.
const std::string k_ImageStackDir = std::string(unit_test::k_SimplnxSourceDIr.view()) + "/src/Plugins/ITKImageProcessing/data/ImageStack";
const DataPath k_ImageGeomPath = {{"ImageGeometry"}};
const DataPath k_ImageDataPath = k_ImageGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath("ImageData");
// The image_flip_test_images directory lives inside the import_image_stack_test_v3 archive
// alongside the main exemplar file. The k_ImageFlipStackDir path below resolves to
// .../TestFiles/import_image_stack_test_v3/image_flip_test_images.
const std::string k_FlippedImageStackSubDirName = "image_flip_test_images";
const DataPath k_XGeneratedImageGeomPath = DataPath({"xGeneratedImageGeom"});
const DataPath k_YGeneratedImageGeomPath = DataPath({"yGeneratedImageGeom"});
const DataPath k_XFlipImageGeomPath = DataPath({"xFlipImageGeom"});
const DataPath k_YFlipImageGeomPath = DataPath({"yFlipImageGeom"});
const std::string k_ImageDataName = "ImageData";
const ChoicesParameter::ValueType k_NoImageTransform = 0;
const ChoicesParameter::ValueType k_FlipAboutXAxis = 1;
const ChoicesParameter::ValueType k_FlipAboutYAxis = 2;
const fs::path k_ImageFlipStackDir = fs::path(fmt::format("{}/import_image_stack_test_v3/{}", unit_test::k_TestFilesDir, k_FlippedImageStackSubDirName));

// Exemplar Array Paths
const DataPath k_XFlippedImageDataPath = k_XFlipImageGeomPath.createChildPath(Constants::k_Cell_Data).createChildPath(::k_ImageDataName);
const DataPath k_YFlippedImageDataPath = k_YFlipImageGeomPath.createChildPath(Constants::k_Cell_Data).createChildPath(::k_ImageDataName);

void ExecuteImportImageStackXY(DataStructure& dataStructure, const std::string& filePrefix)
{
  UnitTest::LoadPlugins();

  // Define Shared parameters
  const std::vector<float32> origin = {0.0f, 0.0f, 0.0f};
  const std::vector<float32> spacing = {1.0f, 1.0f, 1.0f};
  GeneratedFileListParameter::ValueType fileListInfo;

  // Set File list for reads
  {
    fileListInfo.inputPath = k_ImageFlipStackDir.string();
    fileListInfo.startIndex = 1;
    fileListInfo.endIndex = 1;
    fileListInfo.incrementIndex = 1;
    fileListInfo.fileExtension = ".tiff";
    fileListInfo.filePrefix = filePrefix;
    fileListInfo.fileSuffix = "";
    fileListInfo.paddingDigits = 1;
    fileListInfo.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
  }

  // Run generated X flip
  {
    ReadImageStackFilter filter;
    Arguments args;

    args.insertOrAssign(ReadImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
    args.insertOrAssign(ReadImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
    args.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
    args.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_XGeneratedImageGeomPath));
    args.insertOrAssign(ReadImageStackFilter::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutXAxis));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Run generated Y flip
  {
    ReadImageStackFilter filter;
    Arguments args;

    args.insertOrAssign(ReadImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
    args.insertOrAssign(ReadImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
    args.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
    args.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(::k_YGeneratedImageGeomPath));
    args.insertOrAssign(ReadImageStackFilter::k_ImageTransformChoice_Key, std::make_any<ChoicesParameter::ValueType>(::k_FlipAboutYAxis));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
}

void ReadInFlippedXYExemplars(DataStructure& dataStructure, const std::string& filePrefix)
{
  {
    ReadImageFilter filter;
    Arguments args;

    fs::path filePath = k_ImageFlipStackDir / (filePrefix + "flip_x.tiff");
    args.insertOrAssign(ReadImageFilter::k_FileName_Key, filePath);
    args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, ::k_XFlipImageGeomPath);
    args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(::k_ImageDataName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  {
    ReadImageFilter filter;
    Arguments args;

    fs::path filePath = k_ImageFlipStackDir / (filePrefix + "flip_y.tiff");
    args.insertOrAssign(ReadImageFilter::k_FileName_Key, filePath);
    args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, ::k_YFlipImageGeomPath);
    args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(::k_ImageDataName));

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
const std::string k_TestDataDirName = "import_image_stack_test_v3";
const fs::path k_TestDataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_TestDataDirName;
const fs::path k_InputImagesDir = k_TestDataDir / "input_images";
const fs::path k_ExemplarFile = k_TestDataDir / "import_image_stack_test_v3.dream3d";

// Standard test parameters
const std::string k_FilePrefix = "200x200_";
const std::string k_FileExtension = ".tif";

// Cropping boundaries (crop to the colored square: 50:150 in X/Y, all Z)
const SizeVec2 k_VoxelCropX = {50, 150};
const SizeVec2 k_VoxelCropY = {50, 150};
const SizeVec2 k_VoxelCropZ = {0, 1};

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
                                 ChoicesParameter::ValueType flipMode = k_NoFlip, bool convertToGrayscale = false, bool changeOrigin = false, const std::vector<float32>& origin = {0.0, 0.0, 0.0},
                                 bool changeSpacing = false, const std::vector<float32>& spacing = {1.0, 1.0, 1.0}, ChoicesParameter::ValueType originSpacingTiming = k_Postprocessed)
{
  ReadImageStackFilter filter;
  Arguments args;

  auto fileList = CreateStandardFileList();

  args.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, fileList);
  args.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, outputGeomPath);
  args.insertOrAssign(ReadImageStackFilter::k_CroppingOptions_Key, cropOptions);
  args.insertOrAssign(ReadImageStackFilter::k_ResampleImagesChoice_Key, resampleMode);
  args.insertOrAssign(ReadImageStackFilter::k_ImageTransformChoice_Key, flipMode);
  args.insertOrAssign(ReadImageStackFilter::k_ConvertToGrayScale_Key, convertToGrayscale);
  args.insertOrAssign(ReadImageStackFilter::k_ChangeOrigin_Key, changeOrigin);
  args.insertOrAssign(ReadImageStackFilter::k_Origin_Key, origin);
  args.insertOrAssign(ReadImageStackFilter::k_ChangeSpacing_Key, changeSpacing);
  args.insertOrAssign(ReadImageStackFilter::k_Spacing_Key, spacing);
  args.insertOrAssign(ReadImageStackFilter::k_OriginSpacingProcessing_Key, originSpacingTiming);

  if(resampleMode == k_ScalingFactor)
  {
    args.insertOrAssign(ReadImageStackFilter::k_Scaling_Key, scalingFactor);
  }
  else if(resampleMode == k_ExactDimensions)
  {
    args.insertOrAssign(ReadImageStackFilter::k_ExactXYDimensions_Key, exactDims);
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

TEST_CASE("ImageProcessing::ReadImageStackFilter: NoInput", "[ImageProcessing][ReadImageStackFilter]")
{
  ReadImageStackFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageStackFilter: NoImageGeometry", "[ImageProcessing][ReadImageStackFilter]")
{
  ReadImageStackFilter filter;
  DataStructure dataStructure;
  Arguments args;

  GeneratedFileListParameter::ValueType fileListInfo;

  fileListInfo.inputPath = k_ImageStackDir;

  args.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageStackFilter: NoFiles", "[ImageProcessing][ReadImageStackFilter]")
{
  ReadImageStackFilter filter;
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

  args.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ReadImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ReadImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageStackFilter: FileDoesNotExist", "[ImageProcessing][ReadImageStackFilter]")
{
  ReadImageStackFilter filter;
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

  args.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ReadImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ReadImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageStackFilter: CompareImage", "[ImageProcessing][ReadImageStackFilter]")
{
  UnitTest::LoadPlugins();

  ReadImageStackFilter filter;
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

  args.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
  args.insertOrAssign(ReadImageStackFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ReadImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
  args.insertOrAssign(ReadImageStackFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ReadImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
  args.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageStackFilter: Flipped Image Even-Even X/Y", "[ImageProcessing][ReadImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  const std::string filePrefix = "image_flip_even_even_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ReadImageStackFilter
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

TEST_CASE("ImageProcessing::ReadImageStackFilter: Flipped Image Even-Odd X/Y", "[ImageProcessing][ReadImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  const std::string filePrefix = "image_flip_even_odd_";

  DataStructure dataStructure;

  // Generate XY Image Geometries with ReadImageStackFilter
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

TEST_CASE("ImageProcessing::ReadImageStackFilter: Flipped Image Odd-Even X/Y", "[ImageProcessing][ReadImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  const std::string filePrefix = "image_flip_odd_even_";

  DataStructure dataStructure;

  ::ExecuteImportImageStackXY(dataStructure, filePrefix);
  ::ReadInFlippedXYExemplars(dataStructure, filePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/odd_even_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  ::CompareXYFlippedGeometries(dataStructure);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageStackFilter: Flipped Image Odd-Odd X/Y", "[ImageProcessing][ReadImageStackFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  const std::string filePrefix = "image_flip_odd_odd_";

  DataStructure dataStructure;

  ::ExecuteImportImageStackXY(dataStructure, filePrefix);
  ::ReadInFlippedXYExemplars(dataStructure, filePrefix);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/odd_odd_import_image_stack_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  ::CompareXYFlippedGeometries(dataStructure);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageStackFilter::Baseline_NoProcessing", "[ImageProcessing][ReadImageStackFilter][Baseline]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Crop_Voxel_XOnly", "[ImageProcessing][ReadImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_X"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 200, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Crop_Voxel_YOnly", "[ImageProcessing][ReadImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_Y"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, false, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 101, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Crop_Voxel_ZOnly", "[ImageProcessing][ReadImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_Z"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, false, false, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 2);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Crop_Voxel_XY", "[ImageProcessing][ReadImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_XY"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Crop_Voxel_XYZ", "[ImageProcessing][ReadImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Voxel_XYZ"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 2);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Crop_Physical_XY", "[ImageProcessing][ReadImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Physical_XY"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Crop_Physical_Z", "[ImageProcessing][ReadImageStackFilter][Cropping]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Physical_Z"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume, false, false, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 2);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Resample_ScalingFactor", "[ImageProcessing][ReadImageStackFilter][Resampling]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Resample_Scaling50"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_ScalingFactor, 50.0f);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 100, 100, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Resample_ExactDimensions", "[ImageProcessing][ReadImageStackFilter][Resampling]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Resample_Exact128x128"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_ExactDimensions, 100.0f, {128, 128});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 128, 128, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Grayscale_Conversion", "[ImageProcessing][ReadImageStackFilter][Grayscale]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Grayscale"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_NoResample, 100.0f, {200, 200}, k_NoFlip, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 3);

  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<IDataArray>(grayscalePath));

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::FlipY", "[ImageProcessing][ReadImageStackFilter][Flip]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"FlipY_Test"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_NoResample, 100.0f, {200, 200}, k_FlipY);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::OriginSpacing_Preprocessed", "[ImageProcessing][ReadImageStackFilter][OriginSpacing]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"OriginSpacing_Preprocessed"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, false, true, {10.0, 20.0, 30.0}, true, {2.0, 2.0, 2.0}, k_Preprocessed);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);
  VerifyOriginSpacing(ds, geomPath, {110.0f, 120.0f, 30.0f}, {2.0f, 2.0f, 2.0f});

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::OriginSpacing_Postprocessed", "[ImageProcessing][ReadImageStackFilter][OriginSpacing]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"OriginSpacing_Postprocessed"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, false, true, {10.0, 20.0, 30.0}, true, {2.0, 2.0, 2.0}, k_Postprocessed);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);
  VerifyOriginSpacing(ds, geomPath, {10.0f, 20.0f, 30.0f}, {2.0f, 2.0f, 2.0f});

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::OriginSpacing_Preprocessed_WithZCrop", "[ImageProcessing][ReadImageStackFilter][OriginSpacing]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"OriginSpacing_Preprocessed_WithZCrop"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, false, false, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, false, true, {10.0, 20.0, 30.0}, true, {2.0, 2.0, 2.0}, k_Preprocessed);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 2);
  VerifyOriginSpacing(ds, geomPath, {10.0f, 20.0f, 30.0f}, {2.0f, 2.0f, 2.0f});

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::OriginSpacing_Postprocessed_WithZCrop", "[ImageProcessing][ReadImageStackFilter][OriginSpacing]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"OriginSpacing_Postprocessed_WithZCrop"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, false, false, true);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, false, true, {10.0, 20.0, 30.0}, true, {2.0, 2.0, 2.0}, k_Postprocessed);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 2);
  VerifyOriginSpacing(ds, geomPath, {10.0f, 20.0f, 30.0f}, {2.0f, 2.0f, 2.0f});

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Interaction_Crop_Resample", "[ImageProcessing][ReadImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Then_Resample"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_ExactDimensions, 100, {64, 64});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 64, 64, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Interaction_Crop_Flip", "[ImageProcessing][ReadImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Then_FlipX"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_FlipX);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Interaction_Resample_Flip", "[ImageProcessing][ReadImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Resample_Then_FlipX"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_ExactDimensions, 100.0f, {128, 128}, k_FlipX);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 128, 128, 3);

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Interaction_Crop_Grayscale", "[ImageProcessing][ReadImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Crop_Then_Grayscale"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_NoResample, 100.0f, {200, 200}, k_NoFlip, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 101, 101, 3);

  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<IDataArray>(grayscalePath));

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Interaction_Resample_Grayscale", "[ImageProcessing][ReadImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Resample_Then_Grayscale"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_ExactDimensions, 100.0f, {128, 128}, k_NoFlip, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 128, 128, 3);

  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<IDataArray>(grayscalePath));

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Interaction_Grayscale_Flip", "[ImageProcessing][ReadImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Grayscale_Then_FlipX"});
  auto noCrop = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::NoCropping, false, false, false);

  auto result = ExecuteImportImageStack(ds, geomPath, noCrop, k_NoResample, 100.0f, {200, 200}, k_FlipX, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 200, 200, 3);

  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<IDataArray>(grayscalePath));

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

TEST_CASE("ImageProcessing::ReadImageStackFilter::Interaction_FullPipeline", "[ImageProcessing][ReadImageStackFilter][Interaction]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "import_image_stack_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  DataStructure ds;

  const DataPath geomPath({"Full_Pipeline_Calculated"});
  auto cropOptions = CreateCropOptions(CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume, true, true, false);

  auto result = ExecuteImportImageStack(ds, geomPath, cropOptions, k_ScalingFactor, 50.0f, {100, 100}, k_FlipX, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  VerifyGeometryDimensions(ds, geomPath, 50, 50, 3);

  DataPath grayscalePath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  REQUIRE_NOTHROW(ds.getDataRefAs<IDataArray>(grayscalePath));

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

// =============================================================================
// MULTI-PAGE INPUT TESTS
// =============================================================================

namespace
{
// Directory for the synthesized multi-page TIFF inputs (created at runtime, no committed binaries).
fs::path MultiPageStackOutputDir()
{
  fs::path dir = fs::path(std::string(unit_test::k_BinaryTestOutputDir.view())) / "ReadImageStackMultiPage";
  fs::create_directories(dir);
  return dir;
}

// Writes an N-page uint8 (single-component) TIFF. pagePixels[p] is row-major (top-to-bottom),
// size == width*height. Mirrors the libtiff synthesis idiom used in ReadImageTest.cpp/ImageIOTest.cpp.
void WriteMultiPageTiff(const fs::path& path, uint32_t width, uint32_t height, const std::vector<std::vector<uint8_t>>& pagePixels)
{
  TIFF* tif = TIFFOpen(path.string().c_str(), "w");
  REQUIRE(tif != nullptr);
  for(const auto& pixels : pagePixels)
  {
    REQUIRE(pixels.size() == static_cast<size_t>(width) * height);
    REQUIRE(TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, static_cast<uint16_t>(1)) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0)) != 0);
    for(uint32_t row = 0; row < height; ++row)
    {
      // TIFFWriteScanline takes a non-const void* but does not modify the buffer.
      REQUIRE(TIFFWriteScanline(tif, const_cast<uint8_t*>(pixels.data() + static_cast<size_t>(row) * width), row, 0) >= 0);
    }
    REQUIRE(TIFFWriteDirectory(tif) != 0);
  }
  TIFFClose(tif);
}
} // namespace

// Stacks a list of multi-page TIFFs. Legacy ITKImportImageStack behavior (and the expected NX behavior)
// is: Z == number of FILES, each slice is the FIRST page of the corresponding file, and pages past the
// first are dropped with a diagnostic warning. This guards against the naive "read every page" behavior
// that would make Z == the sum of all pages and waste IO/memory on large multi-page stacks.
TEST_CASE("ImageProcessing::ReadImageStackFilter::MultiPage_FirstPagePerFile", "[ImageProcessing][ReadImageStackFilter]")
{
  UnitTest::LoadPlugins();

  constexpr uint32_t width = 5;
  constexpr uint32_t height = 4;
  constexpr usize numFiles = 3;
  constexpr usize pagesPerFile = 3;
  constexpr usize sliceElems = static_cast<usize>(width) * height; // 20

  // pixel(file f, page p, index i) = f*10 + p*100 + i (all < 256). First page (p == 0) of file f is
  // therefore f*10 + i, and every later page differs, so reading the wrong page fails the comparison.
  const fs::path inputDir = MultiPageStackOutputDir();
  for(usize f = 0; f < numFiles; ++f)
  {
    std::vector<std::vector<uint8_t>> pagePixels(pagesPerFile, std::vector<uint8_t>(sliceElems));
    for(usize p = 0; p < pagesPerFile; ++p)
    {
      for(usize i = 0; i < sliceElems; ++i)
      {
        pagePixels[p][i] = static_cast<uint8_t>(f * 10 + p * 100 + i);
      }
    }
    WriteMultiPageTiff(inputDir / fmt::format("multipage_{}.tif", f), width, height, pagePixels);
  }

  GeneratedFileListParameter::ValueType fileList;
  fileList.inputPath = inputDir.string();
  fileList.filePrefix = "multipage_";
  fileList.fileSuffix = "";
  fileList.fileExtension = ".tif";
  fileList.startIndex = 0;
  fileList.endIndex = numFiles - 1;
  fileList.incrementIndex = 1;
  fileList.paddingDigits = 1;
  fileList.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

  const DataPath geomPath({"MultiPage_Stack"});

  ReadImageStackFilter filter;
  DataStructure ds;
  Arguments args;
  args.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileList));
  args.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(geomPath));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // (a) Z equals the number of FILES, not the summed page count (which would be numFiles*pagesPerFile).
  VerifyGeometryDimensions(ds, geomPath, width, height, numFiles);

  // (b) Each slice equals the FIRST page of the corresponding file.
  const DataPath dataPath = geomPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_ImageDataName);
  const auto& store = ds.getDataRefAs<UInt8Array>(dataPath).getDataStoreRef();
  REQUIRE(store.getNumberOfTuples() == sliceElems * numFiles);
  for(usize f = 0; f < numFiles; ++f)
  {
    for(usize i = 0; i < sliceElems; ++i)
    {
      REQUIRE(store.getValue(f * sliceElems + i) == static_cast<uint8_t>(f * 10 + i));
    }
  }

  // (c) The execute result carries the multi-page diagnostic warning (one per multi-page file).
  const auto& warnings = executeResult.result.warnings();
  const bool hasMultiPageWarning = std::any_of(warnings.cbegin(), warnings.cend(), [](const Warning& w) { return w.message.find("reading only the first page") != std::string::npos; });
  REQUIRE(hasMultiPageWarning);
}
