#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include "ImageProcessing/Filters/Algorithms/WriteImageColorUtilities.hpp"
#include "ImageProcessing/Filters/ReadImageFilter.hpp"
#include "ImageProcessing/Filters/ReadImageStackFilter.hpp"
#include "ImageProcessing/Filters/ReadMhaFileFilter.hpp"
#include "ImageProcessing/Filters/WriteImageFilter.hpp"
#include "ImageProcessing/ImageProcessing_test_dirs.hpp"
#include "SimplnxCore/Filters/CreateColorMapFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CreateColorMapParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/ImageIO/IImageIO.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

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

void validateOutputFiles(size_t numImages, uint64 offset, const std::string& tempDirName, const fs::path& tempDirPath)
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
void readBackSingleImage(DataStructure& readDs, const fs::path& imagePath)
{
  ReadImageFilter reader;
  Arguments rArgs;
  rArgs.insertOrAssign(ReadImageFilter::k_FileName_Key, std::make_any<fs::path>(imagePath));
  rArgs.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ReadGeom"})));
  auto readerPreflight = reader.preflight(readDs, rArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readerPreflight.outputActions);
  auto readerExecute = reader.execute(readDs, rArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readerExecute.result);
}

// Writes one scalar slice through the color-table path.
// The test compares the RGB result to an independent Create Color Map result.
template <typename T>
void RunColorRoundtripForType(UnitTest::AlgorithmTestScenario scenario)
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

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
  ScopedTempDir inlineDir(k_BinaryTestOutputDir);
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
    SIMPLNX_RESULT_REQUIRE_VALID(ccmPreflight.outputActions);
    // Store the result because SIMPLNX_RESULT_REQUIRE_VALID evaluates its argument more than once.
    // Repeated execution fails because the RGB output already exists.
    auto ccmExecute = ccm.execute(dataStructure, ccmArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(ccmExecute.result);
  }

  const DataPath rgbPath = geomPath.createChildPath("CellData").createChildPath("RGB");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(rgbPath));
  const auto& expectedRgbStore = dataStructure.getDataRefAs<UInt8Array>(rgbPath).getDataStoreRef();

  // Read the inline-written slice back and compare pixel RGB to the Create Color Map result.
  DataStructure readDs;
  readBackSingleImage(readDs, inlineDir.path() / "slice.tif");

  // Locate the single RGB image-data array created by the reader and compare tuple-by-tuple.
  const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  const auto& readCellAm = readGeom.getCellData();
  const auto* readArrayPtr = dynamic_cast<const UInt8Array*>(readCellAm->begin()->second.get());
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  UnitTest::CreateTestDataArray<T>(dataStructure, "Scalar", {1, 4, 4}, {numComponents}, cellAmPtr->getId());

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(k_BinaryTestOutputDir / fileName));
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

TEST_CASE("ImageProcessing::WriteImageFilter: nonfinite policy is explicit and defaults to Error", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageSafeRed]")
{
  UnitTest::LoadPlugins();
  const WriteImageFilter filter;
  const Parameters parameters = filter.parameters();
  const std::string policyKey = "nonfinite_input_handling_index";

  // Inspect the public schema without sending nonfinite values through the baseline writer.
  REQUIRE(parameters.contains(policyKey));
  const auto* policyPtr = dynamic_cast<const ChoicesParameter*>(parameters.at(policyKey).get());
  REQUIRE(policyPtr != nullptr);
  REQUIRE(policyPtr->humanName() == "Nonfinite Input Handling");
  REQUIRE(policyPtr->choices() == ChoicesParameter::Choices{"Error", "Warn and Continue"});
  REQUIRE(policyPtr->defaultIndex() == 0);
  const Arguments arguments = filter.getDefaultArguments();
  REQUIRE(arguments.value<ChoicesParameter::ValueType>(policyKey) == 0);
}

TEST_CASE("ImageProcessing::WriteImageFilter: nonfinite policy is active only for color-table output", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageSafeRed]")
{
  UnitTest::LoadPlugins();
  const WriteImageFilter filter;
  const Parameters parameters = filter.parameters();
  const std::string policyKey = "nonfinite_input_handling_index";

  REQUIRE(parameters.contains(policyKey));
  REQUIRE(parameters.contains(WriteImageFilter::k_CreateColorTable_Key));
  Arguments arguments = filter.getDefaultArguments();
  REQUIRE_FALSE(arguments.value<bool>(WriteImageFilter::k_CreateColorTable_Key));
  REQUIRE_FALSE(parameters.isParameterActive(policyKey, arguments));
  arguments.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
  REQUIRE(parameters.isParameterActive(policyKey, arguments));
  arguments.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(false));
  REQUIRE_FALSE(parameters.isParameterActive(policyKey, arguments));
}

namespace
{
template <typename T, usize Size>
DataPath createWriteImageNumericInput(DataStructure& dataStructure, const std::array<T, Size>& values, const ShapeType& shape = {1, 1, Size})
{
  auto* geometryPtr = ImageGeom::Create(dataStructure, "NumericImage");
  REQUIRE(geometryPtr != nullptr);
  geometryPtr->setDimensions({shape.at(2), shape.at(1), shape.at(0)});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "CellData", shape, geometryPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  geometryPtr->setCellData(*cellsPtr);
  auto* arrayPtr = UnitTest::CreateTestDataArray<T>(dataStructure, "Input", shape, {1}, cellsPtr->getId());
  REQUIRE(arrayPtr != nullptr);
  REQUIRE(arrayPtr->getDataType() == GetDataType<T>());
  REQUIRE(arrayPtr->getTupleShape() == shape);
  REQUIRE(arrayPtr->getNumberOfTuples() == Size);
  for(usize index = 0; index < Size; ++index)
  {
    arrayPtr->getDataStoreRef().setValue(index, values.at(index));
  }
  return DataPath({"NumericImage", "CellData", "Input"});
}

template <typename T, usize Size>
DataPath createWriteImageNumericMask(DataStructure& dataStructure, const std::array<T, Size>& values)
{
  const DataPath cellsPath({"NumericImage", "CellData"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(cellsPath));
  const auto& cells = dataStructure.getDataRefAs<AttributeMatrix>(cellsPath);
  auto* maskPtr = UnitTest::CreateTestDataArray<T>(dataStructure, "Mask", cells.getShape(), {1}, cells.getId());
  REQUIRE(maskPtr != nullptr);
  REQUIRE(maskPtr->getNumberOfTuples() == Size);
  for(usize index = 0; index < Size; ++index)
  {
    maskPtr->getDataStoreRef().setValue(index, values.at(index));
  }
  return cellsPath.createChildPath("Mask");
}

Arguments writeImageNumericArguments(const fs::path& output, uint64 policy = 0, const std::string& preset = "Grayscale", bool useMask = false)
{
  const WriteImageFilter filter;
  Arguments arguments = filter.getDefaultArguments();
  arguments.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({"NumericImage"})));
  arguments.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(DataPath({"NumericImage", "CellData", "Input"})));
  arguments.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(output));
  arguments.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
  arguments.insertOrAssign(WriteImageFilter::k_NonfiniteInputHandling_Key, std::make_any<ChoicesParameter::ValueType>(policy));
  arguments.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(preset));
  arguments.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  arguments.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"NumericImage", "CellData", "Mask"})));
  arguments.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{10, 20, 30}));
  return arguments;
}

