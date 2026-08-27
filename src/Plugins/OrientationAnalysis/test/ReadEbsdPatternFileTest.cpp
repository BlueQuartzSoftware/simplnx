#include "OrientationAnalysis/Filters/Algorithms/ReadEbsdPatternFile.hpp"
#include "OrientationAnalysis/Filters/ReadEbsdPatternFileFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/utilities/EbsdPatternFileReaderFactory.hpp"
#include "OrientationAnalysis/utilities/EbsdPatternFileUtilities.hpp"
#include "OrientationAnalysis/utilities/EdaxUpPatternFileReader.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <filesystem>
#include <fstream>
#include <limits>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_OutputArrayPath({"Patterns"});

void WriteUInt32LittleEndian(std::ofstream& stream, uint32 value)
{
  const std::array<char, 4> bytes = {static_cast<char>(value & 0xFFU), static_cast<char>((value >> 8U) & 0xFFU), static_cast<char>((value >> 16U) & 0xFFU), static_cast<char>((value >> 24U) & 0xFFU)};
  stream.write(bytes.data(), bytes.size());
}

void WriteUInt64LittleEndian(std::ofstream& stream, uint64 value)
{
  for(uint64 byteIndex = 0; byteIndex < 8; byteIndex++)
  {
    const uint8 byte = static_cast<uint8>((value >> (byteIndex * 8U)) & 0xFFU);
    stream.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
  }
}

void WriteFloat64LittleEndian(std::ofstream& stream, float64 value)
{
  WriteUInt64LittleEndian(stream, std::bit_cast<uint64>(value));
}

void WriteVersion1Up1File(const fs::path& filePath)
{
  constexpr uint32 k_Width = 12;
  constexpr uint32 k_Height = 12;
  constexpr uint32 k_PatternCount = 4;

  std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
  REQUIRE(stream.is_open());
  WriteUInt32LittleEndian(stream, 1);
  WriteUInt32LittleEndian(stream, k_Width);
  WriteUInt32LittleEndian(stream, k_Height);
  WriteUInt32LittleEndian(stream, 16);

  for(uint32 value = 0; value < k_Width * k_Height * k_PatternCount; value++)
  {
    const uint8 byte = static_cast<uint8>(value % 251U);
    stream.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
  }
}

void WriteVersion1Up2File(const fs::path& filePath)
{
  constexpr uint32 k_Width = 12;
  constexpr uint32 k_Height = 12;
  constexpr uint32 k_PatternCount = 4;

  std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
  REQUIRE(stream.is_open());
  WriteUInt32LittleEndian(stream, 1);
  WriteUInt32LittleEndian(stream, k_Width);
  WriteUInt32LittleEndian(stream, k_Height);
  WriteUInt32LittleEndian(stream, 16);

  for(uint32 value = 0; value < k_Width * k_Height * k_PatternCount; value++)
  {
    const uint16 pixel = static_cast<uint16>(1000U + value);
    const std::array<char, 2> bytes = {static_cast<char>(pixel & 0xFFU), static_cast<char>((pixel >> 8U) & 0xFFU)};
    stream.write(bytes.data(), bytes.size());
  }
}

