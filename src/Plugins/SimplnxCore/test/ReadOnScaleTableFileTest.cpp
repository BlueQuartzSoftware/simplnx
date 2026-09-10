#include "SimplnxCore/Filters/ReadOnScaleTableFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CreateRectGridGeometryAction.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <clocale>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
constexpr int32 k_BadHeaderCountError = -12052;
constexpr int32 k_NonNumericBoundError = -12053;
constexpr int32 k_NameValuesTooShortError = -12055;
constexpr int32 k_MaterialValueError = -12056;
constexpr int32 k_MaterialValuesTooShortError = -12057;
constexpr int32 k_InvalidBoundCountError = -12058;
constexpr int32 k_MissingMaterialSectionError = -12059;
constexpr int32 k_FileChangedError = -12060;
constexpr int32 k_MaterialCountMismatchWarning = -12061;
constexpr int32 k_ExtraMaterialValuesWarning = -12062;

const DataPath k_GeometryPath({"OnScale Volume"});
const DataPath k_CellDataPath = k_GeometryPath.createChildPath("Cell Data");
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
const DataPath k_MaterialDataPath = k_GeometryPath.createChildPath("Material Data");
const DataPath k_MaterialNamesPath = k_MaterialDataPath.createChildPath("Material Names");
const std::array<std::string, 3> k_BoundsNames = {"X Bounds", "Y Bounds", "Z Bounds"};
const std::array<float32, 3> k_Increments = {0.001F, 0.002F, 0.003F};
const VectorFloat32Parameter::ValueType k_FallbackOrigin = {1.0F, 2.0F, 3.0F};
const VectorFloat32Parameter::ValueType k_FallbackSpacing = {0.5F, 0.25F, 0.125F};

enum class FixtureOrder : uint8
{
  Standard,
  ZNameXThenY
};

struct FixtureOptions
{
  std::array<usize, 3> BoundsCounts = {20, 17, 2};
  std::vector<std::string> Names = {"pzt4t11", "pzt4t12", "pzt4t13", "pzt4t14", "pzt4t15", "pzt4t16", "pzt4t17", "pzt4t18", "pzt4t19", "pzt4t20"};
  bool NamesOnePerLine = false;
  bool WriteNameSection = true;
  bool WriteMatrSection = true;
  FixtureOrder Order = FixtureOrder::Standard;
  std::string LineEnding = "\n";
  std::optional<usize> DeclaredNameCount;
  std::optional<usize> DeclaredMatrCount;
  std::optional<usize> WrittenMatrCount;
  std::optional<std::string> XBoundToken;
  std::optional<usize> NonNumericMaterialIndex;
  bool BadXHeaderCount = false;
};

struct GeneratedFixture
{
  std::array<std::vector<float32>, 3> Bounds;
  std::vector<std::string> Names;
  std::vector<int32> MaterialValues;
};

class LocaleGuard
{
public:
  LocaleGuard()
  {
    if(const char* currentLocale = std::setlocale(LC_ALL, nullptr); currentLocale != nullptr)
    {
      m_PreviousLocale = currentLocale;
    }
  }

  ~LocaleGuard()
  {
    if(!m_PreviousLocale.empty())
    {
      std::setlocale(LC_ALL, m_PreviousLocale.c_str());
    }
  }

  LocaleGuard(const LocaleGuard&) = delete;
  LocaleGuard(LocaleGuard&&) noexcept = delete;
  LocaleGuard& operator=(const LocaleGuard&) = delete;
  LocaleGuard& operator=(LocaleGuard&&) noexcept = delete;

  bool set(const char* localeName)
  {
    return std::setlocale(LC_ALL, localeName) != nullptr;
  }

private:
  std::string m_PreviousLocale;
};

class TestableReadOnScaleTableFileFilter : public ReadOnScaleTableFileFilter
{
public:
  using ReadOnScaleTableFileFilter::executeImpl;
};

void WriteTokens(std::ofstream& output, const std::vector<std::string>& tokens, usize tokensPerLine, std::string_view lineEnding)
{
  for(usize index = 0; index < tokens.size(); index++)
  {
    output << tokens[index];
    if(index + 1 < tokens.size())
    {
      output << ((index + 1) % tokensPerLine == 0 ? lineEnding : " ");
    }
  }
  output << lineEnding;
}

