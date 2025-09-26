#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageReaderFilter.hpp"
#include "ITKImageProcessing/Filters/ITKImportImageStackFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

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

struct AxisBoundsChoices
{
  std::vector<IntVec2Type> voxelX{{IntVec2Type{50, 150}}};
  std::vector<IntVec2Type> voxelY{{IntVec2Type{50, 150}}};
  std::vector<IntVec2Type> voxelZ{{IntVec2Type{0, 1}}};

  std::vector<FloatVec2Type> physX{{FloatVec2Type{100.0f, 300.0f}}};
  std::vector<FloatVec2Type> physY{{FloatVec2Type{100.0f, 300.0f}}};
  std::vector<FloatVec2Type> physZ{{FloatVec2Type{10.0f, 30.0f}}};
};

std::vector<CropGeometryParameter::ValueType> GenerateAllCropValues(const AxisBoundsChoices& C)
{
  std::vector<CropGeometryParameter::ValueType> out;

  // NoCropping
  {
    CropGeometryParameter::ValueType cv;
    cv.type = CropGeometryParameter::CropValues::TypeEnum::NoCropping;
    cv.cropX = false;
    cv.cropY = false;
    cv.cropZ = false;
    out.push_back(cv);
  }

  const std::array<std::tuple<bool, bool, bool>, 7> kFlagOrder = {std::tuple{false, false, true}, std::tuple{false, true, false}, std::tuple{false, true, true}, std::tuple{true, false, false},
                                                                  std::tuple{true, false, true},  std::tuple{true, true, false},  std::tuple{true, true, true}};

  for(const auto& [cx, cy, cz] : kFlagOrder)
  {
    if(!(cx || cy || cz))
    {
      return {}; // require at least one axis cropped
    }

    std::vector<std::optional<IntVec2Type>> xOpts = cx ? std::vector<std::optional<IntVec2Type>>(C.voxelX.begin(), C.voxelX.end()) : std::vector<std::optional<IntVec2Type>>{std::nullopt};
    std::vector<std::optional<IntVec2Type>> yOpts = cy ? std::vector<std::optional<IntVec2Type>>(C.voxelY.begin(), C.voxelY.end()) : std::vector<std::optional<IntVec2Type>>{std::nullopt};
    std::vector<std::optional<IntVec2Type>> zOpts = cz ? std::vector<std::optional<IntVec2Type>>(C.voxelZ.begin(), C.voxelZ.end()) : std::vector<std::optional<IntVec2Type>>{std::nullopt};

    for(const auto& xb : xOpts)
    {
      for(const auto& yb : yOpts)
      {
        for(const auto& zb : zOpts)
        {
          CropGeometryParameter::ValueType cv;
          cv.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
          cv.cropX = cx;
          cv.cropY = cy;
          cv.cropZ = cz;
          if(xb)
          {
            cv.xBoundVoxels = *xb;
          }
          if(yb)
          {
            cv.yBoundVoxels = *yb;
          }
          if(zb)
          {
            cv.zBoundVoxels = *zb;
          }
          out.push_back(cv);
        }
      }
    }
  }
  for(const auto& [cx, cy, cz] : kFlagOrder)
  {
    if(!(cx || cy || cz))
    {
      return {};
    }

    std::vector<std::optional<FloatVec2Type>> xOpts = cx ? std::vector<std::optional<FloatVec2Type>>(C.physX.begin(), C.physX.end()) : std::vector<std::optional<FloatVec2Type>>{std::nullopt};
    std::vector<std::optional<FloatVec2Type>> yOpts = cy ? std::vector<std::optional<FloatVec2Type>>(C.physY.begin(), C.physY.end()) : std::vector<std::optional<FloatVec2Type>>{std::nullopt};
    std::vector<std::optional<FloatVec2Type>> zOpts = cz ? std::vector<std::optional<FloatVec2Type>>(C.physZ.begin(), C.physZ.end()) : std::vector<std::optional<FloatVec2Type>>{std::nullopt};

    for(const auto& xb : xOpts)
    {
      for(const auto& yb : yOpts)
      {
        for(const auto& zb : zOpts)
        {
          CropGeometryParameter::ValueType cv;
          cv.type = CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
          cv.cropX = cx;
          cv.cropY = cy;
          cv.cropZ = cz;
          if(xb)
          {
            cv.xBoundPhysical = *xb;
          }
          if(yb)
          {
            cv.yBoundPhysical = *yb;
          }
          if(zb)
          {
            cv.zBoundPhysical = *zb;
          }
          out.push_back(cv);
        }
      }
    }
  }

  return out;
}

std::string BoolToString(bool v)
{
  return v ? "True" : "False";
}