void WriteVersion3Up1File(const fs::path& filePath, uint8 extraPatterns = 0, uint8 storedExtraPatterns = 0, uint32 headerVersion = 3, float64 xStep = 0.5, float64 yStep = 0.75)
{
  constexpr uint32 k_Width = 12;
  constexpr uint32 k_Height = 12;
  constexpr uint32 k_Columns = 2;
  constexpr uint32 k_Rows = 2;

  std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
  REQUIRE(stream.is_open());
  WriteUInt32LittleEndian(stream, headerVersion);
  WriteUInt32LittleEndian(stream, k_Width);
  WriteUInt32LittleEndian(stream, k_Height);
  WriteUInt32LittleEndian(stream, 42);
  stream.write(reinterpret_cast<const char*>(&extraPatterns), sizeof(extraPatterns));
  WriteUInt32LittleEndian(stream, k_Columns);
  WriteUInt32LittleEndian(stream, k_Rows);
  constexpr uint8 k_HexFlag = 0;
  stream.write(reinterpret_cast<const char*>(&k_HexFlag), sizeof(k_HexFlag));
  WriteFloat64LittleEndian(stream, xStep);
  WriteFloat64LittleEndian(stream, yStep);

  const uint32 storedPatterns = k_Columns * k_Rows + storedExtraPatterns;
  for(uint32 value = 0; value < k_Width * k_Height * storedPatterns; value++)
  {
    const uint8 byte = static_cast<uint8>((value + 17U) % 251U);
    stream.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
  }
}

void WriteVersion3Up2File(const fs::path& filePath)
{
  constexpr uint32 k_Width = 12;
  constexpr uint32 k_Height = 12;
  constexpr uint32 k_Columns = 2;
  constexpr uint32 k_Rows = 2;

  std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
  REQUIRE(stream.is_open());
  WriteUInt32LittleEndian(stream, 3);
  WriteUInt32LittleEndian(stream, k_Width);
  WriteUInt32LittleEndian(stream, k_Height);
  WriteUInt32LittleEndian(stream, 42);
  constexpr uint8 k_ExtraPatterns = 0;
  stream.write(reinterpret_cast<const char*>(&k_ExtraPatterns), sizeof(k_ExtraPatterns));
  WriteUInt32LittleEndian(stream, k_Columns);
  WriteUInt32LittleEndian(stream, k_Rows);
  constexpr uint8 k_HexFlag = 0;
  stream.write(reinterpret_cast<const char*>(&k_HexFlag), sizeof(k_HexFlag));
  WriteFloat64LittleEndian(stream, 0.5);
  WriteFloat64LittleEndian(stream, 0.75);

  for(uint32 value = 0; value < k_Width * k_Height * k_Columns * k_Rows; value++)
  {
    const uint16 pixel = static_cast<uint16>(1000U + value);
    const std::array<char, 2> bytes = {static_cast<char>(pixel & 0xFFU), static_cast<char>((pixel >> 8U) & 0xFFU)};
    stream.write(bytes.data(), bytes.size());
  }
}

void WriteCustomVersion1File(const fs::path& filePath, uint32 version, uint32 width, uint32 height, uint32 dataOffset, uint64 payloadBytes)
{
  std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
  REQUIRE(stream.is_open());
  WriteUInt32LittleEndian(stream, version);
  WriteUInt32LittleEndian(stream, width);
  WriteUInt32LittleEndian(stream, height);
  WriteUInt32LittleEndian(stream, dataOffset);
  for(uint64 byteIndex = 16; byteIndex < dataOffset; byteIndex++)
  {
    constexpr uint8 k_Padding = 0;
    stream.write(reinterpret_cast<const char*>(&k_Padding), sizeof(k_Padding));
  }
  for(uint64 byteIndex = 0; byteIndex < payloadBytes; byteIndex++)
  {
    const uint8 value = static_cast<uint8>(byteIndex % 251U);
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
  }
}