std::vector<uint8> readWriteImageNumericPng(const fs::path& path, usize width, usize height)
{
  auto imageIoResult = CreateImageIO(path);
  SIMPLNX_RESULT_REQUIRE_VALID(imageIoResult);
  REQUIRE(imageIoResult.value() != nullptr);
  auto metadata = imageIoResult.value()->readMetadata(path);
  SIMPLNX_RESULT_REQUIRE_VALID(metadata);
  REQUIRE(metadata.value().width == width);
  REQUIRE(metadata.value().height == height);
  REQUIRE(metadata.value().dataType == DataType::uint8);
  REQUIRE(metadata.value().numComponents == 3);
  REQUIRE(metadata.value().numPages == 1);
  std::vector<uint8> bytes(width * height * 3);
  auto readResult = imageIoResult.value()->readPixelData(path, std::span<uint8>(bytes));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  return bytes;
}

template <typename T, usize Size>
void requireWriteImageGray(UnitTest::AlgorithmTestScope& scope, const std::array<T, Size>& values, const std::array<uint8, Size>& expected)
{
  DataStructure dataStructure;
  const DataPath inputPath = createWriteImageNumericInput(dataStructure, values);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, writeImageNumericArguments(output.path() / "numeric.png"));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  const auto bytes = readWriteImageNumericPng(output.path() / "numeric.png", Size, 1);
  for(usize index = 0; index < Size; ++index)
  {
    for(usize channel = 0; channel < 3; ++channel)
    {
      REQUIRE(bytes.at((index * 3) + channel) == expected.at(index));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

std::vector<char> readWriteImageSentinel(const fs::path& path)
{
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.is_open());
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

template <typename T>
class WriteImageReadProbeStore : public DataStore<T>
{
public:
  WriteImageReadProbeStore(const ShapeType& shape, bool failFlat, bool failExtent)
  : DataStore<T>(shape, ShapeType{1}, T{})
  , m_FailFlat(failFlat)
  , m_FailExtent(failExtent)
  {
  }
  ~WriteImageReadProbeStore() noexcept override = default;
  WriteImageReadProbeStore(const WriteImageReadProbeStore&) = delete;
  WriteImageReadProbeStore(WriteImageReadProbeStore&&) = delete;
  WriteImageReadProbeStore& operator=(const WriteImageReadProbeStore&) = delete;
  WriteImageReadProbeStore& operator=(WriteImageReadProbeStore&&) = delete;
  Result<> copyIntoBuffer(usize offset, nonstd::span<T> values) const override
  {
    m_FlatReadCount++;
    m_MaximumReadBytes = std::max(m_MaximumReadBytes, values.size() * sizeof(T));
    if(m_FailFlat)
    {
      return MakeErrorResult(-27901, "Controlled Write Image page read failure.");
    }
    auto result = DataStore<T>::copyIntoBuffer(offset, values);
    if(result.valid())
    {
      result.warnings().push_back({-27903, "Controlled Write Image page read warning."});
    }
    return result;
  }
  Result<std::vector<T>> readExtent(const Extent& extent) const override
  {
    if(m_FailExtent)
    {
      return MakeErrorResult<std::vector<T>>(-27902, "Controlled Write Image slice read failure.");
    }
    return DataStore<T>::readExtent(extent);
  }
  Result<> readExtentIntoBuffer(const Extent& extent, nonstd::span<T> values) const override
  {
    if(m_FailExtent)
    {
      return MakeErrorResult(-27902, "Controlled Write Image slice read failure.");
    }
    return DataStore<T>::readExtentIntoBuffer(extent, values);
  }
  usize flatReadCount() const
  {
    return m_FlatReadCount;
  }
  usize maximumReadBytes() const
  {
    return m_MaximumReadBytes;
  }

private:
  bool m_FailFlat = false;
  bool m_FailExtent = false;
  mutable usize m_FlatReadCount = 0;
  mutable usize m_MaximumReadBytes = 0;
};
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::WriteImageFilter: native endpoints decode to literal RGB", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64, float32, float64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  requireWriteImageGray(scope, std::array<TestType, 2>{std::numeric_limits<TestType>::lowest(), std::numeric_limits<TestType>::max()}, std::array<uint8, 2>{0, 255});
}

TEST_CASE("ImageProcessing::WriteImageFilter: wide and tiny interiors decode without native range collapse", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  const std::array<uint8, 3> expected{0, 127, 255};
  requireWriteImageGray(scope, std::array<float32, 3>{-std::numeric_limits<float32>::max(), 0, std::numeric_limits<float32>::max()}, expected);
  requireWriteImageGray(scope, std::array<float64, 3>{-std::numeric_limits<float64>::max(), 0, std::numeric_limits<float64>::max()}, expected);
  constexpr int64 minimum = std::numeric_limits<int64>::min();
  constexpr int64 maximum = std::numeric_limits<int64>::max();
  constexpr uint64 unsignedMaximum = std::numeric_limits<uint64>::max();
  requireWriteImageGray(scope, std::array<int64, 3>{minimum, minimum + 1, minimum + 2}, expected);
  requireWriteImageGray(scope, std::array<int64, 3>{maximum - 2, maximum - 1, maximum}, expected);
  requireWriteImageGray(scope, std::array<uint64, 3>{unsignedMaximum - 2, unsignedMaximum - 1, unsignedMaximum}, expected);
  requireWriteImageGray(scope, std::array<float64, 3>{0, std::numeric_limits<float64>::min(), 2 * std::numeric_limits<float64>::min()}, expected);
  requireWriteImageGray(scope, std::array<float32, 4>{0, 1, 2, 3}, std::array<uint8, 4>{0, 85, 170, 255});
}

TEST_CASE("ImageProcessing::WriteImageFilter: subnormal double interior has a distinct runtime witness", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  const float64 tiny = std::numeric_limits<float64>::denorm_min();
  const std::array<float64, 3> values{tiny, tiny * 2, tiny * 3};
  REQUIRE(std::fpclassify(values.at(0)) == FP_SUBNORMAL);
  REQUIRE(std::fpclassify(values.at(1)) == FP_SUBNORMAL);
  REQUIRE(std::fpclassify(values.at(2)) == FP_SUBNORMAL);
  REQUIRE(values.at(0) < values.at(1));
  REQUIRE(values.at(1) < values.at(2));
  requireWriteImageGray(scope, values, std::array<uint8, 3>{0, 127, 255});
}

TEST_CASE("ImageProcessing::WriteImageFilter: mask first nonfinite and finite masked range decode literally", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool booleanMask = GENERATE(false, true);
  const uint64 policy = GENERATE(uint64{0}, uint64{1});
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  createWriteImageNumericInput(dataStructure,
                               std::array<float64, 7>{0, 3, 6, 9, std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity(), -std::numeric_limits<float64>::infinity()});
  if(booleanMask)
  {
    createWriteImageNumericMask(dataStructure, std::array<bool, 7>{false, true, true, true, false, false, false});
  }
  else
  {
    createWriteImageNumericMask(dataStructure, std::array<uint8, 7>{0, 2, 1, 1, 0, 0, 0});
  }
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, writeImageNumericArguments(output.path() / "mask.png", policy, "Grayscale", true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  const std::vector<uint8> expected{10, 20, 30, 85, 85, 85, 170, 170, 170, 255, 255, 255, 10, 20, 30, 10, 20, 30, 10, 20, 30};
  REQUIRE(readWriteImageNumericPng(output.path() / "mask.png", 7, 1) == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: Warn returns one count and the first actual control color", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool useMask = GENERATE(false, true);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  createWriteImageNumericInput(dataStructure,
                               std::array<float64, 5>{std::numeric_limits<float64>::quiet_NaN(), 0, std::numeric_limits<float64>::infinity(), 2, -std::numeric_limits<float64>::infinity()});
  createWriteImageNumericMask(dataStructure, std::array<uint8, 5>{useMask ? uint8{0} : uint8{1}, 1, 1, 1, 1});
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, writeImageNumericArguments(output.path() / "warn.png", 1, "X Ray", useMask));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings().front().code == -27031);
  REQUIRE(result.result.warnings().front().message.find(useMask ? "2 unmasked" : "3 unmasked") != std::string::npos);
  std::vector<uint8> expected{255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255};
  if(useMask)
  {
    expected.at(0) = 10;
    expected.at(1) = 20;
    expected.at(2) = 30;
  }
  REQUIRE(readWriteImageNumericPng(output.path() / "warn.png", 5, 1) == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: late strict nonfinite preserves every existing slice byte", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const usize invalidKind = GENERATE(usize{0}, usize{1}, usize{2});
  UnitTest::AlgorithmTestScope scope(scenario);
  const std::array<float64, 3> invalidValues{std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity(), -std::numeric_limits<float64>::infinity()};
  DataStructure dataStructure;
  createWriteImageNumericInput(dataStructure, std::array<float64, 8>{0, 1, 2, 3, 4, 5, 6, invalidValues.at(invalidKind)}, {2, 2, 2});
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const std::array<std::string, 2> names{"slice_000.png", "slice_001.png"};
  const std::array<std::string, 2> sentinels{"first-existing-slice", "second-existing-slice"};
  for(usize index = 0; index < names.size(); ++index)
  {
    std::ofstream file(output.path() / names.at(index), std::ios::binary);
    REQUIRE(file.is_open());
    file << sentinels.at(index);
  }
  const WriteImageFilter filter;
  const auto arguments = writeImageNumericArguments(output.path() / "slice.png");
  const auto preflight = filter.preflight(dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = scope.executeFilter(filter, dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().front().code == -27027);
  for(usize index = 0; index < names.size(); ++index)
  {
    const auto bytes = readWriteImageSentinel(output.path() / names.at(index));
    REQUIRE(std::string(bytes.begin(), bytes.end()) == sentinels.at(index));
  }
  REQUIRE(std::distance(fs::directory_iterator(output.path()), fs::directory_iterator{}) == 2);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: no finite color range errors in either mode", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const uint64 policy = GENERATE(uint64{0}, uint64{1});
  const bool allMasked = GENERATE(false, true);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  createWriteImageNumericInput(dataStructure, std::array<float64, 3>{std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity(), -std::numeric_limits<float64>::infinity()});
  createWriteImageNumericMask(dataStructure, std::array<bool, 3>{false, false, false});
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, writeImageNumericArguments(output.path() / "none.png", policy, "Grayscale", allMasked));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().front().code == ((!allMasked && policy == 0) ? -27027 : -27028));
  REQUIRE(result.result.warnings().size() == ((!allMasked && policy == 1) ? 1 : 0));
  REQUIRE(fs::is_empty(output.path()));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: finite constants and all finite masked values stay valid", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool allMasked = GENERATE(false, true);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  createWriteImageNumericInput(dataStructure, std::array<float64, 3>{5, 5, 5});
  createWriteImageNumericMask(dataStructure, std::array<bool, 3>{!allMasked, false, !allMasked});
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, writeImageNumericArguments(output.path() / "constant.png", 0, "X Ray", true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  const std::vector<uint8> expected = allMasked ? std::vector<uint8>{10, 20, 30, 10, 20, 30, 10, 20, 30} : std::vector<uint8>{255, 255, 255, 10, 20, 30, 255, 255, 255};
  REQUIRE(readWriteImageNumericPng(output.path() / "constant.png", 3, 1) == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: policy output retains literal layouts in every plane", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const uint64 plane = GENERATE(uint64{0}, uint64{1}, uint64{2});
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  createWriteImageNumericInput(dataStructure, std::array<float64, 8>{0, 3, 6, 9, 0, 3, 6, 9}, {2, 2, 2});
  const ScopedTempDir output(k_BinaryTestOutputDir);
  auto arguments = writeImageNumericArguments(output.path() / "slice.png");
  arguments.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(plane));
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const std::array<std::array<uint8, 8>, 3> expected{{{0, 85, 170, 255, 0, 85, 170, 255}, {0, 85, 0, 85, 170, 255, 170, 255}, {0, 170, 0, 170, 85, 255, 85, 255}}};
  for(usize slice = 0; slice < 2; ++slice)
  {
    const auto bytes = readWriteImageNumericPng(output.path() / (slice == 0 ? "slice_000.png" : "slice_001.png"), 2, 2);
    for(usize pixel = 0; pixel < 4; ++pixel)
    {
      for(usize channel = 0; channel < 3; ++channel)
      {
        REQUIRE(bytes.at((pixel * 3) + channel) == expected.at(plane).at((slice * 4) + pixel));
      }
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::WriteImageFilter: raw floating pixels ignore color policy and mask", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]", float32, float64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const uint64 policy = GENERATE(uint64{0}, uint64{1});
  const bool useMask = GENERATE(false, true);
  UnitTest::AlgorithmTestScope scope(scenario);
  const std::array<TestType, 5> values{TestType{-2}, TestType{0}, std::numeric_limits<TestType>::quiet_NaN(), std::numeric_limits<TestType>::infinity(), -std::numeric_limits<TestType>::infinity()};
  DataStructure dataStructure;
  createWriteImageNumericInput(dataStructure, values);
  createWriteImageNumericMask(dataStructure, std::array<bool, 5>{false, false, false, false, false});
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const fs::path path = output.path() / (std::is_same_v<TestType, float32> ? "raw.tif" : "raw.mha");
  auto arguments = writeImageNumericArguments(path, policy, "Grayscale", useMask);
  arguments.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(false));
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  std::array<TestType, 5> actual{};
  if constexpr(std::is_same_v<TestType, float32>)
  {
    const auto imageIoResult = CreateImageIO(path);
    SIMPLNX_RESULT_REQUIRE_VALID(imageIoResult);
    auto metadata = imageIoResult.value()->readMetadata(path);
    SIMPLNX_RESULT_REQUIRE_VALID(metadata);
    REQUIRE(metadata.value().dataType == DataType::float32);
    REQUIRE(metadata.value().numComponents == 1);
    std::array<uint8, sizeof(TestType) * 5> bytes{};
    const auto readResult = imageIoResult.value()->readPixelData(path, std::span<uint8>(bytes));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    std::memcpy(actual.data(), bytes.data(), bytes.size());
  }
  else
  {
    const ReadMhaFileFilter reader;
    auto readerArguments = reader.getDefaultArguments();
    readerArguments.insertOrAssign(ReadMhaFileFilter::k_InputFilePath_Key, std::make_any<fs::path>(path));
    readerArguments.insertOrAssign(ReadMhaFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"RawRead"})));
    readerArguments.insertOrAssign(ReadMhaFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("Cells"));
    readerArguments.insertOrAssign(ReadMhaFileFilter::k_ImageDataArrayName_Key, std::make_any<std::string>("Pixels"));
    DataStructure readStructure;
    const auto readResult = reader.execute(readStructure, readerArguments);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult.result);
    const DataPath readPath({"RawRead", "Cells", "Pixels"});
    REQUIRE_NOTHROW(readStructure.getDataRefAs<IDataArray>(readPath));
    REQUIRE(readStructure.getDataRefAs<IDataArray>(readPath).getDataType() == DataType::float64);
    REQUIRE_NOTHROW(readStructure.getDataRefAs<Float64Array>(readPath));
    const auto copyResult = readStructure.getDataRefAs<Float64Array>(readPath).getDataStoreRef().copyIntoBuffer(0, nonstd::span<float64>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
    UnitTest::CheckArraysInheritTupleDims(readStructure);
  }
  REQUIRE(actual.at(0) == -2);
  REQUIRE(actual.at(1) == 0);
  REQUIRE(std::isnan(actual.at(2)));
  REQUIRE(std::isinf(actual.at(3)));
  REQUIRE_FALSE(std::signbit(actual.at(3)));
  REQUIRE(std::isinf(actual.at(4)));
  REQUIRE(std::signbit(actual.at(4)));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: cancellation during color scan opens no slice", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  createWriteImageNumericInput(dataStructure, std::array<float64, 3>{0, 1, 2});
  const ScopedTempDir output(k_BinaryTestOutputDir);
  std::atomic_bool cancel = false;
  const IFilter::MessageHandler handler{[&cancel](const IFilter::Message& message) {
    if(message.message.find("Computing color range") != std::string::npos)
    {
      cancel = true;
    }
  }};
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, writeImageNumericArguments(output.path() / "cancel.png"), nullptr, handler, cancel);
  REQUIRE(cancel.load());
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().front().code == -1);
  REQUIRE(fs::is_empty(output.path()));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: color size and palette guards use actionable results", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto empty = WriteImageColor::ValidateSize<float64>(0, 1, 1, false);
  SIMPLNX_RESULT_REQUIRE_INVALID(empty);
  REQUIRE(empty.errors().front().code == -27030);
  const auto large = WriteImageColor::ValidateSize<float64>(std::numeric_limits<usize>::max(), 1, 1, true);
  SIMPLNX_RESULT_REQUIRE_INVALID(large);
  REQUIRE(large.errors().front().code == -27032);
  const auto invalidPalette = WriteImageColor::ValidatePalette({0, 0, 0, 0, 1, 1, 1, 1}, {1, 0});
  SIMPLNX_RESULT_REQUIRE_INVALID(invalidPalette);
  REQUIRE(invalidPalette.errors().front().code == -27029);
  const auto validPalette = WriteImageColor::ValidatePalette({0, 0, 0, 0, 1, 1, 1, 1}, {0, 1});
  SIMPLNX_RESULT_REQUIRE_VALID(validPalette);
}

