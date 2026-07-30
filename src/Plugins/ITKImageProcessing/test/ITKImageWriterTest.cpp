#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageWriterFilter.hpp"
#include "ITKImageProcessing/Filters/ITKImportImageStackFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include <itkImageFileReader.h>
#include <itkRGBAPixel.h>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const std::string k_ImageStackDir = unit_test::k_DataDir.str() + "/ImageStack";
const DataPath k_ImageGeomPath = {{"ImageGeometry"}};
const DataPath k_ImageDataPath = k_ImageGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath("ImageData");

/**
 * @return
 */
std::string CreateRandomDirName()
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(65, 90);

  std::string s(16, 'z');
  for(int i = 0; i < 16; ++i)
  {
    s[i] = static_cast<char>(distrib(gen));
  }

  return s;
}

void validateOutputFiles(size_t numImages, uint64 offset, const std::string& tempDirName, const std::string& tempDirPath)
{
  // Check for the existence of each image file, remove it as we go...
  for(size_t i = 0; i < numImages; i++)
  {
    fs::path imagePath = fs::path() / fmt::format("{}/{}/slice_{:03d}.tif", unit_test::k_BinaryTestOutputDir.view(), tempDirName, i + offset);
    INFO(fmt::format("Checking File: '{}'  ", imagePath.string()));
    REQUIRE(fs::exists(imagePath));
    REQUIRE(std::filesystem::remove(imagePath));
  }

  // Now make sure there are no files left in the directory.
  int count = 0;
  for(const auto& entry : std::filesystem::directory_iterator(tempDirPath))
  {
    count++;
  }
  REQUIRE(count == 0);

  // Now delete the temp directory
  try
  {
    std::filesystem::remove_all(tempDirPath);
    std::cout << "Directory removed successfully: " << tempDirPath << std::endl;
  } catch(std::filesystem::filesystem_error& e)
  {
    std::cout << "Error removing temp directory: " << tempDirPath << std::endl;
    std::cout << "    " << e.what() << std::endl;
  }
}

bool RequireExampleOutputFile(const IFilter::PreflightResult& preflightResult, const fs::path& expectedFilePath)
{
  return std::find_if(preflightResult.outputValues.begin(), preflightResult.outputValues.end(), [expectedFilePath](const IFilter::PreflightValue& value) {
           if(value.name == "Example Output File")
           {
             return fs::path(value.value).lexically_normal() == fs::absolute(expectedFilePath).lexically_normal();
           }
           return false;
         }) != preflightResult.outputValues.end();
}

template <typename PixelT>
void CompareImageToExpected(const fs::path& filePath, const std::array<usize, 2>& expectedDimensions, const std::vector<PixelT>& expectedPixels)
{
  using ImageType = itk::Image<PixelT, 2>;
  auto reader = itk::ImageFileReader<ImageType>::New();
  reader->SetFileName(filePath.string());
  REQUIRE_NOTHROW(reader->Update());

  const auto& image = *reader->GetOutput();
  const auto dimensions = image.GetLargestPossibleRegion().GetSize();
  REQUIRE(dimensions[0] == expectedDimensions[0]);
  REQUIRE(dimensions[1] == expectedDimensions[1]);

  for(usize y = 0; y < expectedDimensions[1]; ++y)
  {
    for(usize x = 0; x < expectedDimensions[0]; ++x)
    {
      typename ImageType::IndexType index;
      index[0] = static_cast<typename ImageType::IndexType::IndexValueType>(x);
      index[1] = static_cast<typename ImageType::IndexType::IndexValueType>(y);
      CHECK(image.GetPixel(index) == expectedPixels[(y * expectedDimensions[0]) + x]);
    }
  }
}

template <typename PixelT>
void CompareImageMetadata(const fs::path& filePath, const std::array<float64, 2>& expectedSpacing, const std::array<float64, 2>& expectedOrigin, bool checkOrigin)
{
  using ImageType = itk::Image<PixelT, 2>;
  auto reader = itk::ImageFileReader<ImageType>::New();
  reader->SetFileName(filePath.string());
  REQUIRE_NOTHROW(reader->Update());

  const auto& image = *reader->GetOutput();
  const auto spacing = image.GetSpacing();
  const auto origin = image.GetOrigin();
  CHECK(spacing[0] == Approx(expectedSpacing[0]));
  CHECK(spacing[1] == Approx(expectedSpacing[1]));
  if(checkOrigin)
  {
    CHECK(origin[0] == Approx(expectedOrigin[0]));
    CHECK(origin[1] == Approx(expectedOrigin[1]));
  }
}

} // namespace

