#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageWriter.hpp"
#include "ITKImageProcessing/Filters/ITKImportImageStack.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const std::string k_ImageStackDir = unit_test::k_DataDir.str() + "/ImageStack";
const DataPath k_ImageGeomPath = {{"ImageGeometry"}};
const DataPath k_ImageDataPath = k_ImageGeomPath.createChildPath(ImageGeom::k_CellDataName).createChildPath("ImageData");

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

} // namespace

TEST_CASE("ITKImageProcessing::ITKImageWriter: Write Stack", "[ITKImageProcessing][ITKImageWriter]")
{
  auto app = Application::GetOrCreateInstance();
  DataStructure dataStructure;
  {
    ITKImportImageStack filter;
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
    std::vector<float32> spacing = {0.3f, 1.2f, 0.9f};

    args.insertOrAssign(ITKImportImageStack::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
    args.insertOrAssign(ITKImportImageStack::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
    args.insertOrAssign(ITKImportImageStack::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
    args.insertOrAssign(ITKImportImageStack::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    ITKImageWriter filter;

    const std::string tempDirName = CreateRandomDirName();
    const std::string tempDirPath = fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir.view(), tempDirName);
    const std::string path = fmt::format("{}/{}/slice.tif", unit_test::k_BinaryTestOutputDir.view(), tempDirName);

    const fs::path outputPath = fs::path() / path;

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(ITKImageWriter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(ITKImageWriter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(ITKImageWriter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(ITKImageWriter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(ITKImageWriter::k_Plane_Key, std::make_any<uint64>(ITKImageWriter::k_XYPlane));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[2], offset, tempDirName, tempDirPath);
  }

  {
    ITKImageWriter filter;

    const std::string tempDirName = CreateRandomDirName();
    const std::string tempDirPath = fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir.view(), tempDirName);
    const std::string path = fmt::format("{}/{}/slice.tif", unit_test::k_BinaryTestOutputDir.view(), tempDirName);

    const fs::path outputPath = fs::path() / path;

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(ITKImageWriter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(ITKImageWriter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(ITKImageWriter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(ITKImageWriter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(ITKImageWriter::k_Plane_Key, std::make_any<uint64>(ITKImageWriter::k_XZPlane));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[1], offset, tempDirName, tempDirPath);
  }

  {
    ITKImageWriter filter;

    const std::string tempDirName = CreateRandomDirName();
    const std::string tempDirPath = fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir.view(), tempDirName);
    const std::string path = fmt::format("{}/{}/slice.tif", unit_test::k_BinaryTestOutputDir.view(), tempDirName);

    const fs::path outputPath = fs::path() / path;

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(ITKImageWriter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(ITKImageWriter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(ITKImageWriter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(ITKImageWriter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(ITKImageWriter::k_Plane_Key, std::make_any<uint64>(ITKImageWriter::k_YZPlane));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[0], offset, tempDirName, tempDirPath);
  }
}