TEST_CASE("ImageProcessing::WriteImageFilter: page and slice IO failures preserve diagnostics and warnings", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const usize failure = GENERATE(usize{0}, usize{1}, usize{2});
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = createWriteImageNumericInput(dataStructure, std::array<float64, 3>{0, std::numeric_limits<float64>::quiet_NaN(), 2});
  const DataPath maskPath = createWriteImageNumericMask(dataStructure, std::array<uint8, 3>{1, 1, 1});
  if(failure != 1)
  {
    auto store = std::make_shared<WriteImageReadProbeStore<float64>>(ShapeType{1, 1, 3}, failure == 0, failure == 2);
    store->setValue(0, 0);
    store->setValue(1, std::numeric_limits<float64>::quiet_NaN());
    store->setValue(2, 2);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(inputPath));
    const auto replace = dataStructure.getDataRefAs<Float64Array>(inputPath).setDataStore(store);
    SIMPLNX_RESULT_REQUIRE_VALID(replace);
  }
  else
  {
    auto store = std::make_shared<WriteImageReadProbeStore<uint8>>(ShapeType{1, 1, 3}, true, false);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(maskPath));
    const auto replace = dataStructure.getDataRefAs<UInt8Array>(maskPath).setDataStore(store);
    SIMPLNX_RESULT_REQUIRE_VALID(replace);
  }
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, writeImageNumericArguments(output.path() / "failure.png", 1, "Grayscale", true));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().front().code == (failure == 2 ? -27902 : -27901));
  if(failure == 2)
  {
    REQUIRE(result.result.warnings().size() == 2);
    REQUIRE(result.result.warnings().at(0).code == -27903);
    REQUIRE(result.result.warnings().at(1).code == -27031);
  }
  REQUIRE(fs::is_empty(output.path()));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: late nonfinite scan uses bounded scalar and mask pages", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool useMask = GENERATE(false, true);
  UnitTest::AlgorithmTestScope scope(scenario);
  constexpr usize pageByteTarget = usize{1024} * usize{1024};
  const usize pageValues = pageByteTarget / (sizeof(float64) + (useMask ? sizeof(uint8) : 0));
  const ShapeType shape{2, 1, pageValues};
  DataStructure dataStructure;
  auto* geometryPtr = ImageGeom::Create(dataStructure, "NumericImage");
  REQUIRE(geometryPtr != nullptr);
  geometryPtr->setDimensions({pageValues, 1, 2});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "CellData", shape, geometryPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  geometryPtr->setCellData(*cellsPtr);
  auto store = std::make_shared<WriteImageReadProbeStore<float64>>(shape, false, false);
  store->setValue(pageValues, std::numeric_limits<float64>::quiet_NaN());
  REQUIRE(Float64Array::Create(dataStructure, "Input", store, cellsPtr->getId()) != nullptr);
  std::shared_ptr<WriteImageReadProbeStore<uint8>> maskStore;
  if(useMask)
  {
    maskStore = std::make_shared<WriteImageReadProbeStore<uint8>>(shape, false, false);
    for(usize index = 0; index < 2 * pageValues; ++index)
    {
      maskStore->setValue(index, 1);
    }
    REQUIRE(UInt8Array::Create(dataStructure, "Mask", maskStore, cellsPtr->getId()) != nullptr);
  }
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const WriteImageFilter filter;
  const auto result = scope.executeFilter(filter, dataStructure, writeImageNumericArguments(output.path() / "page.png", 0, "Grayscale", useMask));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().front().code == -27027);
  REQUIRE(store->flatReadCount() == 2);
  REQUIRE(store->maximumReadBytes() == pageValues * sizeof(float64));
  if(useMask)
  {
    REQUIRE(maskStore->flatReadCount() == 2);
    REQUIRE(maskStore->maximumReadBytes() == pageValues * sizeof(uint8));
    REQUIRE((store->maximumReadBytes() + maskStore->maximumReadBytes()) <= pageByteTarget);
  }
  REQUIRE(result.result.warnings().size() == (useMask ? 4 : 2));
  REQUIRE(fs::is_empty(output.path()));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: later commit failure keeps earlier slice and policy warning", "[ImageProcessing][Issue1874][WriteImageFilter][WriteImageNumeric]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  createWriteImageNumericInput(dataStructure, std::array<float64, 4>{0, std::numeric_limits<float64>::quiet_NaN(), 2, 3}, {2, 1, 2});
  const ScopedTempDir output(k_BinaryTestOutputDir);
  const fs::path blockedTarget = output.path() / "slice_001.png";
  REQUIRE(fs::create_directory(blockedTarget));
  {
    std::ofstream sentinel(blockedTarget / "keep.txt", std::ios::binary);
    REQUIRE(sentinel.is_open());
    sentinel << "keep";
  }
  const WriteImageFilter filter;
  const auto arguments = writeImageNumericArguments(output.path() / "slice.png", 1);
  const auto preflight = filter.preflight(dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = scope.executeFilter(filter, dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().front().code == -15782);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings().front().code == -27031);
  REQUIRE(fs::is_regular_file(output.path() / "slice_000.png"));
  const auto first = readWriteImageNumericPng(output.path() / "slice_000.png", 2, 1);
  REQUIRE(first == std::vector<uint8>{0, 0, 0, 0, 0, 0});
  const auto sentinel = readWriteImageSentinel(blockedTarget / "keep.txt");
  REQUIRE(std::string(sentinel.begin(), sentinel.end()) == "keep");
  REQUIRE(std::distance(fs::directory_iterator(output.path()), fs::directory_iterator{}) == 2);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: Write Stack", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

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

    ScopedTempDir tempDir(k_BinaryTestOutputDir);
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

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[2], offset, tempDir.name(), tempDir.path());
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
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[1], offset, tempDir.name(), tempDir.path());
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
    args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
    args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
    SizeVec3 imageDims = imageGeom->getDimensions();

    validateOutputFiles(imageDims[0], offset, tempDir.name(), tempDir.path());
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::WriteImageFilter: Single-slice write omits the index suffix", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 4, 1});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 4, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  UnitTest::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // The preflight example previews the exact single-slice file name (no index suffix).
  bool foundExample = false;
  for(const auto& value : preflightResult.outputValues)
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::WriteImageFilter: MHA preserves ITK scalar data and plane metadata", "[ImageProcessing][WriteImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64, float32, float64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions({3, 1, 2});
  imageGeom->setOrigin({10.0f, 20.0f, 40.0f});
  imageGeom->setSpacing({1.0f, 2.0f, 4.0f});
  auto* cellData = AttributeMatrix::Create(dataStructure, "CellData", {2, 1, 3}, imageGeom->getId());
  imageGeom->setCellData(*cellData);
  auto* imageData = UnitTest::CreateTestDataArray<TestType>(dataStructure, "ImageData", {2, 1, 3}, {1}, cellData->getId());
  auto& imageStore = imageData->getDataStoreRef();
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
  SIMPLNX_RESULT_REQUIRE_VALID(writePreflightResult.outputActions);
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
  SIMPLNX_RESULT_REQUIRE_VALID(readPreflightResult.outputActions);
  auto readExecuteResult = reader.execute(readDataStructure, readArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readExecuteResult.result);

  const auto& readGeom = readDataStructure.getDataRefAs<ImageGeom>(readGeomPath);
  CHECK(readGeom.getDimensions() == SizeVec3{3, 2, 1});
  CHECK(readGeom.getOrigin() == FloatVec3{10.0f, 40.0f, 0.0f});
  CHECK(readGeom.getSpacing() == FloatVec3{1.0f, 4.0f, 1.0f});

  const DataPath readArrayPath = readGeomPath.createChildPath("CellData").createChildPath("ImageData");
  REQUIRE_NOTHROW(readDataStructure.getDataRefAs<DataArray<TestType>>(readArrayPath));
  const auto& readStore = readDataStructure.getDataRefAs<DataArray<TestType>>(readArrayPath).getDataStoreRef();
  REQUIRE(readStore.getNumberOfTuples() == expectedValues.size());
  for(usize index = 0; index < expectedValues.size(); ++index)
  {
    CHECK(readStore[index] == static_cast<TestType>(expectedValues[index]));
  }
}

