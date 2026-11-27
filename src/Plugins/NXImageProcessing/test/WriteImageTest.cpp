#include <catch2/catch.hpp>

#include "NXImageProcessing/Filters/ReadImageFilter.hpp"
#include "NXImageProcessing/Filters/ReadImageStackFilter.hpp"
#include "NXImageProcessing/Filters/ReadMhaFileFilter.hpp"
#include "NXImageProcessing/Filters/WriteImageFilter.hpp"
#include "NXImageProcessing/NXImageProcessing_test_dirs.hpp"
#include "SimplnxCore/Filters/CreateColorMapFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
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
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

#include <array>
#include <atomic>
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
// The test uses the image-stack fixtures from the sibling ITKImageProcessing plugin.
const std::string k_ImageStackDir = (fs::path(unit_test::k_SourceDir.view()) / "data/ImageStack").string();
const fs::path k_BinaryTestOutputDir = fs::path(unit_test::k_BinaryDir.view()) / "test_output";
const DataPath k_ImageGeomPath = {{"ImageGeometry"}};
const DataPath k_ImageDataPath = k_ImageGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath("ImageData");

// WriteImageFilter plane choices (ChoicesParameter index values)
constexpr uint64 k_XYPlane = 0;
constexpr uint64 k_XZPlane = 1;
constexpr uint64 k_YZPlane = 2;

template <class T>
class ExtentCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<std::vector<T>> readExtent(const Extent& extent) const override
  {
    m_ReturnedExtentReadCount++;
    m_InsideReturnedExtentRead = true;
    Result<std::vector<T>> values = DataStore<T>::readExtent(extent);
    m_InsideReturnedExtentRead = false;
    return values;
  }

  Result<> readExtentIntoBuffer(const Extent& extent, nonstd::span<T> destination) const override
  {
    if(!m_InsideReturnedExtentRead)
    {
      m_CallerProvidedExtentReadCount++;
      m_MaxCallerProvidedExtentValues = std::max(m_MaxCallerProvidedExtentValues, destination.size());
    }
    return DataStore<T>::readExtentIntoBuffer(extent, destination);
  }

  usize returnedExtentReadCount() const noexcept
  {
    return m_ReturnedExtentReadCount;
  }

  usize callerProvidedExtentReadCount() const noexcept
  {
    return m_CallerProvidedExtentReadCount;
  }

  usize maxCallerProvidedExtentValues() const noexcept
  {
    return m_MaxCallerProvidedExtentValues;
  }

private:
  mutable bool m_InsideReturnedExtentRead = false;
  mutable usize m_ReturnedExtentReadCount = 0;
  mutable usize m_CallerProvidedExtentReadCount = 0;
  mutable usize m_MaxCallerProvidedExtentValues = 0;
};

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

void ValidateOutputFiles(size_t numImages, uint64 offset, const std::string& tempDirName, const fs::path& tempDirPath)
{
  // Check and remove each expected image file.
  for(size_t i = 0; i < numImages; i++)
  {
    fs::path imagePath = fs::path() / fmt::format("{}/{}/slice_{:03d}.tif", k_BinaryTestOutputDir.string(), tempDirName, i + offset);
    INFO(fmt::format("Checking File: '{}'  ", imagePath.string()));
    REQUIRE(fs::exists(imagePath));
    REQUIRE(std::filesystem::remove(imagePath));
  }

  // Make sure that no files remain in the directory.
  int count = 0;
  for(const auto& entry : std::filesystem::directory_iterator(tempDirPath))
  {
    count++;
  }
  REQUIRE(count == 0);
}

// Reads one image into the "ReadGeom" Image Geometry.
// Single-slice output has no index, so the test reads the file directly.
void ReadBackSingleImage(DataStructure& readDs, const fs::path& imagePath)
{
  ReadImageFilter reader;
  Arguments rArgs;
  rArgs.insertOrAssign(ReadImageFilter::k_FileName_Key, std::make_any<fs::path>(imagePath));
  rArgs.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ReadGeom"})));
  auto readerPreflight = reader.preflight(readDs, rArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readerPreflight.output_actions);
  auto readerExecute = reader.execute(readDs, rArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readerExecute.result);
}

// Writes one scalar slice through the color-table path.
// The test compares the RGB result to an independent Create Color Map result.
template <typename T>
void RunColorRoundtripForType(unit_test::AlgorithmTestScenario scenario)
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  const std::string presetName = color_table_utilities::GetDefaultRGBPresetName();

  // Build a small single-slice XY volume with a known scalar ramp of type T.
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* scalarPtr = unit_test::CreateTestDataArray<T>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
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
  ScopedTempDir inlineDir(k_BinaryTestOutputDir);
  {
    WriteImageFilter filter;
    Arguments args;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(inlineDir.path() / "slice.tif"));
    args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
    args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
    args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
    args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
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
    SIMPLNX_RESULT_REQUIRE_VALID(ccmPreflight.output_actions);
    // Store the result because SIMPLNX_RESULT_REQUIRE_VALID evaluates its argument more than once.
    // Repeated execution fails because the RGB output already exists.
    auto ccmExecute = ccm.execute(dataStructure, ccmArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(ccmExecute.result);
  }

  const DataPath rgbPath = geomPath.createChildPath("CellData").createChildPath("RGB");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8ArrayType>(rgbPath));
  const auto& expectedRgbStore = dataStructure.getDataRefAs<UInt8ArrayType>(rgbPath).getDataStoreRef();

  // Read the inline-written slice back and compare pixel RGB to the Create Color Map result.
  DataStructure readDs;
  ReadBackSingleImage(readDs, inlineDir.path() / "slice.tif");

  // Locate the single RGB image-data array created by the reader and compare tuple-by-tuple.
  const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  const auto& readCellAm = readGeom.getCellData();
  const auto* readArrayPtr = dynamic_cast<const UInt8ArrayType*>(readCellAm->begin()->second.get());
  REQUIRE(readArrayPtr != nullptr);
  const auto& readStore = readArrayPtr->getDataStoreRef();

  // Compare only the RGB values.
  // This keeps the test valid if a reader returns RGBA data.
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

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// Preflights a 4 x 4 x 1 image with the selected type and component count.
template <typename T>
IFilter::PreflightResult RunFormatPreflightForType(const std::string& fileName, usize numComponents = 1)
{
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  unit_test::CreateTestDataArray<T>(dataStructure, "Scalar", {1, 4, 4}, {numComponents}, cellAmPtr->getId());

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(k_BinaryTestOutputDir / fileName));
  args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
  args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
  args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
  args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(color_table_utilities::GetDefaultRGBPresetName()));
  args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

  return filter.preflight(dataStructure, args);
}

} // namespace

