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
#include <type_traits>

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
    // Signed types (and floats) span negatives (-8..7) so the roundtrip exercises negative-input
    // normalization; unsigned types keep the plain 0..15 ramp. The oracle (Create Color Map) reads
    // the very same array, so parity is preserved regardless of the offset.
    if constexpr(std::is_signed_v<T>)
    {
      scalarStore[i] = static_cast<T>(i) - static_cast<T>(8);
    }
    else
    {
      scalarStore[i] = static_cast<T>(i);
    }
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

// Builds a 4x4x1 image geometry with a `numComponents`-component array of type T and runs WriteImageFilter
// preflight (color table OFF) against the given output file name, returning the preflight result.
template <typename T>
IFilter::PreflightResult RunFormatPreflightForType(const std::string& fileName, usize numComponents = 1)
{
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  UnitTest::CreateTestDataArray<T>(dataStructure, "Scalar", {1, 4, 4}, {numComponents}, cellAmPtr->getId());

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
  // A multi-component array in color-table mode must fail with the dedicated -27012 code.
  const auto& ctErrors = preflightResult.outputActions.errors();
  REQUIRE(ctErrors.size() == 1);
  REQUIRE(ctErrors[0].code == -27012);

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

TEST_CASE("SimplnxCore::WriteImageFilter: Inline color table 3D multi-slice across planes", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  const std::string presetName = ColorTableUtilities::GetDefaultRGBPresetName();

  // Build a 3D volume with DISTINCT dimensions so every plane's slice count and slice shape differ,
  // exercising the ColorizeVolumeFunctor slice loop and its XY/XZ/YZ index branches.
  const usize dimX = 3;
  const usize dimY = 4;
  const usize dimZ = 5;

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {dimZ, dimY, dimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* scalarPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Scalar", {dimZ, dimY, dimX}, {1}, cellAmPtr->getId());
  auto& scalarStore = scalarPtr->getDataStoreRef();
  for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
  {
    scalarStore[i] = static_cast<float32>(i); // 0..59 ramp
  }

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Oracle: run Create Color Map once. Its RGB output is parallel to the scalar array (tuple index
  // = z*dimY*dimX + y*dimX + x), giving an independent reference for every voxel's expected color.
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
    auto ccmExecute = ccm.execute(dataStructure, ccmArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(ccmExecute.result);
  }

  const DataPath rgbPath = geomPath.createChildPath("CellData").createChildPath("RGB");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(rgbPath));
  const auto& expectedRgbStore = dataStructure.getDataRefAs<UInt8Array>(rgbPath).getDataStoreRef();

  // Writes the volume through the color-table path for the given plane, reads every written slice
  // back, and compares each read pixel to the oracle RGB using an INDEPENDENT source-index derivation.
  auto checkPlane = [&](uint64 planeIndex, usize sliceCount, usize sliceW, usize sliceH) {
    ScopedTempDir planeDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(planeDir.path() / "slice.tif"));
      args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(planeIndex));
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

    // Read the full stack back (one file per slice).
    DataStructure readDs;
    {
      ReadImageStackFilter reader;
      GeneratedFileListParameter::ValueType fileList;
      fileList.inputPath = planeDir.path().string();
      fileList.startIndex = 0;
      fileList.endIndex = static_cast<int32>(sliceCount - 1);
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

    const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    const auto* readArrayPtr = dynamic_cast<const UInt8Array*>(readGeom.getCellData()->begin()->second.get());
    REQUIRE(readArrayPtr != nullptr);
    const auto& readStore = readArrayPtr->getDataStoreRef();
    const usize readComps = readArrayPtr->getNumberOfComponents();
    REQUIRE(readStore.getNumberOfTuples() == sliceCount * sliceH * sliceW);

    for(usize s = 0; s < sliceCount; s++)
    {
      for(usize r = 0; r < sliceH; r++)
      {
        for(usize c = 0; c < sliceW; c++)
        {
          // Independent source-tuple derivation (NOT taken from production code).
          usize srcTuple = 0;
          if(planeIndex == k_XYPlane) // slice=z, row=y, col=x
          {
            srcTuple = s * dimY * dimX + r * dimX + c;
          }
          else if(planeIndex == k_XZPlane) // slice=y, row=z, col=x
          {
            srcTuple = r * dimY * dimX + s * dimX + c;
          }
          else // YZ: slice=x, row=z, col=y
          {
            srcTuple = r * dimY * dimX + c * dimX + s;
          }

          const usize readTuple = s * sliceH * sliceW + r * sliceW + c;
          for(usize comp = 0; comp < 3; comp++)
          {
            INFO(fmt::format("plane={} slice={} row={} col={} comp={}", planeIndex, s, r, c, comp));
            REQUIRE(readStore.getValue(readTuple * readComps + comp) == expectedRgbStore.getValue(srcTuple * 3 + comp));
          }
        }
      }
    }
  };

  checkPlane(k_XYPlane, dimZ, dimX, dimY); // 5 slices of 3x4
  checkPlane(k_XZPlane, dimY, dimX, dimZ); // 4 slices of 3x5
  checkPlane(k_YZPlane, dimX, dimY, dimZ); // 3 slices of 4x5

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  // BMP/JPEG go through the stb backend (uint8 only).
  {
    auto result = RunFormatPreflightForType<uint8>("fmt_u8.bmp");
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  {
    auto result = RunFormatPreflightForType<uint16>("fmt_u16.bmp");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  {
    auto result = RunFormatPreflightForType<uint8>("fmt_u8.jpg");
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  // TIFF writes uint8/uint16/float32 but not float64.
  {
    auto result = RunFormatPreflightForType<float64>("fmt_f64.tif");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }

  // Component-count validation (non-color mode): stb PNG supports {1,3,4} components.
  {
    // A 2-component array is not a conforming write for any format -> -27014.
    auto result = RunFormatPreflightForType<uint8>("fmt_u8_2comp.png", 2);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    const auto& compErrors = result.outputActions.errors();
    REQUIRE(compErrors.size() == 1);
    REQUIRE(compErrors[0].code == -27014);
  }
  {
    // A 3-component (RGB) uint8 array writes directly with no color table.
    auto result = RunFormatPreflightForType<uint8>("fmt_u8_3comp.png", 3);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
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
    // Exercise both mask data types the filter accepts (boolean and uint8 0/1) with identical
    // expected output, covering both branches of MakeMaskPredicate.
    const bool useUint8Mask = GENERATE(false, true);
    DYNAMIC_SECTION("maskType=" << (useUint8Mask ? "uint8" : "bool"))
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
      // Mark voxels 0 and 5 as bad, using either a bool mask or a uint8 (0/1) mask.
      if(useUint8Mask)
      {
        auto* maskPtr = UnitTest::CreateTestDataArray<uint8>(dataStructure, "Mask", {1, 4, 4}, {1}, cellAmPtr->getId());
        auto& maskStore = maskPtr->getDataStoreRef();
        for(usize i = 0; i < maskStore.getNumberOfTuples(); i++)
        {
          maskStore[i] = 1;
        }
        maskStore[0] = 0;
        maskStore[5] = 0;
      }
      else
      {
        auto* maskPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", {1, 4, 4}, {1}, cellAmPtr->getId());
        auto& maskStore = maskPtr->getDataStoreRef();
        for(usize i = 0; i < maskStore.getNumberOfTuples(); i++)
        {
          maskStore[i] = true;
        }
        maskStore[0] = false;
        maskStore[5] = false;
      }

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

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Writes a single-component uint8 image of the given (dimX, dimY) with the supplied row-major pixel
  // values to a fresh temp dir as a PNG with the given flip mode, reads it back via ReadImageStackFilter,
  // and returns the read-back single-component pixel values in row-major (y then x) order.
  auto writeAndReadBackFlip = [&](uint64 flipMode, uint64 dimX, uint64 dimY, const std::vector<uint8>& pixelValues) -> std::vector<uint8> {
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({dimX, dimY, 1});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, dimY, dimX}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = UnitTest::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, dimY, dimX}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    REQUIRE(pixelValues.size() == scalarStore.getNumberOfTuples());
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
    const usize numPixels = dimX * dimY;
    REQUIRE(readStore.getNumberOfTuples() == numPixels);

    std::vector<uint8> result(numPixels);
    for(usize i = 0; i < numPixels; i++)
    {
      result[i] = readStore.getValue(i * readComps + 0);
    }
    return result;
  };

  // Tiny asymmetric 3x2x1 (X,Y,Z) single-component uint8 image (even height):
  //   row y=0: 0  1  2
  //   row y=1: 10 11 12
  const std::vector<uint8> pixels3x2{0, 1, 2, 10, 11, 12};

  SECTION("None: rows unchanged")
  {
    const auto pixels = writeAndReadBackFlip(k_FlipNone, 3, 2, pixels3x2);
    REQUIRE(pixels == std::vector<uint8>{0, 1, 2, 10, 11, 12});
  }

  SECTION("FlipAboutXAxis: row order reversed (top-to-bottom mirror)")
  {
    const auto pixels = writeAndReadBackFlip(k_FlipAboutX, 3, 2, pixels3x2);
    REQUIRE(pixels == std::vector<uint8>{10, 11, 12, 0, 1, 2});
  }

  SECTION("FlipAboutYAxis: pixel order within each row reversed (left-to-right mirror)")
  {
    const auto pixels = writeAndReadBackFlip(k_FlipAboutY, 3, 2, pixels3x2);
    REQUIRE(pixels == std::vector<uint8>{2, 1, 0, 12, 11, 10});
  }

  SECTION("FlipAboutXAxis with odd height: end rows swap, middle row fixed")
  {
    // 3x3x1 (odd height). FlipAboutXAxis reverses row order: rows 0 and 2 swap, row 1 (middle) stays.
    //   row y=0: 0  1  2          row y=0: 20 21 22
    //   row y=1: 10 11 12   -->   row y=1: 10 11 12   (unchanged)
    //   row y=2: 20 21 22         row y=2: 0  1  2
    const std::vector<uint8> pixels3x3{0, 1, 2, 10, 11, 12, 20, 21, 22};
    const auto pixels = writeAndReadBackFlip(k_FlipAboutX, 3, 3, pixels3x3);
    REQUIRE(pixels == std::vector<uint8>{20, 21, 22, 10, 11, 12, 0, 1, 2});
  }
}

TEST_CASE("SimplnxCore::WriteImageFilter: Output flip composes with the color-table path", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  constexpr uint64 k_FlipNone = 0;
  constexpr uint64 k_FlipAboutX = 1;
  constexpr uint64 k_FlipAboutY = 2;

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
  const std::vector<uint8> flipYImage = writeColorAndReadBack(k_FlipAboutY);

  constexpr usize width = 4;
  constexpr usize height = 4;

  // Build the expected row-reversed image from the None result (4 rows of 4 pixels * 3 components)
  // and compare against the FlipAboutXAxis result: this proves the flip composes with the color
  // path without re-asserting the color math (already covered by the roundtrip tests above).
  std::vector<uint8> expectedFlipXImage(noneImage.size());
  for(usize y = 0; y < height; y++)
  {
    const usize srcRow = y;
    const usize dstRow = height - 1 - y;
    std::copy(noneImage.begin() + static_cast<std::ptrdiff_t>(srcRow * width * 3), noneImage.begin() + static_cast<std::ptrdiff_t>((srcRow + 1) * width * 3),
              expectedFlipXImage.begin() + static_cast<std::ptrdiff_t>(dstRow * width * 3));
  }
  REQUIRE(flipXImage == expectedFlipXImage);

  // Build the expected column-reversed image from the None result by reversing the 3-byte RGB pixel
  // groups within each row (multi-byte pixel stride) and compare against the FlipAboutYAxis result.
  std::vector<uint8> expectedFlipYImage(noneImage.size());
  for(usize y = 0; y < height; y++)
  {
    for(usize x = 0; x < width; x++)
    {
      const usize srcPixel = y * width + x;
      const usize dstPixel = y * width + (width - 1 - x);
      for(usize c = 0; c < 3; c++)
      {
        expectedFlipYImage[dstPixel * 3 + c] = noneImage[srcPixel * 3 + c];
      }
    }
  }
  REQUIRE(flipYImage == expectedFlipYImage);
}

TEST_CASE("SimplnxCore::WriteImageFilter: Scale bar preflight validation", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  const DataPath geomPath({"ImageGeometry"});
  const DataPath cellPath = geomPath.createChildPath("CellData");

  // Builds a 200x100x1 ImageGeom with the requested cell array type/comps, spacing and units,
  // then preflights WriteImageFilter with the scale bar enabled.
  auto preflightScaleBar = [&](DataType dataType, usize numComps, const FloatVec3& spacing, bool createColorTable) -> IFilter::PreflightResult {
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({200, 100, 1});
    imageGeomPtr->setSpacing(spacing);
    imageGeomPtr->setUnits(IGeometry::LengthUnit::Micrometer);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 100, 200}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    DataPath arrayPath = cellPath.createChildPath("Data");
    switch(dataType)
    {
    case DataType::uint8:
      UnitTest::CreateTestDataArray<uint8>(dataStructure, "Data", {1, 100, 200}, {numComps}, cellAmPtr->getId());
      break;
    case DataType::uint16:
      UnitTest::CreateTestDataArray<uint16>(dataStructure, "Data", {1, 100, 200}, {numComps}, cellAmPtr->getId());
      break;
    default:
      FAIL("unhandled test data type");
    }

    WriteImageFilter filter;
    Arguments args;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(arrayPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "scale_bar_preflight.png"));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
    args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(createColorTable));
    args.insertOrAssign(WriteImageFilter::k_AddScaleBar_Key, std::make_any<bool>(true));
    return filter.preflight(dataStructure, args);
  };

  SECTION("uint8 single component passes and reports the padded size")
  {
    auto result = preflightScaleBar(DataType::uint8, 1, {0.5f, 0.5f, 1.0f}, false);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    // band = max(24, llround(0.08*100)=8) = 24 -> 200 x 124
    bool foundSizeValue = false;
    for(const auto& value : result.outputValues)
    {
      if(value.name == "Output Image Size (with scale bar)")
      {
        REQUIRE(value.value == "200 x 124");
        foundSizeValue = true;
      }
    }
    REQUIRE(foundSizeValue);
  }

  SECTION("uint16 without color table fails with a color-table hint")
  {
    auto result = preflightScaleBar(DataType::uint16, 1, {0.5f, 0.5f, 1.0f}, false);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }

  SECTION("uint8 with 2 components fails")
  {
    auto result = preflightScaleBar(DataType::uint8, 2, {0.5f, 0.5f, 1.0f}, false);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }

  SECTION("uint16 with color table enabled passes")
  {
    auto result = preflightScaleBar(DataType::uint16, 1, {0.5f, 0.5f, 1.0f}, true);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }

  SECTION("zero horizontal spacing fails")
  {
    auto result = preflightScaleBar(DataType::uint8, 1, {0.0f, 0.5f, 1.0f}, false);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
}

TEST_CASE("SimplnxCore::WriteImageFilter: Scale bar pads the written image with a band", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Writes a 200x100 single-component uint8 ramp at 0.5 µm/pixel as PNG (with or without the
  // scale bar / inline color table), reads the file back via ReadImageStackFilter into readDs.
  // Follows the writeAndReadBackFlip pattern. When createColorTable is true the filter's default
  // preset is used (the preset argument is left unset so IFilter fills in the parameter default).
  auto writeAndReadBack = [&](bool addScaleBar, bool createColorTable, DataStructure& readDs) {
    constexpr usize dimX = 200;
    constexpr usize dimY = 100;
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({dimX, dimY, 1});
    imageGeomPtr->setSpacing({0.5f, 0.5f, 1.0f});
    imageGeomPtr->setUnits(IGeometry::LengthUnit::Micrometer);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, dimY, dimX}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = UnitTest::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, dimY, dimX}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
    {
      scalarStore[i] = static_cast<uint8>(i % 251); // asymmetric ramp
    }

    ScopedTempDir tempDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.png"));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
      args.insertOrAssign(WriteImageFilter::k_AddScaleBar_Key, std::make_any<bool>(addScaleBar));
      args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(createColorTable));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);

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
  };

  // Expected layout for 200x100 @ 0.5 µm/px:
  //   band = max(24, llround(0.08*100)=8) = 24  -> written image is 200 x 124
  //   nice = ComputeNiceBarLength(200, 0.5) : target 25 µm -> 20 µm -> 40 px bar
  //   margin = 3, thickness = 2 -> bar rows (global) 100+24-3-2 = 119 and 120
  //   barStartCol = (200-40)/2 = 80 -> bar cols 80..119
  constexpr usize dimX = 200;
  constexpr usize dimY = 100;
  constexpr usize bandHeight = 24;

  DataStructure withBarDs;
  writeAndReadBack(true, false, withBarDs);
  DataStructure withoutBarDs;
  writeAndReadBack(false, false, withoutBarDs);

  REQUIRE_NOTHROW(withBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
  const auto& readGeom = withBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  SizeVec3 readDims = readGeom.getDimensions();
  REQUIRE(readDims[0] == dimX);
  REQUIRE(readDims[1] == dimY + bandHeight);

  const auto* readArrayPtr = dynamic_cast<const UInt8Array*>(readGeom.getCellData()->begin()->second.get());
  REQUIRE(readArrayPtr != nullptr);
  const auto& readStore = readArrayPtr->getDataStoreRef();
  const usize readComps = readArrayPtr->getNumberOfComponents();
  REQUIRE(readComps == 3); // scale-bar output is always RGB

  REQUIRE_NOTHROW(withoutBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
  const auto& plainGeom = withoutBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  const auto* plainArrayPtr = dynamic_cast<const UInt8Array*>(plainGeom.getCellData()->begin()->second.get());
  REQUIRE(plainArrayPtr != nullptr);
  const auto& plainStore = plainArrayPtr->getDataStoreRef();
  const usize plainComps = plainArrayPtr->getNumberOfComponents();

  auto barPixel = [&](usize row, usize col) -> std::array<uint8, 3> {
    const usize i = (row * dimX + col) * readComps;
    return {readStore.getValue(i), readStore.getValue(i + 1), readStore.getValue(i + 2)};
  };

  SECTION("image region is untouched and grayscale-replicated")
  {
    for(usize row = 0; row < dimY; row++)
    {
      for(usize col = 0; col < dimX; col++)
      {
        const uint8 expected = plainStore.getValue((row * dimX + col) * plainComps);
        const std::array<uint8, 3> rgb = barPixel(row, col);
        REQUIRE(rgb[0] == expected);
        REQUIRE(rgb[1] == expected);
        REQUIRE(rgb[2] == expected);
      }
    }
  }

  SECTION("band background is white and the bar is a crisp black run")
  {
    const std::array<uint8, 3> white = {255, 255, 255};
    const std::array<uint8, 3> black = {0, 0, 0};
    REQUIRE(barPixel(dimY, 0) == white);
    REQUIRE(barPixel(dimY + bandHeight - 1, dimX - 1) == white);
    for(usize row : {usize(119), usize(120)})
    {
      REQUIRE(barPixel(row, 79) == white);
      for(usize col = 80; col < 120; col++)
      {
        REQUIRE(barPixel(row, col) == black);
      }
      REQUIRE(barPixel(row, 120) == white);
    }
  }

  SECTION("label text renders in the band above the bar")
  {
    bool foundTextPixel = false;
    for(usize row = dimY; row < usize(119) && !foundTextPixel; row++)
    {
      for(usize col = 0; col < dimX; col++)
      {
        if(barPixel(row, col)[0] < 128)
        {
          foundTextPixel = true;
          break;
        }
      }
    }
    REQUIRE(foundTextPixel);
  }

  SECTION("scale bar composes with the inline color-table path")
  {
    // Color table on, bar on vs. color table on, bar off: the colorized image region must be
    // identical, with the band appended below it.
    DataStructure colorBarDs;
    writeAndReadBack(true, true, colorBarDs);
    DataStructure colorPlainDs;
    writeAndReadBack(false, true, colorPlainDs);

    REQUIRE_NOTHROW(colorBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
    const auto& colorBarGeom = colorBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    SizeVec3 colorBarDims = colorBarGeom.getDimensions();
    REQUIRE(colorBarDims[0] == dimX);
    REQUIRE(colorBarDims[1] == dimY + bandHeight);

    const auto* colorBarArrayPtr = dynamic_cast<const UInt8Array*>(colorBarGeom.getCellData()->begin()->second.get());
    REQUIRE(colorBarArrayPtr != nullptr);
    const auto& colorBarStore = colorBarArrayPtr->getDataStoreRef();
    REQUIRE(colorBarArrayPtr->getNumberOfComponents() == 3);

    REQUIRE_NOTHROW(colorPlainDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
    const auto& colorPlainGeom = colorPlainDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    const auto* colorPlainArrayPtr = dynamic_cast<const UInt8Array*>(colorPlainGeom.getCellData()->begin()->second.get());
    REQUIRE(colorPlainArrayPtr != nullptr);
    const auto& colorPlainStore = colorPlainArrayPtr->getDataStoreRef();
    REQUIRE(colorPlainArrayPtr->getNumberOfComponents() == 3);

    for(usize i = 0; i < dimX * dimY * 3; i++)
    {
      REQUIRE(colorBarStore.getValue(i) == colorPlainStore.getValue(i));
    }
  }
}

TEST_CASE("SimplnxCore::WriteImageFilter: Scale bar composes with flip and RGB input", "[SimplnxCore][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  constexpr uint64 k_FlipAboutX = 1;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath rgbPath = geomPath.createChildPath("CellData").createChildPath("Rgb");

  // Tiny 3x2 RGB image with distinct per-pixel colors, flipped about X, with the scale bar.
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({3, 2, 1});
  imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeomPtr->setUnits(IGeometry::LengthUnit::Micrometer);
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 2, 3}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* rgbPtr = UnitTest::CreateTestDataArray<uint8>(dataStructure, "Rgb", {1, 2, 3}, {3}, cellAmPtr->getId());
  auto& rgbStore = rgbPtr->getDataStoreRef();
  // row y=0: (10,11,12) (20,21,22) (30,31,32) ; row y=1: (110,111,112) (120,121,122) (130,131,132)
  const std::vector<uint8> srcRgb = {10, 11, 12, 20, 21, 22, 30, 31, 32, 110, 111, 112, 120, 121, 122, 130, 131, 132};
  for(usize i = 0; i < srcRgb.size(); i++)
  {
    rgbStore[i] = srcRgb[i];
  }

  ScopedTempDir tempDir(fs::path(unit_test::k_BinaryTestOutputDir.view()));
  {
    WriteImageFilter filter;
    Arguments args;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(rgbPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.png"));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
    args.insertOrAssign(WriteImageFilter::k_FlipMode_Key, std::make_any<ChoicesParameter::ValueType>(k_FlipAboutX));
    args.insertOrAssign(WriteImageFilter::k_AddScaleBar_Key, std::make_any<bool>(true));

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

  REQUIRE_NOTHROW(readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
  const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  SizeVec3 readDims = readGeom.getDimensions();
  REQUIRE(readDims[0] == 3);
  REQUIRE(readDims[1] == 2 + 24); // band = max(24, llround(0.08*2)=0) = 24

  const auto* readArrayPtr = dynamic_cast<const UInt8Array*>(readGeom.getCellData()->begin()->second.get());
  REQUIRE(readArrayPtr != nullptr);
  const auto& readStore = readArrayPtr->getDataStoreRef();
  REQUIRE(readArrayPtr->getNumberOfComponents() == 3);

  // Flip about X applies to the image region only: source row 1 is now on top, the band stays below.
  const std::vector<uint8> expectedTopRows = {110, 111, 112, 120, 121, 122, 130, 131, 132, 10, 11, 12, 20, 21, 22, 30, 31, 32};
  for(usize i = 0; i < expectedTopRows.size(); i++)
  {
    REQUIRE(readStore.getValue(i) == expectedTopRows[i]);
  }
  // First band row is white background
  REQUIRE(readStore.getValue((2 * 3 + 0) * 3) == 255);
}