TEST_CASE("ImageProcessing::WriteImageFilter: MHA maps metadata for every plane", "[ImageProcessing][WriteImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions({3, 2, 2});
  imageGeom->setOrigin({10.0f, 20.0f, 40.0f});
  imageGeom->setSpacing({1.0f, 2.0f, 4.0f});
  auto* cellData = AttributeMatrix::Create(dataStructure, "CellData", {2, 2, 3}, imageGeom->getId());
  imageGeom->setCellData(*cellData);
  UnitTest::CreateTestDataArray<uint8>(dataStructure, "ImageData", {2, 2, 3}, {1}, cellData->getId());

  struct PlaneMetadata
  {
    ChoicesParameter::ValueType plane;
    SizeVec3 dimensions;
    FloatVec3 origin;
    FloatVec3 spacing;
  };
  const std::array<PlaneMetadata, 3> metadataCases = {PlaneMetadata{k_XYPlane, {3, 2, 1}, {10.0f, 20.0f, 0.0f}, {1.0f, 2.0f, 1.0f}},
                                                      PlaneMetadata{k_XZPlane, {3, 2, 1}, {10.0f, 40.0f, 0.0f}, {1.0f, 4.0f, 1.0f}},
                                                      PlaneMetadata{k_YZPlane, {2, 2, 1}, {20.0f, 40.0f, 0.0f}, {2.0f, 4.0f, 1.0f}}};

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
      SIMPLNX_RESULT_REQUIRE_VALID(writePreflightResult.outputActions);
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
      SIMPLNX_RESULT_REQUIRE_VALID(readPreflightResult.outputActions);
      auto readExecuteResult = reader.execute(readDataStructure, readArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(readExecuteResult.result);

      const auto& readGeom = readDataStructure.getDataRefAs<ImageGeom>(readGeomPath);
      CHECK(readGeom.getDimensions() == metadataCase.dimensions);
      CHECK(readGeom.getOrigin() == metadataCase.origin);
      CHECK(readGeom.getSpacing() == metadataCase.spacing);
    }
  }
}