std::string CropTypeToString(CropGeometryParameter::CropValues::TypeEnum t)
{
  using T = CropGeometryParameter::CropValues::TypeEnum;
  switch(t)
  {
  case T::NoCropping:
    return "NoCropping";
  case T::VoxelSubvolume:
    return "VoxelSubvolume";
  case T::PhysicalSubvolume:
    return "PhysicalSubvolume";
  }
  return "Unknown";
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
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(3));
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
  args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(3));
  args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(3));
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImportImageStackFilter: All Combinations", "[ITKImageProcessing][ITKImportImageStackFilter]")
{
  UnitTest::LoadPlugins();

  //  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ITKMhaFileReaderTest_v3.tar.gz",
  //  "ITKMhaFileReaderTest_v3"); const fs::path exemplaryFilePath = fs::path(unit_test::k_TestFilesDir.view()) / "ITKMhaFileReaderTest_v3/ExemplarySmallIN100.dream3d";
  const fs::path exemplaryFilePath = "/Users/bluequartz/Downloads/image_stack_exemplary.dream3d";

  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplaryFilePath);

  GeneratedFileListParameter::ValueType fileList;
  fileList.filePrefix = "input_image_";
  fileList.fileSuffix = "";
  fileList.fileExtension = ".tif";
  fileList.inputPath = "/Users/bluequartz/Downloads/input_images";
  fileList.incrementIndex = 1;
  fileList.startIndex = 0;
  fileList.endIndex = 2;
  fileList.paddingDigits = 1;

  const AxisBoundsChoices bounds{};
  auto allCropVals = GenerateAllCropValues(bounds);

  bool convertToGrayScale = GENERATE(false, true);
  ChoicesParameter::ValueType resampleIdx = GENERATE(0, 1, 2);
  ChoicesParameter::ValueType imageTransformIdx = GENERATE(0, 1, 2);
  auto croppingOptions = GENERATE_COPY(from_range(allCropVals));
  float32 scalingChoice = GENERATE(10.0);
  VectorUInt64Parameter::ValueType exactXYDimensionsChoice = GENERATE(VectorUInt64Parameter::ValueType{64, 64});

  static std::atomic<int> geomCounter{1};
  const int myId = geomCounter.fetch_add(1);

  const std::string exemplaryGeomName =
      fmt::format("ImageGeometry {:0>3} (CroppingOptions=[{}, {}, {}, {}] Resample={} ImageFlip={} Grayscale={})", myId, CropTypeToString(croppingOptions.type), BoolToString(croppingOptions.cropX),
                  BoolToString(croppingOptions.cropY), BoolToString(croppingOptions.cropZ), resampleIdx, imageTransformIdx, BoolToString(convertToGrayScale));
  const std::string computedGeomName = "ImageGeometry";

  DYNAMIC_SECTION(exemplaryGeomName)
  {
    ITKImportImageStackFilter filter;
    Arguments args;

    args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, fileList);
    args.insertOrAssign(ITKImportImageStackFilter::k_CroppingOptions_Key, croppingOptions);
    args.insertOrAssign(ITKImportImageStackFilter::k_ResampleImagesChoice_Key, resampleIdx);
    args.insertOrAssign(ITKImportImageStackFilter::k_ImageTransformChoice_Key, imageTransformIdx);
    args.insertOrAssign(ITKImportImageStackFilter::k_ConvertToGrayScale_Key, convertToGrayScale);
    args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::vector<float64>{2.0f, 2.0f, 20.0f});
    args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, DataPath({computedGeomName}));

    if(resampleIdx == 1)
    {
      args.insertOrAssign(ITKImportImageStackFilter::k_Scaling_Key, scalingChoice);
    }
    else if(resampleIdx == 2)
    {
      args.insertOrAssign(ITKImportImageStackFilter::k_ExactXYDimensions_Key, exactXYDimensionsChoice);
    }

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    if(executeResult.result.invalid())
    {
      INFO([&] {
        std::ostringstream oss;
        oss << "Errors for combo '" << exemplaryGeomName << "':\n";
        for(const auto& e : executeResult.result.errors())
          oss << "  - " << e.code << " : " << e.message << "\n";
        return oss.str();
      }());
    }
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    UnitTest::CompareImageGeometry(dataStructure, DataPath({exemplaryGeomName}), DataPath({computedGeomName}));

    auto exemplaryAttrMatrixPath = DataPath({exemplaryGeomName}).createChildPath(Constants::k_Cell_Data);
    auto computedAttrMatrixPath = DataPath({computedGeomName}).createChildPath(Constants::k_Cell_Data);
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplaryAttrMatrixPath, dataStructure, computedAttrMatrixPath, true);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