TEST_CASE("NXImageProcessing::WriteImageFilter: Write Stack", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  {
    ReadImageStackFilter filter;
    Arguments args;

    GeneratedFileListParameter::ValueType fileListInfo;
    fileListInfo.input_path = k_ImageStackDir;
    fileListInfo.start_index = 11;
    fileListInfo.end_index = 13;
    fileListInfo.increment_index = 1;
    fileListInfo.file_extension = ".tif";
    fileListInfo.file_prefix = "slice_";
    fileListInfo.file_suffix = "";
    fileListInfo.padding_digits = 2;
    fileListInfo.ordering = GeneratedFileListParameter::OrderingType::LowToHigh;

    std::vector<float32> origin = {1.0f, 4.0f, 8.0f};
    std::vector<float32> spacing = {0.3f, 1.2f, 0.9f};

    args.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileListInfo));
    args.insertOrAssign(ReadImageStackFilter::k_ChangeOrigin_Key, true);
    args.insertOrAssign(ReadImageStackFilter::k_Origin_Key, std::make_any<std::vector<float32>>(origin));
    args.insertOrAssign(ReadImageStackFilter::k_ChangeSpacing_Key, true);
    args.insertOrAssign(ReadImageStackFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(spacing));
    args.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    WriteImageFilter filter;

    ScopedTempDir tempDir(k_BinaryTestOutputDir);
    const fs::path outputPath = tempDir.path() / "slice.tif";

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3Type imageDims = imageGeomPtr->getDimensions();

    ValidateOutputFiles(imageDims[2], offset, tempDir.name(), tempDir.path());
  }

  {
    WriteImageFilter filter;

    ScopedTempDir tempDir(k_BinaryTestOutputDir);
    const fs::path outputPath = tempDir.path() / "slice.tif";

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XZPlane));
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3Type imageDims = imageGeomPtr->getDimensions();

    ValidateOutputFiles(imageDims[1], offset, tempDir.name(), tempDir.path());
  }

  {
    WriteImageFilter filter;

    ScopedTempDir tempDir(k_BinaryTestOutputDir);
    const fs::path outputPath = tempDir.path() / "slice.tif";

    Arguments args;
    const uint64 offset = 100;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(k_ImageDataPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
    args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(offset));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_YZPlane));
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3Type imageDims = imageGeomPtr->getDimensions();

    ValidateOutputFiles(imageDims[0], offset, tempDir.name(), tempDir.path());
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Single-slice write omits the index suffix", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  unit_test::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  const fs::path outputPath = tempDir.path() / "single.tif";

  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  // The preflight example previews the exact single-slice file name (no index suffix).
  bool foundExample = false;
  for(const auto& value : preflightResult.output_values)
  {
    if(value.name == "Example Output File")
    {
      foundExample = true;
      REQUIRE(value.value == fs::absolute(outputPath).string());
    }
  }
  REQUIRE(foundExample);

  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // The single slice is written with exactly the user-specified name and no other files appear.
  REQUIRE(fs::exists(outputPath));
  REQUIRE(!fs::exists(tempDir.path() / "single_000.tif"));
  usize fileCount = 0;
  for(const auto& entry : fs::directory_iterator(tempDir.path()))
  {
    fileCount++;
  }
  REQUIRE(fileCount == 1);
  REQUIRE(fs::remove(outputPath));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::WriteImageFilter: MHA preserves ITK scalar data and plane metadata", "[NXImageProcessing][WriteImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64, float32, float64)
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({3, 1, 2});
  REQUIRE(imageGeomPtr->setOrigin({10.0f, 20.0f, 40.0f}).valid());
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 2.0f, 4.0f}).valid());
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {2, 1, 3}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  auto* imageDataPtr = unit_test::CreateTestDataArray<TestType>(dataStructure, "ImageData", {2, 1, 3}, {1}, cellDataPtr->getId());
  auto& imageStore = imageDataPtr->getDataStoreRef();
  const std::array<int32, 6> expectedValues = {0, 1, 2, 100, 101, 102};
  for(usize index = 0; index < expectedValues.size(); ++index)
  {
    imageStore[index] = static_cast<TestType>(expectedValues[index]);
  }

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  const fs::path outputPath = tempDir.path() / "xz.mha";
  WriteImageFilter writer;
  Arguments writeArgs = writer.getDefaultArguments();
  writeArgs.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  writeArgs.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "ImageData"})));
  writeArgs.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
  writeArgs.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XZPlane));

  auto writePreflightResult = writer.preflight(dataStructure, writeArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(writePreflightResult.output_actions);
  auto writeResult = algorithmTestScope.executeFilter(writer, dataStructure, writeArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult.result);

  DataStructure readDataStructure;
  ReadMhaFileFilter reader;
  Arguments readArgs = reader.getDefaultArguments();
  const DataPath readGeomPath({"ReadGeometry"});
  readArgs.insertOrAssign(ReadMhaFileFilter::k_InputFilePath_Key, std::make_any<fs::path>(outputPath));
  readArgs.insertOrAssign(ReadMhaFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(readGeomPath));
  readArgs.insertOrAssign(ReadMhaFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("CellData"));
  readArgs.insertOrAssign(ReadMhaFileFilter::k_ImageDataArrayName_Key, std::make_any<std::string>("ImageData"));

  auto readPreflightResult = reader.preflight(readDataStructure, readArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readPreflightResult.output_actions);
  auto readExecuteResult = reader.execute(readDataStructure, readArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readExecuteResult.result);

  const auto& readGeom = readDataStructure.getDataRefAs<ImageGeom>(readGeomPath);
  CHECK(readGeom.getDimensions() == SizeVec3Type{3, 2, 1});
  CHECK(readGeom.getOrigin() == FloatVec3Type{10.0f, 40.0f, 0.0f});
  CHECK(readGeom.getSpacing() == FloatVec3Type{1.0f, 4.0f, 1.0f});

  const DataPath readArrayPath = readGeomPath.createChildPath("CellData").createChildPath("ImageData");
  REQUIRE_NOTHROW(readDataStructure.getDataRefAs<DataArray<TestType>>(readArrayPath));
  const auto& readStore = readDataStructure.getDataRefAs<DataArray<TestType>>(readArrayPath).getDataStoreRef();
  REQUIRE(readStore.getNumberOfTuples() == expectedValues.size());
  for(usize index = 0; index < expectedValues.size(); ++index)
  {
    CHECK(readStore[index] == static_cast<TestType>(expectedValues[index]));
  }
}