TEST_CASE("ImageProcessing::WriteImageFilter: MHA preserves ITK component counts", "[ImageProcessing][WriteImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  for(const usize componentCount : std::array<usize, 6>{2, 3, 4, 10, 11, 36})
  {
    DYNAMIC_SECTION("components=" << componentCount)
    {
      DataStructure dataStructure;
      auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
      imageGeom->setDimensions({2, 1, 1});
      auto* cellData = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 2}, imageGeom->getId());
      imageGeom->setCellData(*cellData);
      auto* imageData = UnitTest::CreateTestDataArray<uint8>(dataStructure, "ImageData", {1, 1, 2}, {componentCount}, cellData->getId());
      auto& imageStore = imageData->getDataStoreRef();
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
      SIMPLNX_RESULT_REQUIRE_VALID(writePreflightResult.outputActions);
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
      SIMPLNX_RESULT_REQUIRE_VALID(readPreflightResult.outputActions);
      auto readExecuteResult = reader.execute(readDataStructure, readArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(readExecuteResult.result);

      const DataPath readArrayPath = readGeomPath.createChildPath("CellData").createChildPath("ImageData");
      const auto& readArray = readDataStructure.getDataRefAs<UInt8Array>(readArrayPath);
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

TEST_CASE("ImageProcessing::WriteImageFilter: validates fill characters and previews the first output index", "[ImageProcessing][WriteImageFilter]")
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions({2, 1, 2});
  auto* cellData = AttributeMatrix::Create(dataStructure, "CellData", {2, 1, 2}, imageGeom->getId());
  imageGeom->setCellData(*cellData);
  UnitTest::CreateTestDataArray<uint8>(dataStructure, "ImageData", {2, 1, 2}, {1}, cellData->getId());

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  WriteImageFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "ImageData"})));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
  args.insertOrAssign(WriteImageFilter::k_IndexOffset_Key, std::make_any<uint64>(100));

  const auto previewResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(previewResult.outputActions);
  REQUIRE(std::any_of(previewResult.outputValues.cbegin(), previewResult.outputValues.cend(),
                      [](const IFilter::PreflightValue& value) { return value.name == "Example Output File" && value.value.ends_with("slice_100.tif"); }));

  args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<std::string>("/"));
  const auto invalidFillResult = filter.preflight(dataStructure, args);
  REQUIRE(invalidFillResult.outputActions.invalid());
  REQUIRE(invalidFillResult.outputActions.errors()[0].code == -27018);
}