TEMPLATE_TEST_CASE("ITKImageProcessing::ITKImageWriterFilter: Analytical Pixel Order", "[ITKImageProcessing][ITKImageWriterFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32,
                   float64)
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  // Class 1 oracle: the 3x2x2 fixture is value(x, y, z) = x + 10*y + 100*z.
  DataStructure dataStructure;

  const SizeVec3 imageDims = {3, 2, 2};
  const ShapeType arrayDims(std::reverse_iterator(imageDims.end()), std::reverse_iterator(imageDims.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageDims);
  imageGeom->setOrigin({10.0f, 20.0f, 40.0f});
  imageGeom->setSpacing({1.0f, 2.0f, 4.0f});
  auto* cellData = AttributeMatrix::Create(dataStructure, ImageGeom::k_CellAttributeMatrixName, arrayDims, imageGeom->getId());
  imageGeom->setCellData(*cellData);
  auto* imageData = UnitTest::CreateTestDataArray<TestType>(dataStructure, "ImageData", arrayDims, {1}, cellData->getId());
  auto& imageStore = imageData->getDataStoreRef();
  for(usize z = 0; z < imageDims.getZ(); ++z)
  {
    for(usize y = 0; y < imageDims.getY(); ++y)
    {
      for(usize x = 0; x < imageDims.getX(); ++x)
      {
        imageStore[(z * 6) + (y * 3) + x] = static_cast<TestType>(x + (10 * y) + (100 * z));
      }
    }
  }

  const DataPath imageGeomPath({"ImageGeometry"});
  const DataPath imageDataPath = imageGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath("ImageData");
  const fs::path outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / CreateRandomDirName();

  const auto writeAndCheck = [&](const DataPath& inputPath, ChoicesParameter::ValueType plane, const std::string& name, const std::string& extension, const std::array<usize, 2>& dimensions,
                                 const std::array<float64, 2>& expectedSpacing, const std::array<float64, 2>& expectedOrigin, const auto& expectedSlices) {
    ITKImageWriterFilter filter;
    const fs::path outputPath = outputDir / name / fmt::format("slice{}", extension);
    Arguments args;
    args.insertOrAssign(ITKImageWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(imageGeomPath));
    args.insertOrAssign(ITKImageWriterFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(ITKImageWriterFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(ITKImageWriterFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ITKImageWriterFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(plane));
    args.insertOrAssign(ITKImageWriterFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(ITKImageWriterFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    for(usize slice = 0; slice < expectedSlices.size(); slice++)
    {
      const fs::path imagePath = outputDir / name / fmt::format("slice_{:03d}{}", slice, extension);
      CompareImageToExpected(imagePath, dimensions, expectedSlices[slice]);
      CompareImageMetadata<TestType>(imagePath, expectedSpacing, expectedOrigin, extension != ".tif");
    }
  };

  // Rows are y/z respectively. The non-square XY plane makes an X/Y transpose observable.
  const std::string extension = std::is_same_v<TestType, uint8> ? ".tif" : ".mha";
  writeAndCheck(imageDataPath, ITKImageWriterFilter::k_XYPlane, "xy", extension, {3, 2}, {1.0, 2.0}, {10.0, 20.0},
                std::vector<std::vector<TestType>>{{0, 1, 2, 10, 11, 12}, {100, 101, 102, 110, 111, 112}});
  writeAndCheck(imageDataPath, ITKImageWriterFilter::k_XZPlane, "xz", extension, {3, 2}, {1.0, 4.0}, {10.0, 40.0},
                std::vector<std::vector<TestType>>{{0, 1, 2, 100, 101, 102}, {10, 11, 12, 110, 111, 112}});
  writeAndCheck(imageDataPath, ITKImageWriterFilter::k_YZPlane, "yz", extension, {2, 2}, {2.0, 4.0}, {20.0, 40.0},
                std::vector<std::vector<TestType>>{{0, 10, 100, 110}, {1, 11, 101, 111}, {2, 12, 102, 112}});

  std::error_code error;
  fs::remove_all(outputDir, error);
  REQUIRE_FALSE(error);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageWriterFilter: Fill Character Validation", "[ITKImageProcessing][ITKImageWriterFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageDims = {1, 1, 2};
  const ShapeType arrayDims(std::reverse_iterator(imageDims.end()), std::reverse_iterator(imageDims.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageDims);
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  auto* cellData = AttributeMatrix::Create(dataStructure, ImageGeom::k_CellAttributeMatrixName, arrayDims, imageGeom->getId());
  imageGeom->setCellData(*cellData);
  UnitTest::CreateTestDataArray<uint8>(dataStructure, "ImageData", arrayDims, {1}, cellData->getId());

  ITKImageWriterFilter filter;
  Arguments args;
  args.insertOrAssign(ITKImageWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImageWriterFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
  args.insertOrAssign(ITKImageWriterFilter::k_FileName_Key, std::make_any<fs::path>("invalid_fill.tif"));
  args.insertOrAssign(ITKImageWriterFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ITKImageWriterFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(ITKImageWriterFilter::k_XYPlane));
  args.insertOrAssign(ITKImageWriterFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
  const auto checkInvalidFillCharacter = [&](const StringParameter::ValueType& fillCharacter, int32 expectedCode) {
    args.insertOrAssign(ITKImageWriterFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>(fillCharacter));
    const auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
    REQUIRE(preflightResult.outputActions.errors()[0].code == expectedCode);
  };

  SECTION("Empty")
  {
    checkInvalidFillCharacter("", -25601);
  }
  SECTION("Format control character")
  {
    checkInvalidFillCharacter("{", -25602);
  }
  SECTION("Path separator")
  {
    checkInvalidFillCharacter("/", -25602);
  }
  SECTION("ASCII control character")
  {
    checkInvalidFillCharacter("\n", -25602);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageWriterFilter: Dimension Mismatch Validation", "[ITKImageProcessing][ITKImageWriterFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions({1, 1, 2});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  UnitTest::CreateTestDataArray<uint8>(dataStructure, "ImageData", {1, 1, 1}, {1});

  ITKImageWriterFilter filter;
  Arguments args;
  args.insertOrAssign(ITKImageWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImageWriterFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"ImageData"}));
  args.insertOrAssign(ITKImageWriterFilter::k_FileName_Key, std::make_any<fs::path>("dimension_mismatch.tif"));
  args.insertOrAssign(ITKImageWriterFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ITKImageWriterFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(ITKImageWriterFilter::k_XYPlane));
  args.insertOrAssign(ITKImageWriterFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
  args.insertOrAssign(ITKImageWriterFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

  const auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors()[0].code == -25600);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageWriterFilter: 3D Image Single-File Output", "[ITKImageProcessing][ITKImageWriterFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageDims = {3, 1, 2};
  const ShapeType arrayDims(std::reverse_iterator(imageDims.end()), std::reverse_iterator(imageDims.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageDims);
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  auto* cellData = AttributeMatrix::Create(dataStructure, ImageGeom::k_CellAttributeMatrixName, arrayDims, imageGeom->getId());
  imageGeom->setCellData(*cellData);
  auto* imageData = UnitTest::CreateTestDataArray<uint8>(dataStructure, "ImageData", arrayDims, {1}, cellData->getId());
  auto& imageStore = imageData->getDataStoreRef();
  for(usize z = 0; z < imageDims.getZ(); ++z)
  {
    for(usize x = 0; x < imageDims.getX(); ++x)
    {
      imageStore[(z * 3) + x] = static_cast<uint8>(x + (100 * z));
    }
  }

  const fs::path outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / CreateRandomDirName();
  const fs::path outputPath = outputDir / "volume.mha";
  ITKImageWriterFilter filter;
  Arguments args;
  args.insertOrAssign(ITKImageWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImageWriterFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
  args.insertOrAssign(ITKImageWriterFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
  args.insertOrAssign(ITKImageWriterFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ITKImageWriterFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(ITKImageWriterFilter::k_XZPlane));
  args.insertOrAssign(ITKImageWriterFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
  args.insertOrAssign(ITKImageWriterFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(RequireExampleOutputFile(preflightResult, outputPath));
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(fs::exists(outputPath));
  CompareImageToExpected(outputPath, {3, 2}, std::vector<uint8>{0, 1, 2, 100, 101, 102});

  std::error_code error;
  fs::remove_all(outputDir, error);
  REQUIRE_FALSE(error);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageWriterFilter: RGBA Image Output", "[ITKImageProcessing][ITKImageWriterFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  const SizeVec3 imageDims = {1, 1, 1};
  const ShapeType arrayDims(std::reverse_iterator(imageDims.end()), std::reverse_iterator(imageDims.begin()));
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageDims);
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  auto* cellData = AttributeMatrix::Create(dataStructure, ImageGeom::k_CellAttributeMatrixName, arrayDims, imageGeom->getId());
  imageGeom->setCellData(*cellData);
  auto* imageData = UnitTest::CreateTestDataArray<uint8>(dataStructure, "ImageData", arrayDims, {4}, cellData->getId());
  auto& imageStore = imageData->getDataStoreRef();
  imageStore[0] = 10;
  imageStore[1] = 20;
  imageStore[2] = 30;
  imageStore[3] = 40;

  const fs::path outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / CreateRandomDirName();
  const fs::path outputPath = outputDir / "rgba.mha";
  ITKImageWriterFilter filter;
  Arguments args;
  args.insertOrAssign(ITKImageWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ITKImageWriterFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
  args.insertOrAssign(ITKImageWriterFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
  args.insertOrAssign(ITKImageWriterFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ITKImageWriterFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(ITKImageWriterFilter::k_XYPlane));
  args.insertOrAssign(ITKImageWriterFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
  args.insertOrAssign(ITKImageWriterFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  SIMPLNX_RESULT_REQUIRE_VALID(filter.execute(dataStructure, args).result);

  using ImageType = itk::Image<itk::RGBAPixel<uint8>, 2>;
  auto reader = itk::ImageFileReader<ImageType>::New();
  reader->SetFileName(outputPath.string());
  REQUIRE_NOTHROW(reader->Update());
  const auto pixel = reader->GetOutput()->GetPixel({0, 0});
  CHECK(pixel.GetRed() == 10);
  CHECK(pixel.GetGreen() == 20);
  CHECK(pixel.GetBlue() == 30);
  CHECK(pixel.GetAlpha() == 40);

  std::error_code error;
  fs::remove_all(outputDir, error);
  REQUIRE_FALSE(error);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageWriterFilter: Write Stack", "[ITKImageProcessing][ITKImageWriterFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  {
    ITKImportImageStackFilter filter;
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
    std::vector<float64> spacing = {0.3f, 1.2f, 0.9f};

    args.insertOrAssign(ITKImportImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
    args.insertOrAssign(ITKImportImageStackFilter::k_Origin_Key, std::make_any<std::vector<float64>>(origin));
    args.insertOrAssign(ITKImportImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(spacing));
    args.insertOrAssign(ITKImportImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    ITKImageWriterFilter filter;

    const std::string tempDirName = CreateRandomDirName();
    const std::string tempDirPath = fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir.view(), tempDirName);
    const std::string path = fmt::format("{}/{}/slice.tif", unit_test::k_BinaryTestOutputDir.view(), tempDirName);

    const fs::path outputPath = fs::path() / path;

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(ITKImageWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(ITKImageWriterFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(ITKImageWriterFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(ITKImageWriterFilter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(ITKImageWriterFilter::k_Plane_Key, std::make_any<uint64>(ITKImageWriterFilter::k_XYPlane));
    args.insertOrAssign(ITKImageWriterFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(ITKImageWriterFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(RequireExampleOutputFile(preflightResult, outputPath.parent_path() / "slice_100.tif"));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[2], offset, tempDirName, tempDirPath);
  }

  {
    ITKImageWriterFilter filter;

    const std::string tempDirName = CreateRandomDirName();
    const std::string tempDirPath = fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir.view(), tempDirName);
    const std::string path = fmt::format("{}/{}/slice.tif", unit_test::k_BinaryTestOutputDir.view(), tempDirName);

    const fs::path outputPath = fs::path() / path;

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(ITKImageWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(ITKImageWriterFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(ITKImageWriterFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(ITKImageWriterFilter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(ITKImageWriterFilter::k_Plane_Key, std::make_any<uint64>(ITKImageWriterFilter::k_XZPlane));
    args.insertOrAssign(ITKImageWriterFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(ITKImageWriterFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[1], offset, tempDirName, tempDirPath);
  }

  {
    ITKImageWriterFilter filter;

    const std::string tempDirName = CreateRandomDirName();
    const std::string tempDirPath = fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir.view(), tempDirName);
    const std::string path = fmt::format("{}/{}/slice.tif", unit_test::k_BinaryTestOutputDir.view(), tempDirName);

    const fs::path outputPath = fs::path() / path;

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(ITKImageWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(ITKImageWriterFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(ITKImageWriterFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(ITKImageWriterFilter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(ITKImageWriterFilter::k_Plane_Key, std::make_any<uint64>(ITKImageWriterFilter::k_YZPlane));
    args.insertOrAssign(ITKImageWriterFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(ITKImageWriterFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[0], offset, tempDirName, tempDirPath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ITKImageProcessing::ITKImageWriterFilter: SIMPL Backwards Compatibility", "[ITKImageProcessing][ITKImageWriterFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ITKImageWriterFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ITKImageWriterFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ITKImageWriterFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<ChoicesParameter::ValueType>(ITKImageWriterFilter::k_Plane_Key) == 0);
        CHECK(args.value<uint64>(ITKImageWriterFilter::k_IndexOffset_Key) == 5);
      }
      CHECK(args.value<FileSystemPathParameter::ValueType>(ITKImageWriterFilter::k_FileName_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<DataPath>(ITKImageWriterFilter::k_ImageGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ITKImageWriterFilter::k_ImageArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
