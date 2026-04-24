#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ReadImageStackFilter.hpp"
#include "SimplnxCore/Filters/WriteImageFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
// The WriteImage test re-uses the ImageStack data from the ITKImageProcessing plugin source tree.
// The ITK plugin keeps a small set of input slices in its in-source data directory and the ITK
// test references them via `unit_test::k_DataDir`. When this test lives in SimplnxCore, the
// `k_DataDir` macro points to SimplnxCore's data directory instead, so we reach over to the ITK
// plugin's data directory using `k_SimplnxSourceDIr`.
const std::string k_ImageStackDir = std::string(unit_test::k_SimplnxSourceDIr.view()) + "/src/Plugins/ITKImageProcessing/data/ImageStack";
const DataPath k_ImageGeomPath = {{"ImageGeometry"}};
const DataPath k_ImageDataPath = k_ImageGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath("ImageData");

// WriteImageFilter plane choices (ChoicesParameter index values)
constexpr uint64 k_XYPlane = 0;
constexpr uint64 k_XZPlane = 1;
constexpr uint64 k_YZPlane = 2;

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

// Creates a unique temporary directory on construction and removes it on destruction.
// Using RAII so the directory is cleaned up even if a REQUIRE() fails mid-test.
class ScopedTempDir
{
public:
  explicit ScopedTempDir(const fs::path& parentDir)
  : m_Name(CreateRandomDirName())
  , m_Path(parentDir / m_Name)
  {
    fs::create_directories(m_Path);
  }

  ~ScopedTempDir() noexcept
  {
    std::error_code ec;
    fs::remove_all(m_Path, ec);
    if(ec)
    {
      std::cerr << "ScopedTempDir: failed to remove '" << m_Path.string() << "': " << ec.message() << '\n';
    }
  }

  ScopedTempDir(const ScopedTempDir&) = delete;
  ScopedTempDir& operator=(const ScopedTempDir&) = delete;
  ScopedTempDir(ScopedTempDir&&) = delete;
  ScopedTempDir& operator=(ScopedTempDir&&) = delete;

  const std::string& name() const
  {
    return m_Name;
  }
  const fs::path& path() const
  {
    return m_Path;
  }

private:
  std::string m_Name;
  fs::path m_Path;
};

void validateOutputFiles(size_t numImages, uint64 offset, const std::string& tempDirName, const fs::path& tempDirPath)
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
}

} // namespace

TEST_CASE("SimplnxCore::WriteImageFilter: Write Stack", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  {
    ReadImageStackFilter filter;
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
  }

  {
    WriteImageFilter filter;

    ScopedTempDir tempDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
    const fs::path outputPath = tempDir.path() / "slice.tif";

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[2], offset, tempDir.name(), tempDir.path());
  }

  {
    WriteImageFilter filter;

    ScopedTempDir tempDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
    const fs::path outputPath = tempDir.path() / "slice.tif";

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XZPlane));
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[1], offset, tempDir.name(), tempDir.path());
  }

  {
    WriteImageFilter filter;

    ScopedTempDir tempDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
    const fs::path outputPath = tempDir.path() / "slice.tif";

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_YZPlane));
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[0], offset, tempDir.name(), tempDir.path());
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