GeneratedFixture WriteFixture(const fs::path& filePath, const FixtureOptions& options)
{
  fs::create_directories(filePath.parent_path());
  std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
  REQUIRE(output.is_open());

  GeneratedFixture fixture;
  fixture.Names = options.Names;
  for(usize axis = 0; axis < fixture.Bounds.size(); axis++)
  {
    fixture.Bounds[axis].reserve(options.BoundsCounts[axis]);
    float32 value = 0.0F;
    for(usize index = 0; index < options.BoundsCounts[axis]; index++)
    {
      fixture.Bounds[axis].push_back(value);
      value += k_Increments[axis];
    }
  }

  const auto effectiveCount = [&options](usize axis) { return options.BoundsCounts[axis] == 0 ? usize{2} : options.BoundsCounts[axis]; };
  const usize numCells = (effectiveCount(0) - 1) * (effectiveCount(1) - 1) * (effectiveCount(2) - 1);
  const usize writtenMatrCount = options.WrittenMatrCount.value_or(numCells);
  fixture.MaterialValues.reserve(writtenMatrCount);
  for(usize index = 0; index < writtenMatrCount; index++)
  {
    fixture.MaterialValues.push_back(options.Names.empty() ? 0 : static_cast<int32>(index % options.Names.size()));
  }

  const auto writeBounds = [&](usize axis, std::string_view sectionName) {
    if(options.BoundsCounts[axis] == 0)
    {
      return;
    }
    if(axis == 0 && options.BadXHeaderCount)
    {
      output << "xcrd invalid-count" << options.LineEnding;
    }
    else
    {
      output << sectionName << "     " << options.BoundsCounts[axis] << options.LineEnding;
    }

    std::vector<std::string> tokens;
    tokens.reserve(fixture.Bounds[axis].size());
    for(usize index = 0; index < fixture.Bounds[axis].size(); index++)
    {
      if(axis == 0 && options.XBoundToken.has_value() && index == 1)
      {
        tokens.push_back(options.XBoundToken.value());
      }
      else
      {
        tokens.push_back(fmt::format("{:.8E}", fixture.Bounds[axis][index]));
      }
    }
    WriteTokens(output, tokens, 6, options.LineEnding);
  };

  const auto writeNames = [&] {
    if(!options.WriteNameSection)
    {
      return;
    }
    output << "name        " << options.DeclaredNameCount.value_or(options.Names.size()) << options.LineEnding;
    WriteTokens(output, options.Names, options.NamesOnePerLine || options.Names.empty() ? 1 : options.Names.size(), options.LineEnding);
  };

  output << "hedr         0" << options.LineEnding;
  output << "info         1" << options.LineEnding;
  if(options.Order == FixtureOrder::Standard)
  {
    writeBounds(0, "xcrd");
    writeBounds(1, "ycrd");
    writeBounds(2, "zcrd");
    output << "keypoints" << options.LineEnding << "2 2 2" << options.LineEnding;
    output << "divisions" << options.LineEnding << "1 1 1" << options.LineEnding;
    writeNames();
  }
  else
  {
    writeBounds(2, "zcrd");
    writeNames();
    output << "divisions" << options.LineEnding << "1 1 1" << options.LineEnding;
    writeBounds(0, "xcrd");
    output << "keypoints" << options.LineEnding << "2 2 2" << options.LineEnding;
    writeBounds(1, "ycrd");
  }

  if(!options.WriteMatrSection)
  {
    return fixture;
  }

  output << "matr        " << options.DeclaredMatrCount.value_or(numCells) << options.LineEnding;
  std::vector<std::string> materialTokens;
  materialTokens.reserve(fixture.MaterialValues.size());
  for(usize index = 0; index < fixture.MaterialValues.size(); index++)
  {
    materialTokens.push_back(options.NonNumericMaterialIndex == index ? "not-an-integer" : fmt::format("{}", fixture.MaterialValues[index]));
  }
  for(usize index = 0; index < materialTokens.size(); index++)
  {
    output << materialTokens[index];
    if(index + 1 < materialTokens.size())
    {
      output << ((index + 1) % 40 == 0 ? options.LineEnding : " ");
    }
  }

  return fixture;
}