TEST_CASE("ImageProcessing::WriteImageFilter: Color table preflight validation", "[ImageProcessing][WriteImageFilter]")
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
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(k_BinaryTestOutputDir / "ct_pf.tif"));
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

TEST_CASE("ImageProcessing::WriteImageFilter: Inline color table across numeric input types", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
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

TEST_CASE("ImageProcessing::WriteImageFilter: Inline color table 3D multi-slice across planes", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  const std::string presetName = ColorTableUtilities::GetDefaultRGBPresetName();

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
  auto* scalarPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Scalar", {dimZ, dimY, dimX}, {1}, cellAmPtr->getId());
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
    SIMPLNX_RESULT_REQUIRE_VALID(ccmPreflight.outputActions);
    auto ccmExecute = ccm.execute(dataStructure, ccmArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(ccmExecute.result);
  }

  const DataPath rgbPath = geomPath.createChildPath("CellData").createChildPath("RGB");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(rgbPath));
  const auto& expectedRgbStore = dataStructure.getDataRefAs<UInt8Array>(rgbPath).getDataStoreRef();

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
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(true));
      args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(presetName));
      args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
      args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
      args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{0, 0, 0}));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
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

TEST_CASE("ImageProcessing::WriteImageFilter: Format-aware write-type preflight", "[ImageProcessing][WriteImageFilter]")
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