Arguments CreateArguments(const fs::path& inputFile, const DataPath& outputPath = k_OutputArrayPath, bool setScanDimensions = false, uint64 rows = 1, uint64 columns = 1)
{
  ReadEbsdPatternFileFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ReadEbsdPatternFileFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(inputFile));
  args.insertOrAssign(ReadEbsdPatternFileFilter::k_SetScanDimensions_Key, std::make_any<bool>(setScanDimensions));
  args.insertOrAssign(ReadEbsdPatternFileFilter::k_NumberOfRows_Key, std::make_any<uint64>(rows));
  args.insertOrAssign(ReadEbsdPatternFileFilter::k_NumberOfColumns_Key, std::make_any<uint64>(columns));
  args.insertOrAssign(ReadEbsdPatternFileFilter::k_OutputArrayPath_Key, std::make_any<DataPath>(outputPath));
  return args;
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Version 1 UP2 Manual Scan Dimensions", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_v1_manual.up2";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion1Up2File(inputFile);

  DataStructure dataStructure;
  ReadEbsdPatternFileFilter filter;
  const Arguments args = CreateArguments(inputFile, k_OutputArrayPath, true, 2, 2);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt16Array>(k_OutputArrayPath));
  const auto& patterns = dataStructure.getDataRefAs<UInt16Array>(k_OutputArrayPath);
  REQUIRE(patterns.getTupleShape() == ShapeType{2, 2});
  REQUIRE(patterns.getComponentShape() == ShapeType{12, 12});
  REQUIRE(patterns[0] == 1000);
  REQUIRE(patterns[575] == 1575);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Version 3 Header Geometry", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_v3.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion3Up1File(inputFile);

  DataStructure dataStructure;
  ReadEbsdPatternFileFilter filter;
  const Arguments args = CreateArguments(inputFile);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_OutputArrayPath));
  const auto& patterns = dataStructure.getDataRefAs<UInt8Array>(k_OutputArrayPath);
  REQUIRE(patterns.getTupleShape() == ShapeType{2, 2});
  REQUIRE(patterns.getComponentShape() == ShapeType{12, 12});
  REQUIRE(patterns[0] == 17);
  REQUIRE(patterns[575] == static_cast<uint8>((575U + 17U) % 251U));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::EBSD Pattern Reader: Version 3 Metadata", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_v3_metadata.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion3Up1File(inputFile);

  auto readerResult = CreateEbsdPatternFileReader(inputFile);
  SIMPLNX_RESULT_REQUIRE_VALID(readerResult);
  auto fileInfoResult = readerResult.value()->readFileInfo();
  SIMPLNX_RESULT_REQUIRE_VALID(fileInfoResult);

  const EbsdPatternFileInfo& fileInfo = fileInfoResult.value();
  REQUIRE(fileInfo.headerVersion == 3);
  REQUIRE(fileInfo.pixelDataType == DataType::uint8);
  REQUIRE(fileInfo.patternWidth == 12);
  REQUIRE(fileInfo.patternHeight == 12);
  REQUIRE(fileInfo.numberOfColumns == 2);
  REQUIRE(fileInfo.numberOfRows == 2);
  REQUIRE(fileInfo.numberOfPatterns == 4);
  REQUIRE(fileInfo.xStep == 0.5);
  REQUIRE(fileInfo.yStep == 0.75);
  REQUIRE_FALSE(fileInfo.isHexagonal);
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Version 3 Extra Patterns Are Skipped", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_v3_extra.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion3Up1File(inputFile, 2, 2);

  DataStructure dataStructure;
  ReadEbsdPatternFileFilter filter;
  const Arguments args = CreateArguments(inputFile);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE_FALSE(preflightResult.outputActions.warnings().empty());

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE_FALSE(executeResult.result.warnings().empty());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_OutputArrayPath));
  const auto& patterns = dataStructure.getDataRefAs<UInt8Array>(k_OutputArrayPath);
  REQUIRE(patterns.getNumberOfTuples() == 4);
  REQUIRE(patterns.getSize() == 576);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Attribute Matrix Tuple Contract", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_attribute_matrix.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion1Up1File(inputFile);

  SECTION("Matching tuple count inherits the Attribute Matrix shape")
  {
    DataStructure dataStructure;
    auto* attributeMatrix = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 2});
    REQUIRE(attributeMatrix != nullptr);

    const DataPath outputPath({"Cell Data", "Patterns"});
    ReadEbsdPatternFileFilter filter;
    const Arguments args = CreateArguments(inputFile, outputPath);
    const auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(outputPath));
    const auto& patterns = dataStructure.getDataRefAs<UInt8Array>(outputPath);
    REQUIRE(patterns.getTupleShape() == ShapeType{2, 2});
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Mismatching tuple count is rejected")
  {
    DataStructure dataStructure;
    auto* attributeMatrix = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{3});
    REQUIRE(attributeMatrix != nullptr);

    const DataPath outputPath({"Cell Data", "Patterns"});
    ReadEbsdPatternFileFilter filter;
    const Arguments args = CreateArguments(inputFile, outputPath);
    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == -78040);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Invalid Version 1 Scan Dimensions", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_invalid_scan.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion1Up1File(inputFile);

  DataStructure dataStructure;
  ReadEbsdPatternFileFilter filter;
  const Arguments args = CreateArguments(inputFile, k_OutputArrayPath, true, 3, 2);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == -78043);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Malformed Files", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_malformed.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });

  ReadEbsdPatternFileFilter filter;
  DataStructure dataStructure;

  SECTION("Version 2 is rejected")
  {
    WriteCustomVersion1File(inputFile, 2, 12, 12, 16, 144);
    const auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile));
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == k_UnsupportedUpVersionError);
  }

  SECTION("Residual bytes are rejected")
  {
    WriteCustomVersion1File(inputFile, 1, 12, 12, 16, 145);
    const auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile));
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == k_InvalidUpPayloadError);
  }

  SECTION("Data offset overlapping the header is rejected")
  {
    WriteCustomVersion1File(inputFile, 1, 12, 12, 12, 144);
    const auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile));
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == k_InvalidUpDataOffsetError);
  }

  SECTION("Pattern dimensions outside the accepted range are rejected")
  {
    WriteCustomVersion1File(inputFile, 1, 11, 12, 16, 132);
    const auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile));
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == k_InvalidUpDimensionsError);
  }

  SECTION("A truncated version 3 grid is rejected")
  {
    WriteVersion3Up1File(inputFile);
    fs::resize_file(inputFile, 42 + 3 * 12 * 12);
    const auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile));
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == k_InvalidUpPayloadError);
  }

  SECTION("Overflowing manual scan dimensions are rejected")
  {
    WriteVersion1Up1File(inputFile);
    const auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile, k_OutputArrayPath, true, std::numeric_limits<uint64>::max(), 2));
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == EbsdPatternFileUtilities::k_SizeOverflowError);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::EBSD Pattern Reader: Factory Rejects Unsupported Extension", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "patterns.ebsp";
  auto readerResult = CreateEbsdPatternFileReader(inputFile);
  SIMPLNX_RESULT_REQUIRE_INVALID(readerResult);
  REQUIRE(readerResult.errors().front().code == k_UnsupportedEbsdPatternExtensionError);
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Probable Extension Mismatch", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_wrong_depth.up2";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion3Up1File(inputFile);

  DataStructure dataStructure;
  ReadEbsdPatternFileFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == k_UpExtensionMismatchError);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Probable Wide Payload Extension Mismatch", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_wrong_wide_depth.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion3Up2File(inputFile);

  DataStructure dataStructure;
  ReadEbsdPatternFileFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == k_UpExtensionMismatchError);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Cancellation Before Payload Read", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_cancel.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion1Up1File(inputFile);

  DataStructure dataStructure;
  auto* outputArray = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, k_OutputArrayPath.getTargetName(), ShapeType{4}, ShapeType{12, 12});
  REQUIRE(outputArray != nullptr);

  ReadEbsdPatternFileInputValues inputValues;
  inputValues.inputFile = inputFile;
  inputValues.outputArrayPath = k_OutputArrayPath;
  const std::atomic_bool shouldCancel{true};
  const auto executeResult = ReadEbsdPatternFile(dataStructure, inputValues, shouldCancel, {})();
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_OutputArrayPath));
  const auto& patterns = dataStructure.getDataRefAs<UInt8Array>(k_OutputArrayPath);
  REQUIRE(patterns[0] == 0);
  REQUIRE(patterns[patterns.getSize() - 1] == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::EBSD Pattern Reader: Version 3 Warnings", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_v3_warnings.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });

  SECTION("A future version uses the version 3 layout")
  {
    WriteVersion3Up1File(inputFile, 0, 0, 4);
    auto readerResult = CreateEbsdPatternFileReader(inputFile);
    SIMPLNX_RESULT_REQUIRE_VALID(readerResult);
    auto fileInfoResult = readerResult.value()->readFileInfo();
    SIMPLNX_RESULT_REQUIRE_VALID(fileInfoResult);
    REQUIRE(fileInfoResult.value().headerVersion == 4);
    REQUIRE(std::any_of(fileInfoResult.warnings().begin(), fileInfoResult.warnings().end(), [](const Warning& warning) { return warning.code == k_FutureUpVersionWarning; }));
  }

  SECTION("Zero scan steps are treated as unknown")
  {
    WriteVersion3Up1File(inputFile, 0, 0, 3, 0.0, 0.0);
    auto readerResult = CreateEbsdPatternFileReader(inputFile);
    SIMPLNX_RESULT_REQUIRE_VALID(readerResult);
    auto fileInfoResult = readerResult.value()->readFileInfo();
    SIMPLNX_RESULT_REQUIRE_VALID(fileInfoResult);
    REQUIRE_FALSE(fileInfoResult.value().xStep.has_value());
    REQUIRE_FALSE(fileInfoResult.value().yStep.has_value());
    REQUIRE(std::any_of(fileInfoResult.warnings().begin(), fileInfoResult.warnings().end(), [](const Warning& warning) { return warning.code == k_UnknownUpStepWarning; }));
  }

  SECTION("Declared and stored extra-pattern counts may disagree")
  {
    WriteVersion3Up1File(inputFile, 2, 1);
    auto readerResult = CreateEbsdPatternFileReader(inputFile);
    SIMPLNX_RESULT_REQUIRE_VALID(readerResult);
    auto fileInfoResult = readerResult.value()->readFileInfo();
    SIMPLNX_RESULT_REQUIRE_VALID(fileInfoResult);
    REQUIRE(std::any_of(fileInfoResult.warnings().begin(), fileInfoResult.warnings().end(), [](const Warning& warning) { return warning.code == k_ExtraUpPatternsWarning; }));
    REQUIRE(std::any_of(fileInfoResult.warnings().begin(), fileInfoResult.warnings().end(), [](const Warning& warning) { return warning.code == k_ExtraUpPatternsSizeWarning; }));
  }
}

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Case Insensitive Extension", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_uppercase.UP1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion1Up1File(inputFile);

  DataStructure dataStructure;
  ReadEbsdPatternFileFilter filter;
  const auto executeResult = filter.execute(dataStructure, CreateArguments(inputFile));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_OutputArrayPath));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("OrientationAnalysis::ReadEbsdPatternFileFilter: Version 1 Flat Preflight", "[OrientationAnalysis][ReadEbsdPatternFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "read_ebsd_pattern_v1_flat.up1";
  auto fileGuard = MakeScopeGuard([&inputFile]() noexcept { fs::remove(inputFile); });
  WriteVersion1Up1File(inputFile);

  DataStructure dataStructure;
  ReadEbsdPatternFileFilter filter;
  Arguments args = CreateArguments(inputFile);

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_OutputArrayPath));
  const auto& patterns = dataStructure.getDataRefAs<UInt8Array>(k_OutputArrayPath);
  REQUIRE(patterns.getTupleShape() == ShapeType{4});
  REQUIRE(patterns.getComponentShape() == ShapeType{12, 12});
  REQUIRE(patterns.getNumberOfTuples() == 4);
  REQUIRE(patterns.getNumberOfComponents() == 144);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