Arguments CreateArguments(const fs::path& inputFile)
{
  Arguments args = ReadOnScaleTableFileFilter().getDefaultArguments();
  args.insertOrAssign(ReadOnScaleTableFileFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(inputFile));
  args.insertOrAssign(ReadOnScaleTableFileFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_FallbackOrigin));
  args.insertOrAssign(ReadOnScaleTableFileFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_FallbackSpacing));
  args.insertOrAssign(ReadOnScaleTableFileFilter::k_CreatedRectGridGeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(ReadOnScaleTableFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_CellDataPath.getTargetName()));
  args.insertOrAssign(ReadOnScaleTableFileFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIdsPath.getTargetName()));
  args.insertOrAssign(ReadOnScaleTableFileFilter::k_PhaseAttributeMatrixName_Key, std::make_any<std::string>(k_MaterialDataPath.getTargetName()));
  args.insertOrAssign(ReadOnScaleTableFileFilter::k_MaterialNamesArrayName_Key, std::make_any<std::string>(k_MaterialNamesPath.getTargetName()));
  return args;
}

void RequireValidImport(const fs::path& inputFile, const GeneratedFixture& expectedFixture, const SizeVec3& expectedDimensions)
{
  DataStructure dataStructure;
  const ReadOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(inputFile));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<RectGridGeom>(k_GeometryPath));
  const auto& geometry = dataStructure.getDataRefAs<RectGridGeom>(k_GeometryPath);
  REQUIRE(geometry.getDimensions() == expectedDimensions);
  REQUIRE(geometry.getUnits() == IGeometry::LengthUnit::Meter);

  for(usize axis = 0; axis < k_BoundsNames.size(); axis++)
  {
    const DataPath boundsPath = k_GeometryPath.createChildPath(k_BoundsNames[axis]);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(boundsPath));
    const auto& bounds = dataStructure.getDataRefAs<Float32Array>(boundsPath);
    if(expectedFixture.Bounds[axis].empty())
    {
      REQUIRE(bounds.getNumberOfTuples() == 2);
      REQUIRE(bounds[0] == k_FallbackOrigin[axis]);
      REQUIRE(bounds[1] == k_FallbackOrigin[axis] + k_FallbackSpacing[axis]);
    }
    else
    {
      REQUIRE(bounds.getNumberOfTuples() == expectedFixture.Bounds[axis].size());
      REQUIRE(std::equal(bounds.cbegin(), bounds.cend(), expectedFixture.Bounds[axis].cbegin()));
    }
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(k_MaterialNamesPath));
  const auto& names = dataStructure.getDataRefAs<StringArray>(k_MaterialNamesPath);
  REQUIRE(std::equal(names.cbegin(), names.cend(), expectedFixture.Names.cbegin()));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  REQUIRE(featureIds.getTupleShape() == ShapeType{expectedDimensions[2], expectedDimensions[1], expectedDimensions[0]});
  REQUIRE(std::equal(featureIds.cbegin(), featureIds.cend(), expectedFixture.MaterialValues.cbegin()));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