TEST_CASE("ImageProcessing::WriteImageFilter: Inline color table mask and constant-array handling", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  const std::string presetName = ColorTableUtilities::GetDefaultRGBPresetName();
  const DataPath geomPath({"ImageGeometry"});

  // Reads a single-slice color image back from a directory and returns the pointer to the RGB array.
  auto readBackSlice = [&](const fs::path& dir, DataStructure& readDs) -> const UInt8Array* {
    readBackSingleImage(readDs, dir / "slice.tif");
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

      ScopedTempDir maskDir(k_BinaryTestOutputDir);
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
        auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
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

    ScopedTempDir constDir(k_BinaryTestOutputDir);
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
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
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

TEST_CASE("ImageProcessing::WriteImageFilter: Flip output image about X or Y axis", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

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
    auto* scalarPtr = UnitTest::CreateTestDataArray<uint8>(dataStructure, "Scalar", {1, dimY, dimX}, {1}, cellAmPtr->getId());
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
      args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
      args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));
      args.insertOrAssign(WriteImageFilter::k_FlipMode_Key, std::make_any<ChoicesParameter::ValueType>(flipMode));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);

    DataStructure readDs;
    readBackSingleImage(readDs, tempDir.path() / "slice.png");

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

TEST_CASE("ImageProcessing::WriteImageFilter: Output flip composes with the color-table path", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr uint64 k_FlipNone = 0;
  constexpr uint64 k_FlipAboutX = 1;
  constexpr uint64 k_FlipAboutY = 2;

  const std::string presetName = ColorTableUtilities::GetDefaultRGBPresetName();
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
    auto* scalarPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Scalar", {1, 4, 4}, {1}, cellAmPtr->getId());
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
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);

    DataStructure readDs;
    readBackSingleImage(readDs, tempDir.path() / "slice.tif");

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

  // Reverse the source rows to calculate the expected X-axis flip.
  // Other tests verify the color calculation.
  std::vector<uint8> expectedFlipXImage(noneImage.size());
  for(usize y = 0; y < height; y++)
  {
    const usize srcRow = y;
    const usize dstRow = height - 1 - y;
    std::copy(noneImage.begin() + static_cast<std::ptrdiff_t>(srcRow * width * 3), noneImage.begin() + static_cast<std::ptrdiff_t>((srcRow + 1) * width * 3),
              expectedFlipXImage.begin() + static_cast<std::ptrdiff_t>(dstRow * width * 3));
  }
  REQUIRE(flipXImage == expectedFlipXImage);

  // Reverse the RGB pixel groups in each row to calculate the expected Y-axis flip.
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

TEST_CASE("ImageProcessing::WriteImageFilter: Scale bar preflight validation", "[ImageProcessing][WriteImageFilter]")
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
    args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(k_BinaryTestOutputDir / "scale_bar_preflight.png"));
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