TEST_CASE("NXImageProcessing::WriteImageFilter: MHA maps metadata for every plane", "[NXImageProcessing][WriteImageFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({3, 2, 2});
  REQUIRE(imageGeomPtr->setOrigin({10.0f, 20.0f, 40.0f}).valid());
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 2.0f, 4.0f}).valid());
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {2, 2, 3}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  unit_test::CreateTestDataArray<uint8>(dataStructure, "ImageData", {2, 2, 3}, {1}, cellDataPtr->getId());

  struct PlaneMetadata
  {
    ChoicesParameter::ValueType plane;
    SizeVec3Type dimensions;
    FloatVec3Type origin;
    FloatVec3Type spacing;
  };
  const std::array<PlaneMetadata, 3> metadataCases = {PlaneMetadata{.plane = k_XYPlane, .dimensions = {3, 2, 1}, .origin = {10.0f, 20.0f, 0.0f}, .spacing = {1.0f, 2.0f, 1.0f}},
                                                      PlaneMetadata{.plane = k_XZPlane, .dimensions = {3, 2, 1}, .origin = {10.0f, 40.0f, 0.0f}, .spacing = {1.0f, 4.0f, 1.0f}},
                                                      PlaneMetadata{.plane = k_YZPlane, .dimensions = {2, 2, 1}, .origin = {20.0f, 40.0f, 0.0f}, .spacing = {2.0f, 4.0f, 1.0f}}};

  for(const PlaneMetadata& metadataCase : metadataCases)
  {
    DYNAMIC_SECTION("plane=" << metadataCase.plane)
    {
      ScopedTempDir tempDir(k_BinaryTestOutputDir);
      const fs::path outputPath = tempDir.path() / "slice.mha";
      WriteImageFilter writer;
      Arguments writeArgs = writer.getDefaultArguments();
      writeArgs.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
      writeArgs.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "ImageData"})));
      writeArgs.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
      writeArgs.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(metadataCase.plane));
      auto writePreflightResult = writer.preflight(dataStructure, writeArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(writePreflightResult.output_actions);
      auto writeResult = algorithmTestScope.executeFilter(writer, dataStructure, writeArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(writeResult.result);

      DataStructure readDataStructure;
      ReadMhaFileFilter reader;
      Arguments readArgs = reader.getDefaultArguments();
      const DataPath readGeomPath({"ReadGeometry"});
      readArgs.insertOrAssign(ReadMhaFileFilter::k_InputFilePath_Key, std::make_any<fs::path>(tempDir.path() / "slice_000.mha"));
      readArgs.insertOrAssign(ReadMhaFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(readGeomPath));
      readArgs.insertOrAssign(ReadMhaFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("CellData"));
      readArgs.insertOrAssign(ReadMhaFileFilter::k_ImageDataArrayName_Key, std::make_any<std::string>("ImageData"));
      auto readPreflightResult = reader.preflight(readDataStructure, readArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(readPreflightResult.output_actions);
      auto readExecuteResult = reader.execute(readDataStructure, readArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(readExecuteResult.result);

      const auto& readGeom = readDataStructure.getDataRefAs<ImageGeom>(readGeomPath);
      CHECK(readGeom.getDimensions() == metadataCase.dimensions);
      CHECK(readGeom.getOrigin() == metadataCase.origin);
      CHECK(readGeom.getSpacing() == metadataCase.spacing);
    }
  }
}

TEST_CASE("NXImageProcessing::WriteImageFilter: MHA preserves ITK component counts", "[NXImageProcessing][WriteImageFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  for(const usize componentCount : std::array<usize, 6>{2, 3, 4, 10, 11, 36})
  {
    DYNAMIC_SECTION("components=" << componentCount)
    {
      DataStructure dataStructure;
      auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
      imageGeomPtr->setDimensions({2, 1, 1});
      auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 2}, imageGeomPtr->getId());
      imageGeomPtr->setCellData(*cellDataPtr);
      auto* imageDataPtr = unit_test::CreateTestDataArray<uint8>(dataStructure, "ImageData", {1, 1, 2}, {componentCount}, cellDataPtr->getId());
      auto& imageStore = imageDataPtr->getDataStoreRef();
      for(usize index = 0; index < imageStore.getSize(); ++index)
      {
        imageStore[index] = static_cast<uint8>(index);
      }

      ScopedTempDir tempDir(k_BinaryTestOutputDir);
      const fs::path outputPath = tempDir.path() / "components.mha";
      WriteImageFilter writer;
      Arguments writeArgs = writer.getDefaultArguments();
      writeArgs.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
      writeArgs.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "ImageData"})));
      writeArgs.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
      auto writePreflightResult = writer.preflight(dataStructure, writeArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(writePreflightResult.output_actions);
      auto writeResult = algorithmTestScope.executeFilter(writer, dataStructure, writeArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(writeResult.result);

      DataStructure readDataStructure;
      ReadMhaFileFilter reader;
      Arguments readArgs = reader.getDefaultArguments();
      const DataPath readGeomPath({"ReadGeometry"});
      readArgs.insertOrAssign(ReadMhaFileFilter::k_InputFilePath_Key, std::make_any<fs::path>(outputPath));
      readArgs.insertOrAssign(ReadMhaFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(readGeomPath));
      readArgs.insertOrAssign(ReadMhaFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("CellData"));
      readArgs.insertOrAssign(ReadMhaFileFilter::k_ImageDataArrayName_Key, std::make_any<std::string>("ImageData"));
      auto readPreflightResult = reader.preflight(readDataStructure, readArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(readPreflightResult.output_actions);
      auto readExecuteResult = reader.execute(readDataStructure, readArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(readExecuteResult.result);

      const DataPath readArrayPath = readGeomPath.createChildPath("CellData").createChildPath("ImageData");
      const auto& readArray = readDataStructure.getDataRefAs<UInt8ArrayType>(readArrayPath);
      CHECK(readArray.getNumberOfComponents() == componentCount);
      const auto& readStore = readArray.getDataStoreRef();
      REQUIRE(readStore.getSize() == imageStore.getSize());
      for(usize index = 0; index < imageStore.getSize(); ++index)
      {
        CHECK(readStore[index] == imageStore[index]);
      }
    }
  }
}

TEST_CASE("NXImageProcessing::WriteImageFilter: validates fill characters and previews the first output index", "[NXImageProcessing][WriteImageFilter]")
{
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({2, 1, 2});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {2, 1, 2}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  unit_test::CreateTestDataArray<uint8>(dataStructure, "ImageData", {2, 1, 2}, {1}, cellDataPtr->getId());

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  WriteImageFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "ImageData"})));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
  args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(100));

  const auto previewResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(previewResult.output_actions);
  REQUIRE(std::any_of(previewResult.output_values.cbegin(), previewResult.output_values.cend(),
                      [](const IFilter::PreflightValue& value) { return value.name == "Example Output File" && value.value.ends_with("slice_100.tif"); }));

  args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<std::string>("/"));
  const auto invalidFillResult = filter.preflight(dataStructure, args);
  REQUIRE(invalidFillResult.output_actions.invalid());
  REQUIRE(invalidFillResult.output_actions.errors()[0].code == -27018);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Color table preflight validation", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  // A 3-component array (invalid for color-table mode).
  unit_test::CreateTestDataArray<uint8>(dataStructure, "RGBInput", {1, 4, 4}, {3}, cellAmPtr->getId());

  const DataPath geomPath({"ImageGeometry"});
  const DataPath arrayPath = geomPath.createChildPath("CellData").createChildPath("RGBInput");

  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(arrayPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(k_BinaryTestOutputDir / "ct_pf.tif"));
  args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
  args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
  args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
  args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(color_table_utilities::GetDefaultRGBPresetName()));
  args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  // A multi-component array in color-table mode must fail with the dedicated -27012 code.
  const auto& ctErrors = preflightResult.output_actions.errors();
  REQUIRE(ctErrors.size() == 1);
  REQUIRE(ctErrors[0].code == -27012);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Inline color table across numeric input types", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  DYNAMIC_SECTION("int8")
  {
    RunColorRoundtripForType<int8>(scenario);
  }
  DYNAMIC_SECTION("uint8")
  {
    RunColorRoundtripForType<uint8>(scenario);
  }
  DYNAMIC_SECTION("int16")
  {
    RunColorRoundtripForType<int16>(scenario);
  }
  DYNAMIC_SECTION("uint16")
  {
    RunColorRoundtripForType<uint16>(scenario);
  }
  DYNAMIC_SECTION("int32")
  {
    RunColorRoundtripForType<int32>(scenario);
  }
  DYNAMIC_SECTION("uint32")
  {
    RunColorRoundtripForType<uint32>(scenario);
  }
  DYNAMIC_SECTION("int64")
  {
    RunColorRoundtripForType<int64>(scenario);
  }
  DYNAMIC_SECTION("uint64")
  {
    RunColorRoundtripForType<uint64>(scenario);
  }
  DYNAMIC_SECTION("float32")
  {
    RunColorRoundtripForType<float32>(scenario);
  }
  DYNAMIC_SECTION("float64")
  {
    RunColorRoundtripForType<float64>(scenario);
  }
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Inline color table 3D multi-slice across planes", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  const std::string presetName = color_table_utilities::GetDefaultRGBPresetName();

  // Distinct dimensions give each plane a different slice count and shape.
  // This fixture covers all plane-index branches.
  const usize dimX = 3;
  const usize dimY = 4;
  const usize dimZ = 5;

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {dimZ, dimY, dimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* scalarPtr = unit_test::CreateTestDataArray<float32>(dataStructure, "Scalar", {dimZ, dimY, dimX}, {1}, cellAmPtr->getId());
  auto& scalarStore = scalarPtr->getDataStoreRef();
  for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
  {
    scalarStore[i] = static_cast<float32>(i); // 0..59 ramp
  }

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Create one RGB oracle. Its tuple order matches the scalar array's Z, Y, X order.
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
    SIMPLNX_RESULT_REQUIRE_VALID(ccmPreflight.output_actions);
    auto ccmExecute = ccm.execute(dataStructure, ccmArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(ccmExecute.result);
  }

  const DataPath rgbPath = geomPath.createChildPath("CellData").createChildPath("RGB");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8ArrayType>(rgbPath));
  const auto& expectedRgbStore = dataStructure.getDataRefAs<UInt8ArrayType>(rgbPath).getDataStoreRef();

  // Writes and reads all colorized slices for one plane.
  // The test uses an independent source-index calculation for the expected RGB values.
  auto checkPlane = [&](uint64 planeIndex, usize sliceCount, usize sliceW, usize sliceH) {
    ScopedTempDir planeDir(k_BinaryTestOutputDir);
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(planeDir.path() / "slice.tif"));
      args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(planeIndex));
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
      args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
      args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
      args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
      args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    // Read the full stack back (one file per slice).
    DataStructure readDs;
    {
      ReadImageStackFilter reader;
      GeneratedFileListParameter::ValueType fileList;
      fileList.input_path = planeDir.path().string();
      fileList.start_index = 0;
      fileList.end_index = static_cast<int32>(sliceCount - 1);
      fileList.increment_index = 1;
      fileList.file_extension = ".tif";
      fileList.file_prefix = "slice_";
      fileList.file_suffix = "";
      fileList.padding_digits = 3;
      fileList.ordering = GeneratedFileListParameter::OrderingType::LowToHigh;

      Arguments rArgs;
      rArgs.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileList));
      rArgs.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ReadGeom"})));
      auto readerPreflight = reader.preflight(readDs, rArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(readerPreflight.output_actions);
      auto readerExecute = reader.execute(readDs, rArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(readerExecute.result);
    }

    const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    const auto* readArrayPtr = dynamic_cast<const UInt8ArrayType*>(readGeom.getCellData()->begin()->second.get());
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

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Format-aware write-type preflight", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();

  // STB backend (.png/.jpg/.bmp) only writes uint8; TIFF backend writes uint8/uint16/float32.
  {
    auto result = RunFormatPreflightForType<float32>("fmt_f32.png");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }
  {
    auto result = RunFormatPreflightForType<uint16>("fmt_u16.png");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }
  {
    auto result = RunFormatPreflightForType<uint8>("fmt_u8.png");
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  {
    auto result = RunFormatPreflightForType<float32>("fmt_f32.tif");
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  {
    auto result = RunFormatPreflightForType<uint16>("fmt_u16.tif");
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  {
    auto result = RunFormatPreflightForType<int32>("fmt_i32.tif");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }
  // BMP/JPEG go through the stb backend (uint8 only).
  {
    auto result = RunFormatPreflightForType<uint8>("fmt_u8.bmp");
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  {
    auto result = RunFormatPreflightForType<uint16>("fmt_u16.bmp");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }
  {
    auto result = RunFormatPreflightForType<uint8>("fmt_u8.jpg");
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  // TIFF writes uint8/uint16/float32 but not float64.
  {
    auto result = RunFormatPreflightForType<float64>("fmt_f64.tif");
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }

  // Component-count validation (non-color mode): stb PNG supports {1,3,4} components.
  {
    // A 2-component array is not a conforming write for any format -> -27014.
    auto result = RunFormatPreflightForType<uint8>("fmt_u8_2comp.png", 2);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    const auto& compErrors = result.output_actions.errors();
    REQUIRE(compErrors.size() == 1);
    REQUIRE(compErrors[0].code == -27014);
  }
  {
    // A 3-component (RGB) uint8 array writes directly with no color table.
    auto result = RunFormatPreflightForType<uint8>("fmt_u8_3comp.png", 3);
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  unit_test::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Inline color table mask and constant-array handling", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  const std::string presetName = color_table_utilities::GetDefaultRGBPresetName();
  const DataPath geomPath({"ImageGeometry"});

  // Reads a single-slice color image back from a directory and returns the pointer to the RGB array.
  auto readBackSlice = [&](const fs::path& dir, DataStructure& readDs) -> const UInt8ArrayType* {
    ReadBackSingleImage(readDs, dir / "slice.tif");
    const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    return dynamic_cast<const UInt8ArrayType*>(readGeom.getCellData()->begin()->second.get());
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
      auto* scalarPtr = unit_test::CreateTestDataArray<float32>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
      auto& scalarStore = scalarPtr->getDataStoreRef();
      for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
      {
        scalarStore[i] = static_cast<float32>(i);
      }
      // Mark voxels 0 and 5 as bad, using either a bool mask or a uint8 (0/1) mask.
      if(useUint8Mask)
      {
        auto* maskPtr = unit_test::CreateTestDataArray<uint8>(dataStructure, "Mask", {1, 4, 4}, {1}, cellAmPtr->getId());
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
        auto* maskPtr = unit_test::CreateTestDataArray<bool>(dataStructure, "Mask", {1, 4, 4}, {1}, cellAmPtr->getId());
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

      ScopedTempDir maskDir(k_BinaryTestOutputDir);
      {
        WriteImageFilter filter;
        Arguments args;
        args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
        args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
        args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(maskDir.path() / "slice.tif"));
        args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
        args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
        args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
        args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
        args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
        args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
        args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(true));
        args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
        args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(maskedColor));

        auto preflightResult = filter.preflight(dataStructure, args);
        SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
        auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
        SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
      }

      DataStructure readDs;
      const UInt8ArrayType* readArrayPtr = readBackSlice(maskDir.path(), readDs);
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

      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }
  }

  SECTION("Constant-valued input maps every pixel to the first control color")
  {
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({4, 4, 1});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = unit_test::CreateTestDataArray<float32>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
    {
      scalarStore[i] = 5.0F; // constant -> arrayMin == arrayMax
    }

    const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

    // Expected color: normalized value of a constant array is 0.0 -> the first control color of the preset.
    auto controlPointsResult = color_table_utilities::ExtractControlPoints(presetName);
    SIMPLNX_RESULT_REQUIRE_VALID(controlPointsResult);
    const std::vector<float32> controlPoints = controlPointsResult.value();
    const std::vector<float32> binPoints = color_table_utilities::NormalizeBinPoints(controlPoints);
    const std::array<uint8, 3> expectedColor = color_table_utilities::ComputeRgbFromControlPoints(0.0F, binPoints, controlPoints, controlPoints.size() / 4);

    ScopedTempDir constDir(k_BinaryTestOutputDir);
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(constDir.path() / "slice.tif"));
      args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
      args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
      args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
      args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
      args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    DataStructure readDs;
    const UInt8ArrayType* readArrayPtr = readBackSlice(constDir.path(), readDs);
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

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Flip output image about X or Y axis", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  // WriteImageFilter flip_mode ChoicesParameter index values.
  constexpr uint64 k_FlipNone = 0;
  constexpr uint64 k_FlipAboutX = 1;
  constexpr uint64 k_FlipAboutY = 2;

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Writes one uint8 PNG with the selected flip and reads it through Read Image Stack.
  // The result uses row-major order.
  auto writeAndReadBackFlip = [&](uint64 flipMode, uint64 dimX, uint64 dimY, const std::vector<uint8>& pixelValues) -> std::vector<uint8> {
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({dimX, dimY, 1});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, dimY, dimX}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = unit_test::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, dimY, dimX}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    REQUIRE(pixelValues.size() == scalarStore.getNumberOfTuples());
    for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
    {
      scalarStore[i] = pixelValues[i];
    }

    ScopedTempDir tempDir(k_BinaryTestOutputDir);
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.png"));
      args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_FlipMode_Key, std::make_any<ChoicesParameter::ValueType>(flipMode));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
    unit_test::CheckArraysInheritTupleDims(dataStructure);

    DataStructure readDs;
    ReadBackSingleImage(readDs, tempDir.path() / "slice.png");

    const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    const auto* readArrayPtr = dynamic_cast<const UInt8ArrayType*>(readGeom.getCellData()->begin()->second.get());
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

TEST_CASE("NXImageProcessing::WriteImageFilter: Output flip composes with the color-table path", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr uint64 k_FlipNone = 0;
  constexpr uint64 k_FlipAboutX = 1;
  constexpr uint64 k_FlipAboutY = 2;

  const std::string presetName = color_table_utilities::GetDefaultRGBPresetName();
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Writes a 4 x 4 x 1 colorized ramp with the selected flip.
  // The result contains row-major RGB values.
  auto writeColorAndReadBack = [&](uint64 flipMode) -> std::vector<uint8> {
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({4, 4, 1});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = unit_test::CreateTestDataArray<float32>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
    {
      scalarStore[i] = static_cast<float32>(i); // 0..15 ramp
    }

    ScopedTempDir tempDir(k_BinaryTestOutputDir);
    {
      WriteImageFilter filter;
      Arguments args;
      args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
      args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
      args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
      args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
      args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32ParameterType::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
      args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
      args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
      args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
      args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));
      args.insertOrAssign(WriteImageFilter::k_FlipMode_Key, std::make_any<ChoicesParameter::ValueType>(flipMode));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
    unit_test::CheckArraysInheritTupleDims(dataStructure);

    DataStructure readDs;
    ReadBackSingleImage(readDs, tempDir.path() / "slice.tif");

    const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    const auto* readArrayPtr = dynamic_cast<const UInt8ArrayType*>(readGeom.getCellData()->begin()->second.get());
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

  constexpr usize k_Width = 4;
  constexpr usize k_Height = 4;

  // Reverse the source rows to calculate the expected X-axis flip.
  // Other tests verify the color calculation.
  std::vector<uint8> expectedFlipXImage(noneImage.size());
  for(usize y = 0; y < k_Height; y++)
  {
    const usize srcRow = y;
    const usize dstRow = k_Height - 1 - y;
    std::copy(noneImage.begin() + static_cast<std::ptrdiff_t>(srcRow * k_Width * 3), noneImage.begin() + static_cast<std::ptrdiff_t>((srcRow + 1) * k_Width * 3),
              expectedFlipXImage.begin() + static_cast<std::ptrdiff_t>(dstRow * k_Width * 3));
  }
  REQUIRE(flipXImage == expectedFlipXImage);

  // Reverse the RGB pixel groups in each row to calculate the expected Y-axis flip.
  std::vector<uint8> expectedFlipYImage(noneImage.size());
  for(usize y = 0; y < k_Height; y++)
  {
    for(usize x = 0; x < k_Width; x++)
    {
      const usize srcPixel = y * k_Width + x;
      const usize dstPixel = y * k_Width + (k_Width - 1 - x);
      for(usize c = 0; c < 3; c++)
      {
        expectedFlipYImage[dstPixel * 3 + c] = noneImage[srcPixel * 3 + c];
      }
    }
  }
  REQUIRE(flipYImage == expectedFlipYImage);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Scale bar preflight validation", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();

  const DataPath geomPath({"ImageGeometry"});
  const DataPath cellPath = geomPath.createChildPath("CellData");

  // Builds a 200x100x1 ImageGeom with the requested cell array type/comps, spacing and units,
  // then preflights WriteImageFilter with the scale bar enabled.
  auto preflightScaleBar = [&](DataType dataType, usize numComps, const FloatVec3Type& spacing, bool createColorTable) -> IFilter::PreflightResult {
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({200, 100, 1});
    REQUIRE(imageGeomPtr->setSpacing(spacing).valid());
    imageGeomPtr->setUnits(IGeometry::LengthUnit::Micrometer);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 100, 200}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    DataPath arrayPath = cellPath.createChildPath("Data");
    switch(dataType)
    {
    case DataType::uint8:
      unit_test::CreateTestDataArray<uint8>(dataStructure, "Data", {1, 100, 200}, {numComps}, cellAmPtr->getId());
      break;
    case DataType::uint16:
      unit_test::CreateTestDataArray<uint16>(dataStructure, "Data", {1, 100, 200}, {numComps}, cellAmPtr->getId());
      break;
    default:
      FAIL("unhandled test data type");
    }

    WriteImageFilter filter;
    Arguments args;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(arrayPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(k_BinaryTestOutputDir / "scale_bar_preflight.png"));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
    args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(createColorTable));
    args.insertOrAssign(WriteImageFilter::k_AddScaleBar_Key, std::make_any<bool>(true));
    return filter.preflight(dataStructure, args);
  };

  SECTION("uint8 single component passes and reports the padded size")
  {
    auto result = preflightScaleBar(DataType::uint8, 1, {0.5f, 0.5f, 1.0f}, false);
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
    // band = max(24, llround(0.08*100)=8) = 24 -> 200 x 124
    bool foundSizeValue = false;
    for(const auto& value : result.output_values)
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
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }

  SECTION("uint8 with 2 components fails")
  {
    auto result = preflightScaleBar(DataType::uint8, 2, {0.5f, 0.5f, 1.0f}, false);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }

  SECTION("uint16 with color table enabled passes")
  {
    auto result = preflightScaleBar(DataType::uint16, 1, {0.5f, 0.5f, 1.0f}, true);
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Scale bar pads the written image with a band", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Writes a 200 x 100 uint8 ramp with 0.5 µm spacing and reads the PNG result.
  // Color-table output uses the default preset.
  auto writeAndReadBack = [&](bool addScaleBar, bool createColorTable, DataStructure& readDs) {
    constexpr usize k_DimX = 200;
    constexpr usize k_DimY = 100;
    DataStructure dataStructure;
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions({k_DimX, k_DimY, 1});
    REQUIRE(imageGeomPtr->setSpacing({0.5f, 0.5f, 1.0f}).valid());
    imageGeomPtr->setUnits(IGeometry::LengthUnit::Micrometer);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, k_DimY, k_DimX}, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* scalarPtr = unit_test::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, k_DimY, k_DimX}, {1}, cellAmPtr->getId());
    auto& scalarStore = scalarPtr->getDataStoreRef();
    for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
    {
      scalarStore[i] = static_cast<uint8>(i % 251); // asymmetric ramp
    }

    ScopedTempDir tempDir(k_BinaryTestOutputDir);
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
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
    unit_test::CheckArraysInheritTupleDims(dataStructure);

    ReadBackSingleImage(readDs, tempDir.path() / "slice.png");
  };

  // Expected layout for 200x100 @ 0.5 µm/px:
  //   band = max(24, llround(0.08*100)=8) = 24  -> written image is 200 x 124
  //   nice = ComputeNiceBarLength(200, 0.5) : target 25 µm -> 20 µm -> 40 px bar
  //   margin = 3, thickness = 2 -> bar vertically centered: rows (global) 100+(24-2)/2 = 111 and 112
  //   barStartCol = margin = 3 (left-justified) -> bar cols 3..42; label starts on the same line at col 3+40+3 = 46
  constexpr usize k_DimX = 200;
  constexpr usize k_DimY = 100;
  constexpr usize k_BandHeight = 24;

  DataStructure withBarDs;
  writeAndReadBack(true, false, withBarDs);
  DataStructure withoutBarDs;
  writeAndReadBack(false, false, withoutBarDs);

  REQUIRE_NOTHROW(withBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
  const auto& readGeom = withBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  SizeVec3Type readDims = readGeom.getDimensions();
  REQUIRE(readDims[0] == k_DimX);
  REQUIRE(readDims[1] == k_DimY + k_BandHeight);

  const auto* readArrayPtr = dynamic_cast<const UInt8ArrayType*>(readGeom.getCellData()->begin()->second.get());
  REQUIRE(readArrayPtr != nullptr);
  const auto& readStore = readArrayPtr->getDataStoreRef();
  const usize readComps = readArrayPtr->getNumberOfComponents();
  REQUIRE(readComps == 3); // scale-bar output is always RGB

  REQUIRE_NOTHROW(withoutBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
  const auto& plainGeom = withoutBarDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  const auto* plainArrayPtr = dynamic_cast<const UInt8ArrayType*>(plainGeom.getCellData()->begin()->second.get());
  REQUIRE(plainArrayPtr != nullptr);
  const auto& plainStore = plainArrayPtr->getDataStoreRef();
  const usize plainComps = plainArrayPtr->getNumberOfComponents();

  auto barPixel = [&](usize row, usize col) -> std::array<uint8, 3> {
    const usize i = (row * k_DimX + col) * readComps;
    return {readStore.getValue(i), readStore.getValue(i + 1), readStore.getValue(i + 2)};
  };

  SECTION("image region is untouched and grayscale-replicated")
  {
    for(usize row = 0; row < k_DimY; row++)
    {
      for(usize col = 0; col < k_DimX; col++)
      {
        const uint8 expected = plainStore.getValue((row * k_DimX + col) * plainComps);
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
    REQUIRE(barPixel(k_DimY, 0) == white);
    REQUIRE(barPixel(k_DimY + k_BandHeight - 1, k_DimX - 1) == white);
    for(usize row : {usize(111), usize(112)})
    {
      REQUIRE(barPixel(row, 2) == white);
      for(usize col = 3; col < 43; col++)
      {
        REQUIRE(barPixel(row, col) == black);
      }
      REQUIRE(barPixel(row, 43) == white);
    }
  }

  SECTION("label text renders in the band on the same line as the bar")
  {
    bool foundTextPixel = false;
    for(usize row = k_DimY; row < k_DimY + k_BandHeight && !foundTextPixel; row++)
    {
      for(usize col = 46; col < k_DimX; col++)
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
    SizeVec3Type colorBarDims = colorBarGeom.getDimensions();
    REQUIRE(colorBarDims[0] == k_DimX);
    REQUIRE(colorBarDims[1] == k_DimY + k_BandHeight);

    const auto* colorBarArrayPtr = dynamic_cast<const UInt8ArrayType*>(colorBarGeom.getCellData()->begin()->second.get());
    REQUIRE(colorBarArrayPtr != nullptr);
    const auto& colorBarStore = colorBarArrayPtr->getDataStoreRef();
    REQUIRE(colorBarArrayPtr->getNumberOfComponents() == 3);

    REQUIRE_NOTHROW(colorPlainDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
    const auto& colorPlainGeom = colorPlainDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
    const auto* colorPlainArrayPtr = dynamic_cast<const UInt8ArrayType*>(colorPlainGeom.getCellData()->begin()->second.get());
    REQUIRE(colorPlainArrayPtr != nullptr);
    const auto& colorPlainStore = colorPlainArrayPtr->getDataStoreRef();
    REQUIRE(colorPlainArrayPtr->getNumberOfComponents() == 3);

    for(usize i = 0; i < k_DimX * k_DimY * 3; i++)
    {
      REQUIRE(colorBarStore.getValue(i) == colorPlainStore.getValue(i));
    }
  }
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Scale bar composes with flip and RGB input", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr uint64 k_FlipAboutX = 1;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath rgbPath = geomPath.createChildPath("CellData").createChildPath("Rgb");

  // Tiny 3x2 RGB image with distinct per-pixel colors, flipped about X, with the scale bar.
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({3, 2, 1});
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  imageGeomPtr->setUnits(IGeometry::LengthUnit::Micrometer);
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 2, 3}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* rgbPtr = unit_test::CreateTestDataArray<uint8>(dataStructure, "Rgb", {1, 2, 3}, {3}, cellAmPtr->getId());
  auto& rgbStore = rgbPtr->getDataStoreRef();
  // row y=0: (10,11,12) (20,21,22) (30,31,32) ; row y=1: (110,111,112) (120,121,122) (130,131,132)
  const std::vector<uint8> srcRgb = {10, 11, 12, 20, 21, 22, 30, 31, 32, 110, 111, 112, 120, 121, 122, 130, 131, 132};
  for(usize i = 0; i < srcRgb.size(); i++)
  {
    rgbStore[i] = srcRgb[i];
  }

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);

  DataStructure readDs;
  ReadBackSingleImage(readDs, tempDir.path() / "slice.png");

  REQUIRE_NOTHROW(readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
  const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  SizeVec3Type readDims = readGeom.getDimensions();
  REQUIRE(readDims[0] == 3);
  REQUIRE(readDims[1] == 2 + 24); // band = max(24, llround(0.08*2)=0) = 24

  const auto* readArrayPtr = dynamic_cast<const UInt8ArrayType*>(readGeom.getCellData()->begin()->second.get());
  REQUIRE(readArrayPtr != nullptr);
  const auto& readStore = readArrayPtr->getDataStoreRef();
  REQUIRE(readArrayPtr->getNumberOfComponents() == 3);

  // The X-axis flip puts source row 1 on top. The scale-bar band stays below the image.
  const std::vector<uint8> expectedTopRows = {110, 111, 112, 120, 121, 122, 130, 131, 132, 10, 11, 12, 20, 21, 22, 30, 31, 32};
  for(usize i = 0; i < expectedTopRows.size(); i++)
  {
    REQUIRE(readStore.getValue(i) == expectedTopRows[i]);
  }
  // First band row is white background
  REQUIRE(readStore.getValue((2 * 3 + 0) * 3) == 255);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: Scale bar on XZ-plane slices locks per-plane dims and horizontal spacing", "[NXImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Distinct dimensions separate the XZ width and height from the slice count.
  // Distinct spacing detects selection of the incorrect horizontal axis.
  constexpr usize k_DimX = 8;
  constexpr usize k_DimY = 4;
  constexpr usize k_DimZ = 6;

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({k_DimX, k_DimY, k_DimZ});
  REQUIRE(imageGeomPtr->setSpacing({0.5f, 1.0f, 1.0f}).valid());
  imageGeomPtr->setUnits(IGeometry::LengthUnit::Micrometer);
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {k_DimZ, k_DimY, k_DimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* scalarPtr = unit_test::CreateTestDataArray<uint8>(dataStructure, "Scalar", {k_DimZ, k_DimY, k_DimX}, {1}, cellAmPtr->getId());
  auto& scalarStore = scalarPtr->getDataStoreRef();
  for(usize i = 0; i < scalarStore.getNumberOfTuples(); i++)
  {
    scalarStore[i] = static_cast<uint8>(i % 251);
  }

  // XZ plane: sliceCount = dimY, so 4 files are written; only slice 0 is read back below.
  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  {
    WriteImageFilter filter;
    Arguments args;
    args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.png"));
    args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XZPlane));
    args.insertOrAssign(WriteImageFilter::k_AddScaleBar_Key, std::make_any<bool>(true));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);

  DataStructure readDs;
  ReadImageStackFilter reader;
  GeneratedFileListParameter::ValueType fileList;
  fileList.input_path = tempDir.path().string();
  fileList.start_index = 0;
  fileList.end_index = 0;
  fileList.increment_index = 1;
  fileList.file_extension = ".png";
  fileList.file_prefix = "slice_";
  fileList.file_suffix = "";
  fileList.padding_digits = 3;
  fileList.ordering = GeneratedFileListParameter::OrderingType::LowToHigh;

  Arguments rArgs;
  rArgs.insertOrAssign(ReadImageStackFilter::k_InputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileList));
  rArgs.insertOrAssign(ReadImageStackFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ReadGeom"})));
  auto readerPreflight = reader.preflight(readDs, rArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readerPreflight.output_actions);
  auto readerExecute = reader.execute(readDs, rArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readerExecute.result);

  // XZ slice: W = dimX = 8, H = dimZ = 6; band = max(24, llround(0.08*6)=0) = 24 -> 8 x 30.
  REQUIRE_NOTHROW(readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
  const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  SizeVec3Type readDims = readGeom.getDimensions();
  REQUIRE(readDims[0] == k_DimX);
  REQUIRE(readDims[1] == k_DimZ + 24);

  const auto* readArrayPtr = dynamic_cast<const UInt8ArrayType*>(readGeom.getCellData()->begin()->second.get());
  REQUIRE(readArrayPtr != nullptr);
  const auto& readStore = readArrayPtr->getDataStoreRef();
  REQUIRE(readArrayPtr->getNumberOfComponents() == 3);

  // First band row (row = dimZ, col = 0) is white background.
  REQUIRE(readStore.getValue((k_DimZ * k_DimX + 0) * 3) == 255);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: groups YZ slices into caller-owned bounded extents", "[NXImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize k_DimX = 8;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({k_DimX, k_DimY, k_DimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {k_DimZ, k_DimY, k_DimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  auto countingStore = std::make_shared<ExtentCountingDataStore<uint8>>(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
  auto* scalarPtr = DataArray<uint8>::Create(dataStructure, scalarPath.getTargetName(), countingStore, cellAmPtr->getId());
  REQUIRE(scalarPtr != nullptr);
  for(usize tupleIdx = 0; tupleIdx < countingStore->getNumberOfTuples(); ++tupleIdx)
  {
    countingStore->setValue(tupleIdx, static_cast<uint8>(tupleIdx % 251));
  }

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_YZPlane));

  ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(48);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CAPTURE(countingStore->returnedExtentReadCount(), countingStore->callerProvidedExtentReadCount(), countingStore->maxCallerProvidedExtentValues());
  if(scenario == unit_test::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    REQUIRE(countingStore->returnedExtentReadCount() == k_DimX);
    REQUIRE(countingStore->callerProvidedExtentReadCount() == 0);
    REQUIRE(countingStore->maxCallerProvidedExtentValues() == 0);
  }
  else
  {
    REQUIRE(countingStore->returnedExtentReadCount() == 0);
    REQUIRE(countingStore->callerProvidedExtentReadCount() == 4);
    REQUIRE(countingStore->maxCallerProvidedExtentValues() == 48);
  }

  usize outputFileCount = 0;
  for([[maybe_unused]] const fs::directory_entry& entry : fs::directory_iterator(tempDir.path()))
  {
    outputFileCount++;
  }
  REQUIRE(outputFileCount == k_DimX);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: groups color and mask YZ slices into caller-owned bounded extents", "[NXImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize k_DimX = 8;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");
  const DataPath maskPath = geomPath.createChildPath("CellData").createChildPath("Mask");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({k_DimX, k_DimY, k_DimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {k_DimZ, k_DimY, k_DimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  auto scalarStore = std::make_shared<ExtentCountingDataStore<float32>>(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0F);
  auto maskStore = std::make_shared<ExtentCountingDataStore<uint8>>(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{1});
  REQUIRE(DataArray<float32>::Create(dataStructure, scalarPath.getTargetName(), scalarStore, cellAmPtr->getId()) != nullptr);
  REQUIRE(DataArray<uint8>::Create(dataStructure, maskPath.getTargetName(), maskStore, cellAmPtr->getId()) != nullptr);
  for(usize tupleIdx = 0; tupleIdx < scalarStore->getNumberOfTuples(); ++tupleIdx)
  {
    scalarStore->setValue(tupleIdx, static_cast<float32>(tupleIdx));
    maskStore->setValue(tupleIdx, tupleIdx % 5 == 0 ? uint8{0} : uint8{1});
  }

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_YZPlane));
  args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(color_table_utilities::GetDefaultRGBPresetName()));
  args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{10, 20, 30}));

  ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(240);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CAPTURE(scalarStore->returnedExtentReadCount(), scalarStore->callerProvidedExtentReadCount(), scalarStore->maxCallerProvidedExtentValues(), maskStore->returnedExtentReadCount(),
          maskStore->callerProvidedExtentReadCount(), maskStore->maxCallerProvidedExtentValues());
  if(scenario == unit_test::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    REQUIRE(scalarStore->returnedExtentReadCount() == k_DimX);
    REQUIRE(scalarStore->callerProvidedExtentReadCount() == 0);
    REQUIRE(scalarStore->maxCallerProvidedExtentValues() == 0);
    REQUIRE(maskStore->returnedExtentReadCount() == k_DimX);
    REQUIRE(maskStore->callerProvidedExtentReadCount() == 0);
    REQUIRE(maskStore->maxCallerProvidedExtentValues() == 0);
  }
  else
  {
    REQUIRE(scalarStore->returnedExtentReadCount() == 0);
    REQUIRE(scalarStore->callerProvidedExtentReadCount() == 4);
    REQUIRE(scalarStore->maxCallerProvidedExtentValues() == 48);
    REQUIRE(maskStore->returnedExtentReadCount() == 0);
    REQUIRE(maskStore->callerProvidedExtentReadCount() == 4);
    REQUIRE(maskStore->maxCallerProvidedExtentValues() == 48);
  }

  usize outputFileCount = 0;
  for([[maybe_unused]] const fs::directory_entry& entry : fs::directory_iterator(tempDir.path()))
  {
    outputFileCount++;
  }
  REQUIRE(outputFileCount == k_DimX);
}

TEST_CASE("NXImageProcessing::WriteImageFilter: reads direct XY slices into separate caller-owned extents", "[NXImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize k_DimX = 8;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({k_DimX, k_DimY, k_DimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {k_DimZ, k_DimY, k_DimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto countingStore = std::make_shared<ExtentCountingDataStore<uint8>>(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{7});
  REQUIRE(DataArray<uint8>::Create(dataStructure, scalarPath.getTargetName(), countingStore, cellAmPtr->getId()) != nullptr);

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));

  ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_DimX * k_DimY * k_DimZ);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  if(scenario == unit_test::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    REQUIRE(countingStore->returnedExtentReadCount() == k_DimZ);
    REQUIRE(countingStore->callerProvidedExtentReadCount() == 0);
  }
  else
  {
    REQUIRE(countingStore->returnedExtentReadCount() == 0);
    REQUIRE(countingStore->callerProvidedExtentReadCount() == k_DimZ);
    REQUIRE(countingStore->maxCallerProvidedExtentValues() == k_DimX * k_DimY);
  }
}

TEST_CASE("NXImageProcessing::WriteImageFilter: reads color and mask XY slices into separate caller-owned extents", "[NXImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize k_DimX = 8;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");
  const DataPath maskPath = geomPath.createChildPath("CellData").createChildPath("Mask");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({k_DimX, k_DimY, k_DimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {k_DimZ, k_DimY, k_DimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto scalarStore = std::make_shared<ExtentCountingDataStore<float32>>(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 2.0F);
  auto maskStore = std::make_shared<ExtentCountingDataStore<uint8>>(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{1});
  REQUIRE(DataArray<float32>::Create(dataStructure, scalarPath.getTargetName(), scalarStore, cellAmPtr->getId()) != nullptr);
  REQUIRE(DataArray<uint8>::Create(dataStructure, maskPath.getTargetName(), maskStore, cellAmPtr->getId()) != nullptr);

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));
  args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(color_table_utilities::GetDefaultRGBPresetName()));
  args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));

  ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_DimX * k_DimY * k_DimZ * (sizeof(float32) + sizeof(uint8)));
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  if(scenario == unit_test::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    REQUIRE(scalarStore->returnedExtentReadCount() == k_DimZ);
    REQUIRE(maskStore->returnedExtentReadCount() == k_DimZ);
  }
  else
  {
    REQUIRE(scalarStore->returnedExtentReadCount() == 0);
    REQUIRE(scalarStore->callerProvidedExtentReadCount() == k_DimZ);
    REQUIRE(scalarStore->maxCallerProvidedExtentValues() == k_DimX * k_DimY);
    REQUIRE(maskStore->returnedExtentReadCount() == 0);
    REQUIRE(maskStore->callerProvidedExtentReadCount() == k_DimZ);
    REQUIRE(maskStore->maxCallerProvidedExtentValues() == k_DimX * k_DimY);
  }
}

TEST_CASE("NXImageProcessing::WriteImageFilter: rejects a source-group grant smaller than one slice", "[NXImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize k_DimX = 8;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({k_DimX, k_DimY, k_DimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {k_DimZ, k_DimY, k_DimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  unit_test::CreateTestDataArray<uint8>(dataStructure, scalarPath.getTargetName(), {k_DimZ, k_DimY, k_DimX}, {1}, cellAmPtr->getId());

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_YZPlane));

  ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(1);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  if(scenario == unit_test::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors().front().code == -27026);
  }
}
