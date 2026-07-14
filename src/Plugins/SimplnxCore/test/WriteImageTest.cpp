#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/CreateColorMapFilter.hpp"
#include "SimplnxCore/Filters/ReadImageStackFilter.hpp"
#include "SimplnxCore/Filters/WriteImageFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CreateColorMapParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"

#include <array>
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

// Writes a single-slice scalar ramp of type T through the inline color-table path, then compares the
// read-back RGB image tuple-by-tuple against an independent CreateColorMap -> RGB reference. The oracle
// is the real CreateColorMap chain, so this asserts parity across every numeric input type.
template <typename T>
void RunColorRoundtripForType()
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  const std::string presetName = ColorTableUtilities::GetDefaultRGBPresetName();

  // Build a small single-slice XY volume with a known scalar ramp of type T.
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* scalarPtr = UnitTest::CreateTestDataArray<T>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
  auto& scalarStore = scalarPtr->getDataStoreRef();
  for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
  {
    scalarStore[i] = static_cast<T>(i); // 0..15 ramp
  }
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // (A) Inline color-table write.
  ScopedTempDir inlineDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
  {
    WriteImageFilter filter;
    Arguments args;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(inlineDir.path() / "slice.tif"));
    args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
    args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
    args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
    args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // (B) Reference: Create Color Map then plain Write Image of the RGB array.
  {
    CreateColorMapFilter ccm;
    Arguments ccmArgs;
    ccmArgs.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
    ccmArgs.insertOrAssign(CreateColorMapFilter::k_SelectedDataArrayPath_Key, std::make_any<DataPath>(scalarPath));
    ccmArgs.insertOrAssign(CreateColorMapFilter::k_UseMask_Key, std::make_any<bool>(false));
    ccmArgs.insertOrAssign(CreateColorMapFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    ccmArgs.insertOrAssign(CreateColorMapFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));
    ccmArgs.insertOrAssign(CreateColorMapFilter::k_RgbArrayPath_Key, std::make_any<std::string>("RGB"));
    auto ccmPreflight = ccm.preflight(dataStructure, ccmArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(ccmPreflight.outputActions);
    // NOTE: store the execute result before asserting; SIMPLNX_RESULT_REQUIRE_VALID expands its
    // argument multiple times, so inlining ccm.execute(...) would re-run execute and fail because
    // the RGB output array already exists on the second call.
    auto ccmExecute = ccm.execute(dataStructure, ccmArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(ccmExecute.result);
  }

  const DataPath rgbPath = geomPath.createChildPath("CellData").createChildPath("RGB");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(rgbPath));
  const auto& expectedRgbStore = dataStructure.getDataRefAs<UInt8Array>(rgbPath).getDataStoreRef();

  // Read the inline-written slice back and compare pixel RGB to the Create Color Map result.
  DataStructure readDs;
  {
    ReadImageStackFilter reader;
    GeneratedFileListParameter::ValueType fileList;
    fileList.inputPath = inlineDir.path().string();
    fileList.startIndex = 0;
    fileList.endIndex = 0;
    fileList.incrementIndex = 1;
    fileList.fileExtension = ".tif";
    fileList.filePrefix = "slice_";
    fileList.fileSuffix = "";
    fileList.paddingDigits = 3;
    fileList.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

    Arguments rArgs;
    rArgs.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileList));
    rArgs.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ReadGeom"})));
    auto readerPreflight = reader.preflight(readDs, rArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(readerPreflight.outputActions);
    auto readerExecute = reader.execute(readDs, rArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(readerExecute.result);
  }

  // Locate the single RGB image-data array created by the reader and compare tuple-by-tuple.
  const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  const auto& readCellAm = readGeom.getCellData();
  const auto* readArrayPtr = dynamic_cast<const UInt8Array*>(readCellAm->begin()->second.get());
  REQUIRE(readArrayPtr != nullptr);
  const auto& readStore = readArrayPtr->getDataStoreRef();

  // ReadImageStackFilter reads the color TIFF back as a 3-component (RGB) uint8 array; compare only
  // the R,G,B components per pixel so the assertion is robust if a reader ever emits RGBA.
  const usize readComps = readArrayPtr->getNumberOfComponents();
  const usize numPixels = readStore.getNumberOfTuples();
  REQUIRE(numPixels == expectedRgbStore.getNumberOfTuples());
  for(usize p = 0; p < numPixels; p++)
  {
    for(usize c = 0; c < 3; c++)
    {
      REQUIRE(readStore.getValue(p * readComps + c) == expectedRgbStore.getValue(p * 3 + c));
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Builds a 4x4x1 image geometry with a single-component array of type T and runs WriteImageFilter
// preflight (color table OFF) against the given output file name, returning the preflight result.
template <typename T>
IFilter::PreflightResult RunFormatPreflightForType(const std::string& fileName)
{
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  UnitTest::CreateTestDataArray<T>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName));
  args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
  args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
  args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
  args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(ColorTableUtilities::GetDefaultRGBPresetName()));
  args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

  return filter.preflight(dataStructure, args);
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

TEST_CASE("SimplnxCore::WriteImageFilter: Color table preflight validation", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  // A 3-component array (invalid for color-table mode).
  UnitTest::CreateTestDataArray<uint8>(dataStructure, "RGBInput", {1, 4, 4}, {3}, cellAmPtr->getId());

  const DataPath geomPath({"ImageGeometry"});
  const DataPath arrayPath = geomPath.createChildPath("CellData").createChildPath("RGBInput");

  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(arrayPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ct_pf.tif"));
  args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
  args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
  args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
  args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(ColorTableUtilities::GetDefaultRGBPresetName()));
  args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteImageFilter: Inline color table across numeric input types", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  DYNAMIC_SECTION("int8")
  {
    RunColorRoundtripForType<int8>();
  }
  DYNAMIC_SECTION("uint8")
  {
    RunColorRoundtripForType<uint8>();
  }
  DYNAMIC_SECTION("int16")
  {
    RunColorRoundtripForType<int16>();
  }
  DYNAMIC_SECTION("uint16")
  {
    RunColorRoundtripForType<uint16>();
  }
  DYNAMIC_SECTION("int32")
  {
    RunColorRoundtripForType<int32>();
  }
  DYNAMIC_SECTION("uint32")
  {
    RunColorRoundtripForType<uint32>();
  }
  DYNAMIC_SECTION("int64")
  {
    RunColorRoundtripForType<int64>();
  }
  DYNAMIC_SECTION("uint64")
  {
    RunColorRoundtripForType<uint64>();
  }
  DYNAMIC_SECTION("float32")
  {
    RunColorRoundtripForType<float32>();
  }
  DYNAMIC_SECTION("float64")
  {
    RunColorRoundtripForType<float64>();
  }
}

TEST_CASE("SimplnxCore::WriteImageFilter: Format-aware write-type preflight", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  // STB backend (.png/.jpg/.bmp) only writes uint8; TIFF backend writes uint8/uint16/float32.
  {
    auto result = RunFormatPreflightForType<float32>("fmt_f32.png");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  {
    auto result = RunFormatPreflightForType<uint16>("fmt_u16.png");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  {
    auto result = RunFormatPreflightForType<uint8>("fmt_u8.png");
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  {
    auto result = RunFormatPreflightForType<float32>("fmt_f32.tif");
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  {
    auto result = RunFormatPreflightForType<uint16>("fmt_u16.tif");
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  {
    auto result = RunFormatPreflightForType<int32>("fmt_i32.tif");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  UnitTest::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteImageFilter: Inline color table mask and constant-array handling", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  const std::string presetName = ColorTableUtilities::GetDefaultRGBPresetName();
  const DataPath geomPath({"ImageGeometry"});

  // Reads a single-slice color image back from a directory and returns the pointer to the RGB array.
  auto readBackSlice = [&](const fs::path& dir, DataStructure& readDs) -> const UInt8Array* {
    ReadImageStackFilter reader;
    GeneratedFileListParameter::ValueType fileList;
    fileList.inputPath = dir.string();
    fileList.startIndex = 0;
    fileList.endIndex = 0;
    fileList.incrementIndex = 1;
    fileList.fileExtension = ".tif";
    fileList.filePrefix = "slice_";
    fileList.fileSuffix = "";
    fileList.paddingDigits = 3;
    fileList.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

    Arguments rArgs;
    rArgs.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileList));
    rArgs.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ReadGeom"})));
    auto rPreflight = reader.preflight(readDs, rArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(rPreflight.outputActions);
    auto rExecute = reader.execute(readDs, rArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(rExecute.result);
    const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    return dynamic_cast<const UInt8Array*>(readGeom.getCellData()->begin()->second.get());
  };

  SECTION("Masked voxels read back as the masked color")
  {
    const std::vector<uint8> maskedColor{10, 20, 30};

    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({4, 4, 1});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
    {
      scalarStore[i] = static_cast<float32>(i);
    }
    // Mark voxels 0 and 5 as bad.
    auto* maskPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", {1, 4, 4}, {1}, cellAmPtr->getId());
    auto& maskStore = maskPtr->getDataStoreRef();
    for(usize i = 0; i < maskStore.getNumberOfTuples(); i++)
    {
      maskStore[i] = true;
    }
    maskStore[0] = false;
    maskStore[5] = false;

    const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");
    const DataPath maskPath = geomPath.createChildPath("CellData").createChildPath("Mask");

    ScopedTempDir maskDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(maskDir.path() / "slice.tif"));
      args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
      args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
      args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(true));
      args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
      args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(maskedColor));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    DataStructure readDs;
    const UInt8Array* readArrayPtr = readBackSlice(maskDir.path(), readDs);
    REQUIRE(readArrayPtr != nullptr);
    const auto& readStore = readArrayPtr->getDataStoreRef();
    const usize readComps = readArrayPtr->getNumberOfComponents();

    // For an XY single slice, read pixel index equals voxel index; voxels 0 and 5 must be the masked color.
    for(usize badPixel : {static_cast<usize>(0), static_cast<usize>(5)})
    {
      REQUIRE(readStore.getValue(badPixel * readComps + 0) == maskedColor[0]);
      REQUIRE(readStore.getValue(badPixel * readComps + 1) == maskedColor[1]);
      REQUIRE(readStore.getValue(badPixel * readComps + 2) == maskedColor[2]);
    }
    // A known-good voxel must NOT be the masked color.
    const bool goodDiffers =
        readStore.getValue(1 * readComps + 0) != maskedColor[0] || readStore.getValue(1 * readComps + 1) != maskedColor[1] || readStore.getValue(1 * readComps + 2) != maskedColor[2];
    REQUIRE(goodDiffers);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Constant-valued input maps every pixel to the first control color")
  {
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({4, 4, 1});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
    {
      scalarStore[i] = 5.0F; // constant -> arrayMin == arrayMax
    }

    const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

    // Expected color: normalized value of a constant array is 0.0 -> the first control color of the preset.
    auto controlPointsResult = ColorTableUtilities::ExtractControlPoints(presetName);
    SIMPLNX_RESULT_REQUIRE_VALID(controlPointsResult);
    const std::vector<float32> controlPoints = controlPointsResult.value();
    const std::vector<float32> binPoints = ColorTableUtilities::NormalizeBinPoints(controlPoints);
    const std::array<uint8, 3> expectedColor = ColorTableUtilities::ComputeRgbFromControlPoints(0.0F, binPoints, controlPoints, controlPoints.size() / 4);

    ScopedTempDir constDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(constDir.path() / "slice.tif"));
      args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
      args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
      args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
      args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
      args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    DataStructure readDs;
    const UInt8Array* readArrayPtr = readBackSlice(constDir.path(), readDs);
    REQUIRE(readArrayPtr != nullptr);
    const auto& readStore = readArrayPtr->getDataStoreRef();
    const usize readComps = readArrayPtr->getNumberOfComponents();
    const usize numPixels = readStore.getNumberOfTuples();
    REQUIRE(numPixels == 16);
    for(usize p = 0; p < numPixels; p++)
    {
      REQUIRE(readStore.getValue(p * readComps + 0) == expectedColor[0]);
      REQUIRE(readStore.getValue(p * readComps + 1) == expectedColor[1]);
      REQUIRE(readStore.getValue(p * readComps + 2) == expectedColor[2]);
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::WriteImageFilter: Flip output image about X or Y axis", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  // WriteImageFilter flip_mode ChoicesParameter index values.
  constexpr uint64 k_FlipNone = 0;
  constexpr uint64 k_FlipAboutX = 1;
  constexpr uint64 k_FlipAboutY = 2;

  // Tiny asymmetric 3x2x1 (X,Y,Z) single-component uint8 image:
  //   row y=0: 0  1  2
  //   row y=1: 10 11 12
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Writes the tiny 3x2x1 image to a fresh temp dir as a PNG with the given flip mode, reads it back
  // via ReadImageStackFilter, and returns the read-back single-component pixel values in row-major
  // (y then x) order: {row0[0], row0[1], row0[2], row1[0], row1[1], row1[2]}.
  auto writeAndReadBackFlip = [&](uint64 flipMode) -> std::array<uint8, 6> {
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({3, 2, 1});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 2, 3}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = UnitTest::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, 2, 3}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    const std::array<uint8, 6> pixelValues{0, 1, 2, 10, 11, 12};
    for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
    {
      scalarStore[i] = pixelValues[i];
    }

    ScopedTempDir tempDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.png"));
      args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_FlipMode_Key, std::make_any<ChoicesParameter::ValueType>(flipMode));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);

    DataStructure readDs;
    ReadImageStackFilter reader;
    GeneratedFileListParameter::ValueType fileList;
    fileList.inputPath = tempDir.path().string();
    fileList.startIndex = 0;
    fileList.endIndex = 0;
    fileList.incrementIndex = 1;
    fileList.fileExtension = ".png";
    fileList.filePrefix = "slice_";
    fileList.fileSuffix = "";
    fileList.paddingDigits = 3;
    fileList.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

    Arguments rArgs;
    rArgs.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileList));
    rArgs.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ReadGeom"})));
    auto readerPreflight = reader.preflight(readDs, rArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(readerPreflight.outputActions);
    auto readerExecute = reader.execute(readDs, rArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(readerExecute.result);

    const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    const auto* readArrayPtr = dynamic_cast<const UInt8Array*>(readGeom.getCellData()->begin()->second.get());
    REQUIRE(readArrayPtr != nullptr);
    const auto& readStore = readArrayPtr->getDataStoreRef();
    const usize readComps = readArrayPtr->getNumberOfComponents();
    // Confirmed by manual run: a single-channel (grayscale) PNG reads back as 1 component. Compare
    // only the first channel regardless, so the assertion stays meaningful if a reader ever widens it.
    REQUIRE(readStore.getNumberOfTuples() == 6);

    std::array<uint8, 6> result{};
    for(usize i = 0; i < 6; i++)
    {
      result[i] = readStore.getValue(i * readComps + 0);
    }
    return result;
  };

  SECTION("None: rows unchanged")
  {
    const auto pixels = writeAndReadBackFlip(k_FlipNone);
    REQUIRE(pixels == std::array<uint8, 6>{0, 1, 2, 10, 11, 12});
  }

  SECTION("FlipAboutXAxis: row order reversed (top-to-bottom mirror)")
  {
    const auto pixels = writeAndReadBackFlip(k_FlipAboutX);
    REQUIRE(pixels == std::array<uint8, 6>{10, 11, 12, 0, 1, 2});
  }

  SECTION("FlipAboutYAxis: pixel order within each row reversed (left-to-right mirror)")
  {
    const auto pixels = writeAndReadBackFlip(k_FlipAboutY);
    REQUIRE(pixels == std::array<uint8, 6>{2, 1, 0, 12, 11, 10});
  }
}

TEST_CASE("SimplnxCore::WriteImageFilter: Output flip composes with the color-table path", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  constexpr uint64 k_FlipNone = 0;
  constexpr uint64 k_FlipAboutX = 1;

  const std::string presetName = ColorTableUtilities::GetDefaultRGBPresetName();
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Writes a 4x4x1 scalar ramp through the color-table path with the given flip mode and returns the
  // read-back RGB image as a flat vector of 4*4*3 uint8 values in row-major (y then x) order.
  auto writeColorAndReadBack = [&](uint64 flipMode) -> std::vector<uint8> {
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({4, 4, 1});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
    {
      scalarStore[i] = static_cast<float32>(i); // 0..15 ramp
    }

    ScopedTempDir tempDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
      args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
      args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
      args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
      args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
      args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));
      args.insertOrAssign(WriteImageFilter::k_FlipMode_Key, std::make_any<ChoicesParameter::ValueType>(flipMode));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);

    DataStructure readDs;
    ReadImageStackFilter reader;
    GeneratedFileListParameter::ValueType fileList;
    fileList.inputPath = tempDir.path().string();
    fileList.startIndex = 0;
    fileList.endIndex = 0;
    fileList.incrementIndex = 1;
    fileList.fileExtension = ".tif";
    fileList.filePrefix = "slice_";
    fileList.fileSuffix = "";
    fileList.paddingDigits = 3;
    fileList.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

    Arguments rArgs;
    rArgs.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileList));
    rArgs.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ReadGeom"})));
    auto readerPreflight = reader.preflight(readDs, rArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(readerPreflight.outputActions);
    auto readerExecute = reader.execute(readDs, rArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(readerExecute.result);

    const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    const auto* readArrayPtr = dynamic_cast<const UInt8Array*>(readGeom.getCellData()->begin()->second.get());
    REQUIRE(readArrayPtr != nullptr);
    const auto& readStore = readArrayPtr->getDataStoreRef();
    const usize readComps = readArrayPtr->getNumberOfComponents();
    const usize numPixels = readStore.getNumberOfTuples();
    REQUIRE(numPixels == 16);

    std::vector<uint8> result(numPixels * 3);
    for(usize p = 0; p < numPixels; p++)
    {
      for(usize c = 0; c < 3; c++)
      {
        result[p * 3 + c] = readStore.getValue(p * readComps + c);
      }
    }
    return result;
  };

  const std::vector<uint8> noneImage = writeColorAndReadBack(k_FlipNone);
  const std::vector<uint8> flipXImage = writeColorAndReadBack(k_FlipAboutX);

  // Build the expected row-reversed image from the None result (4 rows of 4 pixels * 3 components)
  // and compare against the FlipAboutXAxis result: this proves the flip composes with the color
  // path without re-asserting the color math (already covered by the roundtrip tests above).
  constexpr usize width = 4;
  constexpr usize height = 4;
  std::vector<uint8> expectedFlipXImage(noneImage.size());
  for(usize y = 0; y < height; y++)
  {
    const usize srcRow = y;
    const usize dstRow = height - 1 - y;
    std::copy(noneImage.begin() + static_cast<std::ptrdiff_t>(srcRow * width * 3), noneImage.begin() + static_cast<std::ptrdiff_t>((srcRow + 1) * width * 3),
              expectedFlipXImage.begin() + static_cast<std::ptrdiff_t>(dstRow * width * 3));
  }
  REQUIRE(flipXImage == expectedFlipXImage);
}