TEST_CASE("ImageProcessing::WriteImageFilter: Scale bar pads the written image with a band", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Writes a 200 x 100 uint8 ramp with 0.5 µm spacing and reads the PNG result.
  // Color-table output uses the default preset.
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
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);

    readBackSingleImage(readDs, tempDir.path() / "slice.png");
  };

  // Expected layout for 200x100 @ 0.5 µm/px:
  //   band = max(24, llround(0.08*100)=8) = 24  -> written image is 200 x 124
  //   nice = ComputeNiceBarLength(200, 0.5) : target 25 µm -> 20 µm -> 40 px bar
  //   margin = 3, thickness = 2 -> bar vertically centered: rows (global) 100+(24-2)/2 = 111 and 112
  //   barStartCol = margin = 3 (left-justified) -> bar cols 3..42; label starts on the same line at col 3+40+3 = 46
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
    for(usize row = dimY; row < dimY + bandHeight && !foundTextPixel; row++)
    {
      for(usize col = 46; col < dimX; col++)
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

TEST_CASE("ImageProcessing::WriteImageFilter: Scale bar composes with flip and RGB input", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);

  DataStructure readDs;
  readBackSingleImage(readDs, tempDir.path() / "slice.png");

  REQUIRE_NOTHROW(readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
  const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  SizeVec3 readDims = readGeom.getDimensions();
  REQUIRE(readDims[0] == 3);
  REQUIRE(readDims[1] == 2 + 24); // band = max(24, llround(0.08*2)=0) = 24

  const auto* readArrayPtr = dynamic_cast<const UInt8Array*>(readGeom.getCellData()->begin()->second.get());
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

TEST_CASE("ImageProcessing::WriteImageFilter: Scale bar on XZ-plane slices locks per-plane dims and horizontal spacing", "[ImageProcessing][WriteImageFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  // Distinct dimensions separate the XZ width and height from the slice count.
  // Distinct spacing detects selection of the incorrect horizontal axis.
  constexpr usize dimX = 8;
  constexpr usize dimY = 4;
  constexpr usize dimZ = 6;

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  imageGeomPtr->setSpacing({0.5f, 1.0f, 1.0f});
  imageGeomPtr->setUnits(IGeometry::LengthUnit::Micrometer);
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {dimZ, dimY, dimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* scalarPtr = UnitTest::CreateTestDataArray<uint8>(dataStructure, "Scalar", {dimZ, dimY, dimX}, {1}, cellAmPtr->getId());
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
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

  // XZ slice: W = dimX = 8, H = dimZ = 6; band = max(24, llround(0.08*6)=0) = 24 -> 8 x 30.
  REQUIRE_NOTHROW(readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"})));
  const auto& readGeom = readDs.getDataRefAs<ImageGeom>(DataPath({"ReadGeom"}));
  SizeVec3 readDims = readGeom.getDimensions();
  REQUIRE(readDims[0] == dimX);
  REQUIRE(readDims[1] == dimZ + 24);

  const auto* readArrayPtr = dynamic_cast<const UInt8Array*>(readGeom.getCellData()->begin()->second.get());
  REQUIRE(readArrayPtr != nullptr);
  const auto& readStore = readArrayPtr->getDataStoreRef();
  REQUIRE(readArrayPtr->getNumberOfComponents() == 3);

  // First band row (row = dimZ, col = 0) is white background.
  REQUIRE(readStore.getValue((dimZ * dimX + 0) * 3) == 255);
}

TEST_CASE("ImageProcessing::WriteImageFilter: groups YZ slices into caller-owned bounded extents", "[ImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize dimX = 8;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {dimZ, dimY, dimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  auto countingStore = std::make_shared<ExtentCountingDataStore<uint8>>(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{0});
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
  if(scenario == UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    REQUIRE(countingStore->returnedExtentReadCount() == dimX);
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
  REQUIRE(outputFileCount == dimX);
}

TEST_CASE("ImageProcessing::WriteImageFilter: groups color and mask YZ slices into caller-owned bounded extents", "[ImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize dimX = 8;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");
  const DataPath maskPath = geomPath.createChildPath("CellData").createChildPath("Mask");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {dimZ, dimY, dimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  auto scalarStore = std::make_shared<ExtentCountingDataStore<float32>>(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0F);
  auto maskStore = std::make_shared<ExtentCountingDataStore<uint8>>(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{1});
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
  args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(ColorTableUtilities::GetDefaultRGBPresetName()));
  args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(WriteImageFilter::k_InvalidColorValue_Key, std::make_any<std::vector<uint8>>(std::vector<uint8>{10, 20, 30}));

  ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(240);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CAPTURE(scalarStore->returnedExtentReadCount(), scalarStore->callerProvidedExtentReadCount(), scalarStore->maxCallerProvidedExtentValues(), maskStore->returnedExtentReadCount(),
          maskStore->callerProvidedExtentReadCount(), maskStore->maxCallerProvidedExtentValues());
  if(scenario == UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    REQUIRE(scalarStore->returnedExtentReadCount() == dimX);
    REQUIRE(scalarStore->callerProvidedExtentReadCount() == 0);
    REQUIRE(scalarStore->maxCallerProvidedExtentValues() == 0);
    REQUIRE(maskStore->returnedExtentReadCount() == dimX);
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
  REQUIRE(outputFileCount == dimX);
}

TEST_CASE("ImageProcessing::WriteImageFilter: reads direct XY slices into separate caller-owned extents", "[ImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize dimX = 8;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {dimZ, dimY, dimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto countingStore = std::make_shared<ExtentCountingDataStore<uint8>>(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{7});
  REQUIRE(DataArray<uint8>::Create(dataStructure, scalarPath.getTargetName(), countingStore, cellAmPtr->getId()) != nullptr);

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_XYPlane));

  ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(dimX * dimY * dimZ);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  if(scenario == UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    REQUIRE(countingStore->returnedExtentReadCount() == dimZ);
    REQUIRE(countingStore->callerProvidedExtentReadCount() == 0);
  }
  else
  {
    REQUIRE(countingStore->returnedExtentReadCount() == 0);
    REQUIRE(countingStore->callerProvidedExtentReadCount() == dimZ);
    REQUIRE(countingStore->maxCallerProvidedExtentValues() == dimX * dimY);
  }
}

TEST_CASE("ImageProcessing::WriteImageFilter: reads color and mask XY slices into separate caller-owned extents", "[ImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize dimX = 8;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");
  const DataPath maskPath = geomPath.createChildPath("CellData").createChildPath("Mask");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {dimZ, dimY, dimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto scalarStore = std::make_shared<ExtentCountingDataStore<float32>>(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 2.0F);
  auto maskStore = std::make_shared<ExtentCountingDataStore<uint8>>(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{1});
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
  args.insertOrAssign(WriteImageFilter::k_SelectedPreset_Key, std::make_any<CreateColorMapParameter::ValueType>(ColorTableUtilities::GetDefaultRGBPresetName()));
  args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));

  ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(dimX * dimY * dimZ * (sizeof(float32) + sizeof(uint8)));
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  if(scenario == UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    REQUIRE(scalarStore->returnedExtentReadCount() == dimZ);
    REQUIRE(maskStore->returnedExtentReadCount() == dimZ);
  }
  else
  {
    REQUIRE(scalarStore->returnedExtentReadCount() == 0);
    REQUIRE(scalarStore->callerProvidedExtentReadCount() == dimZ);
    REQUIRE(scalarStore->maxCallerProvidedExtentValues() == dimX * dimY);
    REQUIRE(maskStore->returnedExtentReadCount() == 0);
    REQUIRE(maskStore->callerProvidedExtentReadCount() == dimZ);
    REQUIRE(maskStore->maxCallerProvidedExtentValues() == dimX * dimY);
  }
}

TEST_CASE("ImageProcessing::WriteImageFilter: rejects a source-group grant smaller than one slice", "[ImageProcessing][WriteImageFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  constexpr usize dimX = 8;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 4;
  const DataPath geomPath({"ImageGeometry"});
  const DataPath scalarPath = geomPath.createChildPath("CellData").createChildPath("Scalar");

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {dimZ, dimY, dimX}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  UnitTest::CreateTestDataArray<uint8>(dataStructure, scalarPath.getTargetName(), {dimZ, dimY, dimX}, {1}, cellAmPtr->getId());

  ScopedTempDir tempDir(k_BinaryTestOutputDir);
  WriteImageFilter filter;
  Arguments args;
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(scalarPath));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(tempDir.path() / "slice.tif"));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(k_YZPlane));

  ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(1);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  if(scenario == UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors().front().code == -27026);
  }
}