const CreateRectGridGeometryAction& GetGeometryAction(const IFilter::PreflightResult& preflightResult)
{
  REQUIRE(preflightResult.outputActions.valid());
  REQUIRE_FALSE(preflightResult.outputActions.value().actions.empty());
  const auto* action = dynamic_cast<const CreateRectGridGeometryAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(action != nullptr);
  return *action;
}
} // namespace

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Full 3D", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "Full3D.flxtbl";
  const GeneratedFixture fixture = WriteFixture(inputFile, FixtureOptions{});

  DataStructure preflightDataStructure;
  const ReadOnScaleTableFileFilter filter;
  auto preflightResult = filter.preflight(preflightDataStructure, CreateArguments(inputFile));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputValues.size() == 1);
  REQUIRE(preflightResult.outputValues.front().name == "Rect Grid Geometry Info");
  REQUIRE(preflightResult.outputValues.front().value.find("X Range: 0.00000000E+00 to 1.90000013E-02 (20 bounds, 19 cells)") != std::string::npos);
  REQUIRE(preflightResult.outputValues.front().value.find("Y Range: 0.00000000E+00 to 3.20000015E-02 (17 bounds, 16 cells)") != std::string::npos);
  REQUIRE(preflightResult.outputValues.front().value.find("Z Range: 0.00000000E+00 to 3.00000003E-03 (2 bounds, 1 cells)") != std::string::npos);
  REQUIRE(preflightResult.outputValues.front().value.find("Units: meters") != std::string::npos);
  auto applyResult = preflightResult.outputActions.value().applyRegular(preflightDataStructure, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  REQUIRE_NOTHROW(preflightDataStructure.getDataRefAs<AttributeMatrix>(k_CellDataPath));
  REQUIRE(preflightDataStructure.getDataRefAs<AttributeMatrix>(k_CellDataPath).getShape() == ShapeType{1, 16, 19});

  RequireValidImport(inputFile, fixture, {19, 16, 1});
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Missing Axis", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  const auto [label, counts, expectedDimensions] = GENERATE(table<std::string, std::array<usize, 3>, SizeVec3>({
      {"Missing X", {0, 10, 10}, {1, 9, 9}},
      {"Missing Y", {10, 0, 10}, {9, 1, 9}},
      {"Missing Z", {10, 10, 0}, {9, 9, 1}},
  }));
  CAPTURE(label);

  FixtureOptions options;
  options.BoundsCounts = counts;
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / fmt::format("{}.flxtbl", label);
  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, expectedDimensions);
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Names One Per Line", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  FixtureOptions options;
  options.BoundsCounts = {4, 3, 2};
  options.NamesOnePerLine = true;
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "NamesOnePerLine.flxtbl";
  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, {3, 2, 1});
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Sections In Different Order", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  FixtureOptions options;
  options.BoundsCounts = {4, 3, 2};
  options.Order = FixtureOrder::ZNameXThenY;
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "DifferentOrder.flxtbl";
  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, {3, 2, 1});
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Matr Count Mismatch Warns", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  FixtureOptions options;
  options.BoundsCounts = {4, 3, 2};
  options.DeclaredMatrCount = 5;
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "MatrCountMismatch.flxtbl";
  WriteFixture(inputFile, options);

  std::vector<IFilter::Message> messages;
  std::mutex messagesMutex;
  const IFilter::MessageHandler messageHandler{[&messages, &messagesMutex](const IFilter::Message& message) {
    const std::lock_guard lock(messagesMutex);
    messages.push_back(message);
  }};
  DataStructure dataStructure;
  const ReadOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(inputFile), nullptr, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(std::none_of(messages.cbegin(), messages.cend(), [](const IFilter::Message& message) { return message.type == IFilter::Message::Type::Warning; }));
  REQUIRE(executeResult.result.warnings().size() == 1);
  REQUIRE(executeResult.result.warnings().front().code == k_MaterialCountMismatchWarning);
  REQUIRE(executeResult.result.warnings().front().message.find("declares 5 values") != std::string::npos);
  REQUIRE(executeResult.result.warnings().front().message.find("6 cells") != std::string::npos);
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Matr Too Short Fails", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  FixtureOptions options;
  options.BoundsCounts = {4, 3, 2};
  options.WrittenMatrCount = 5;
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "MatrTooShort.flxtbl";
  WriteFixture(inputFile, options);

  DataStructure dataStructure;
  const ReadOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(inputFile));
  REQUIRE(executeResult.result.invalid());
  REQUIRE(executeResult.result.errors().front().code == k_MaterialValuesTooShortError);
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Invalid Preflight Input", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  const std::string testName = GENERATE("Non Numeric Bound", "Bad Header Count", "Missing Input File");
  CAPTURE(testName);

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / fmt::format("{}.flxtbl", testName);
  if(testName != "Missing Input File")
  {
    FixtureOptions options;
    options.BoundsCounts = {4, 3, 2};
    if(testName == "Non Numeric Bound")
    {
      options.XBoundToken = "not-a-number";
    }
    options.BadXHeaderCount = testName == "Bad Header Count";
    WriteFixture(inputFile, options);
  }
  else
  {
    fs::remove(inputFile);
  }

  DataStructure dataStructure;
  const ReadOnScaleTableFileFilter filter;
  auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile));
  REQUIRE(preflightResult.outputActions.invalid());
  if(testName == "Non Numeric Bound")
  {
    REQUIRE(preflightResult.outputActions.errors().front().code == k_NonNumericBoundError);
  }
  else if(testName == "Bad Header Count")
  {
    REQUIRE(preflightResult.outputActions.errors().front().code == k_BadHeaderCountError);
  }
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Additional Valid Inputs", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  const std::string testName = GENERATE("CRLF Line Endings", "Empty Names", "Nonstandard Extension");
  CAPTURE(testName);

  FixtureOptions options;
  options.BoundsCounts = {4, 3, 2};
  fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / fmt::format("{}.flxtbl", testName);
  if(testName == "CRLF Line Endings")
  {
    options.LineEnding = "\r\n";
  }
  else if(testName == "Empty Names")
  {
    options.Names.clear();
  }
  else
  {
    inputFile.replace_extension(".table");
  }

  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, {3, 2, 1});
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Malformed File", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  const std::string testName = GENERATE("Names Too Short", "Missing Matr", "Non Numeric Material", "Bound Count Too Small", "Bound Out Of Range");
  CAPTURE(testName);

  FixtureOptions options;
  options.BoundsCounts = {4, 3, 2};
  if(testName == "Names Too Short")
  {
    options.Names = {"first", "second"};
    options.DeclaredNameCount = 3;
  }
  else if(testName == "Missing Matr")
  {
    options.WriteMatrSection = false;
  }
  else if(testName == "Non Numeric Material")
  {
    options.NonNumericMaterialIndex = 2;
  }
  else if(testName == "Bound Count Too Small")
  {
    options.BoundsCounts = {1, 3, 2};
  }
  else
  {
    options.XBoundToken = "1.0E+1000";
  }

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / fmt::format("{}.flxtbl", testName);
  WriteFixture(inputFile, options);
  DataStructure dataStructure;
  const ReadOnScaleTableFileFilter filter;
  const Arguments args = CreateArguments(inputFile);

  if(testName == "Non Numeric Material")
  {
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.invalid());
    REQUIRE(executeResult.result.errors().front().code == k_MaterialValueError);
  }
  else
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
    if(testName == "Names Too Short")
    {
      REQUIRE(preflightResult.outputActions.errors().front().code == k_NameValuesTooShortError);
      REQUIRE(preflightResult.outputActions.errors().front().message.find("declares 3 values, but only 2 values were found before the next section at line") != std::string::npos);
    }
    else if(testName == "Missing Matr")
    {
      REQUIRE(preflightResult.outputActions.errors().front().code == k_MissingMaterialSectionError);
    }
    else if(testName == "Bound Count Too Small")
    {
      REQUIRE(preflightResult.outputActions.errors().front().code == k_InvalidBoundCountError);
    }
    else
    {
      REQUIRE(preflightResult.outputActions.errors().front().code == k_NonNumericBoundError);
      REQUIRE(preflightResult.outputActions.errors().front().message.find("out of range for float32") != std::string::npos);
    }
  }
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Extra Material Values Warn Once", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  FixtureOptions options;
  options.BoundsCounts = {4, 3, 2};
  options.DeclaredMatrCount = 6;
  options.WrittenMatrCount = 8;
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "ExtraMaterialValues.flxtbl";
  WriteFixture(inputFile, options);

  std::vector<IFilter::Message> messages;
  std::mutex messagesMutex;
  const IFilter::MessageHandler messageHandler{[&messages, &messagesMutex](const IFilter::Message& message) {
    const std::lock_guard lock(messagesMutex);
    messages.push_back(message);
  }};
  DataStructure dataStructure;
  const ReadOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(inputFile), nullptr, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(std::none_of(messages.cbegin(), messages.cend(), [](const IFilter::Message& message) { return message.type == IFilter::Message::Type::Warning; }));
  REQUIRE(executeResult.result.warnings().size() == 1);
  REQUIRE(executeResult.result.warnings().front().code == k_ExtraMaterialValuesWarning);
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: File Changed After Preflight", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  const std::string testName = GENERATE("Different X Count", "Missing X Section", "Missing Name Section");
  CAPTURE(testName);

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / fmt::format("{}.flxtbl", testName);
  FixtureOptions initialOptions;
  initialOptions.BoundsCounts = {4, 3, 2};
  WriteFixture(inputFile, initialOptions);
  const auto initialWriteTime = fs::last_write_time(inputFile);

  DataStructure dataStructure;
  TestableReadOnScaleTableFileFilter filter;
  const Arguments args = CreateArguments(inputFile);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto applyResult = preflightResult.outputActions.value().applyRegular(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);

  FixtureOptions changedOptions = initialOptions;
  if(testName == "Different X Count")
  {
    changedOptions.BoundsCounts[0] = 5;
  }
  else if(testName == "Missing X Section")
  {
    changedOptions.BoundsCounts[0] = 0;
    changedOptions.DeclaredMatrCount = 6;
    changedOptions.WrittenMatrCount = 6;
  }
  else
  {
    changedOptions.WriteNameSection = false;
  }
  WriteFixture(inputFile, changedOptions);
  fs::last_write_time(inputFile, initialWriteTime + std::chrono::seconds(2));

  const std::atomic_bool shouldCancel = false;
  auto executeResult = filter.executeImpl(dataStructure, args, nullptr, {}, shouldCancel, {});
  REQUIRE(executeResult.invalid());
  REQUIRE(executeResult.errors().front().code == k_FileChangedError);
  REQUIRE(executeResult.errors().front().message.find("file changed after preflight") != std::string::npos);
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: C Locale Float Parsing", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  LocaleGuard localeGuard;
  if(!localeGuard.set("de_DE.UTF-8"))
  {
    SUCCEED("The de_DE.UTF-8 locale is not installed; locale-specific parsing test skipped.");
    return;
  }

  FixtureOptions options;
  options.BoundsCounts = {4, 3, 2};
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "CLocaleParsing.flxtbl";
  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, {3, 2, 1});
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Header Cache Refreshes", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "CacheRefresh.flxtbl";
  FixtureOptions options;
  options.BoundsCounts = {4, 3, 2};
  WriteFixture(inputFile, options);

  DataStructure dataStructure;
  const ReadOnScaleTableFileFilter filter;
  const auto firstPreflight = filter.preflight(dataStructure, CreateArguments(inputFile));
  const auto& firstAction = GetGeometryAction(firstPreflight);
  REQUIRE(firstAction.xDims() == 4);
  REQUIRE(firstAction.yDims() == 3);
  REQUIRE(firstAction.zDims() == 2);

  const auto originalWriteTime = fs::last_write_time(inputFile);
  options.BoundsCounts = {6, 5, 4};
  WriteFixture(inputFile, options);
  fs::last_write_time(inputFile, originalWriteTime + std::chrono::seconds(2));

  const auto secondPreflight = filter.preflight(dataStructure, CreateArguments(inputFile));
  const auto& secondAction = GetGeometryAction(secondPreflight);
  REQUIRE(secondAction.xDims() == 6);
  REQUIRE(secondAction.yDims() == 5);
  REQUIRE(secondAction.zDims() == 4);
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();
  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";
  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadOnScaleTableFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadOnScaleTableFileFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ReadOnScaleTableFileFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(ReadOnScaleTableFileFilter::k_InputFile_Key) == fs::path("/test/path/table.flxtbl"));
      CHECK(args.value<VectorFloat32Parameter::ValueType>(ReadOnScaleTableFileFilter::k_Origin_Key) == VectorFloat32Parameter::ValueType{1.0F, 2.0F, 3.0F});
      CHECK(args.value<VectorFloat32Parameter::ValueType>(ReadOnScaleTableFileFilter::k_Spacing_Key) == VectorFloat32Parameter::ValueType{0.5F, 0.25F, 0.125F});
      CHECK(args.value<DataPath>(ReadOnScaleTableFileFilter::k_CreatedRectGridGeometryPath_Key) == DataPath({"Imported OnScale"}));
      CHECK(args.value<std::string>(ReadOnScaleTableFileFilter::k_CellAttributeMatrixName_Key) == "Imported Cell Data");
      CHECK(args.value<std::string>(ReadOnScaleTableFileFilter::k_FeatureIdsArrayName_Key) == "Imported FeatureIds");
      CHECK(args.value<std::string>(ReadOnScaleTableFileFilter::k_PhaseAttributeMatrixName_Key) == "Imported Material Data");
      CHECK(args.value<std::string>(ReadOnScaleTableFileFilter::k_MaterialNamesArrayName_Key) == "Imported Material Names");
    }
  }
}
