#include "simplnx/UnitTest/ReaderGeometryTest.hpp"

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
  std::array<usize, 3> bounds_counts = {20, 17, 2};
  std::vector<std::string> names = {"pzt4t11", "pzt4t12", "pzt4t13", "pzt4t14", "pzt4t15", "pzt4t16", "pzt4t17", "pzt4t18", "pzt4t19", "pzt4t20"};
  bool names_one_per_line = false;
  bool write_name_section = true;
  bool write_matr_section = true;
  FixtureOrder order = FixtureOrder::Standard;
  std::string line_ending = "\n";
  std::optional<usize> declared_name_count;
  std::optional<usize> declared_matr_count;
  std::optional<usize> written_matr_count;
  std::optional<std::string> x_bound_token;
  std::optional<usize> non_numeric_material_index;
  bool bad_x_header_count = false;
};

struct GeneratedFixture
{
  std::array<std::vector<float32>, 3> bounds;
  std::vector<std::string> names;
  std::vector<int32> material_values;
};

class LocaleGuard
{
public:
  LocaleGuard()
  {
    if(const char* currentLocalePtr = std::setlocale(LC_ALL, nullptr); currentLocalePtr != nullptr)
    {
      m_PreviousLocale = currentLocalePtr;
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

  static bool Set(const char* localeName)
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
  fixture.names = options.names;
  for(usize axis = 0; axis < fixture.bounds.size(); axis++)
  {
    fixture.bounds[axis].reserve(options.bounds_counts[axis]);
    float32 value = 0.0F;
    for(usize index = 0; index < options.bounds_counts[axis]; index++)
    {
      fixture.bounds[axis].push_back(value);
      value += k_Increments[axis];
    }
  }

  const auto effectiveCount = [&options](usize axis) { return options.bounds_counts[axis] == 0 ? usize{2} : options.bounds_counts[axis]; };
  const usize numCells = (effectiveCount(0) - 1) * (effectiveCount(1) - 1) * (effectiveCount(2) - 1);
  const usize writtenMatrCount = options.written_matr_count.value_or(numCells);
  fixture.material_values.reserve(writtenMatrCount);
  for(usize index = 0; index < writtenMatrCount; index++)
  {
    fixture.material_values.push_back(options.names.empty() ? 0 : static_cast<int32>(index % options.names.size()));
  }

  const auto writeBounds = [&](usize axis, std::string_view sectionName) {
    if(options.bounds_counts[axis] == 0)
    {
      return;
    }
    if(axis == 0 && options.bad_x_header_count)
    {
      output << "xcrd invalid-count" << options.line_ending;
    }
    else
    {
      output << sectionName << "     " << options.bounds_counts[axis] << options.line_ending;
    }

    std::vector<std::string> tokens;
    tokens.reserve(fixture.bounds[axis].size());
    for(usize index = 0; index < fixture.bounds[axis].size(); index++)
    {
      if(axis == 0 && options.x_bound_token.has_value() && index == 1)
      {
        tokens.push_back(options.x_bound_token.value());
      }
      else
      {
        tokens.push_back(fmt::format("{:.8E}", fixture.bounds[axis][index]));
      }
    }
    WriteTokens(output, tokens, 6, options.line_ending);
  };

  const auto writeNames = [&] {
    if(!options.write_name_section)
    {
      return;
    }
    output << "name        " << options.declared_name_count.value_or(options.names.size()) << options.line_ending;
    WriteTokens(output, options.names, options.names_one_per_line || options.names.empty() ? 1 : options.names.size(), options.line_ending);
  };

  output << "hedr         0" << options.line_ending;
  output << "info         1" << options.line_ending;
  if(options.order == FixtureOrder::Standard)
  {
    writeBounds(0, "xcrd");
    writeBounds(1, "ycrd");
    writeBounds(2, "zcrd");
    output << "keypoints" << options.line_ending << "2 2 2" << options.line_ending;
    output << "divisions" << options.line_ending << "1 1 1" << options.line_ending;
    writeNames();
  }
  else
  {
    writeBounds(2, "zcrd");
    writeNames();
    output << "divisions" << options.line_ending << "1 1 1" << options.line_ending;
    writeBounds(0, "xcrd");
    output << "keypoints" << options.line_ending << "2 2 2" << options.line_ending;
    writeBounds(1, "ycrd");
  }

  if(!options.write_matr_section)
  {
    return fixture;
  }

  output << "matr        " << options.declared_matr_count.value_or(numCells) << options.line_ending;
  std::vector<std::string> materialTokens;
  materialTokens.reserve(fixture.material_values.size());
  for(usize index = 0; index < fixture.material_values.size(); index++)
  {
    materialTokens.push_back(options.non_numeric_material_index == index ? "not-an-integer" : fmt::format("{}", fixture.material_values[index]));
  }
  for(usize index = 0; index < materialTokens.size(); index++)
  {
    output << materialTokens[index];
    if(index + 1 < materialTokens.size())
    {
      output << ((index + 1) % 40 == 0 ? options.line_ending : " ");
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
    if(expectedFixture.bounds[axis].empty())
    {
      REQUIRE(bounds.getNumberOfTuples() == 2);
      REQUIRE(bounds[0] == k_FallbackOrigin[axis]);
      REQUIRE(bounds[1] == k_FallbackOrigin[axis] + k_FallbackSpacing[axis]);
    }
    else
    {
      REQUIRE(bounds.getNumberOfTuples() == expectedFixture.bounds[axis].size());
      REQUIRE(std::equal(bounds.cbegin(), bounds.cend(), expectedFixture.bounds[axis].cbegin()));
    }
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(k_MaterialNamesPath));
  const auto& names = dataStructure.getDataRefAs<StringArray>(k_MaterialNamesPath);
  REQUIRE(std::equal(names.cbegin(), names.cend(), expectedFixture.names.cbegin()));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  REQUIRE(featureIds.getTupleShape() == ShapeType{expectedDimensions[2], expectedDimensions[1], expectedDimensions[0]});
  REQUIRE(std::equal(featureIds.cbegin(), featureIds.cend(), expectedFixture.material_values.cbegin()));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

const CreateRectGridGeometryAction& GetGeometryAction(const IFilter::PreflightResult& preflightResult)
{
  REQUIRE(preflightResult.outputActions.valid());
  REQUIRE_FALSE(preflightResult.outputActions.value().actions.empty());
  const auto* actionPtr = dynamic_cast<const CreateRectGridGeometryAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  return *actionPtr;
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
  options.bounds_counts = counts;
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / fmt::format("{}.flxtbl", label);
  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, expectedDimensions);
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Names One Per Line", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  FixtureOptions options;
  options.bounds_counts = {4, 3, 2};
  options.names_one_per_line = true;
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "NamesOnePerLine.flxtbl";
  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, {3, 2, 1});
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Sections In Different Order", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  FixtureOptions options;
  options.bounds_counts = {4, 3, 2};
  options.order = FixtureOrder::ZNameXThenY;
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "DifferentOrder.flxtbl";
  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, {3, 2, 1});
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Matr Count Mismatch Warns", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  FixtureOptions options;
  options.bounds_counts = {4, 3, 2};
  options.declared_matr_count = 5;
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
  options.bounds_counts = {4, 3, 2};
  options.written_matr_count = 5;
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
    options.bounds_counts = {4, 3, 2};
    if(testName == "Non Numeric Bound")
    {
      options.x_bound_token = "not-a-number";
    }
    options.bad_x_header_count = testName == "Bad Header Count";
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
  const std::string testName = GENERATE("CRLF Line Endings", "Empty Names");
  CAPTURE(testName);

  FixtureOptions options;
  options.bounds_counts = {4, 3, 2};
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / fmt::format("{}.flxtbl", testName);
  if(testName == "CRLF Line Endings")
  {
    options.line_ending = "\r\n";
  }
  else if(testName == "Empty Names")
  {
    options.names.clear();
  }
  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, {3, 2, 1});
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Unsupported Extension", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  FixtureOptions options;
  options.bounds_counts = {4, 3, 2};
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "UnsupportedExtension.table";
  WriteFixture(inputFile, options);

  const DataStructure dataStructure;
  const ReadOnScaleTableFileFilter filter;
  auto preflightResult = filter.preflight(dataStructure, CreateArguments(inputFile));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == -3003);
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Malformed File", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  const std::string testName = GENERATE("Names Too Short", "Missing Matr", "Non Numeric Material", "Bound Count Too Small", "Bound Out Of Range");
  CAPTURE(testName);

  FixtureOptions options;
  options.bounds_counts = {4, 3, 2};
  if(testName == "Names Too Short")
  {
    options.names = {"first", "second"};
    options.declared_name_count = 3;
  }
  else if(testName == "Missing Matr")
  {
    options.write_matr_section = false;
  }
  else if(testName == "Non Numeric Material")
  {
    options.non_numeric_material_index = 2;
  }
  else if(testName == "Bound Count Too Small")
  {
    options.bounds_counts = {1, 3, 2};
  }
  else
  {
    options.x_bound_token = "1.0E+1000";
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
  options.bounds_counts = {4, 3, 2};
  options.declared_matr_count = 6;
  options.written_matr_count = 8;
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
  initialOptions.bounds_counts = {4, 3, 2};
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
    changedOptions.bounds_counts[0] = 5;
  }
  else if(testName == "Missing X Section")
  {
    changedOptions.bounds_counts[0] = 0;
    changedOptions.declared_matr_count = 6;
    changedOptions.written_matr_count = 6;
  }
  else
  {
    changedOptions.write_name_section = false;
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
  if(!localeGuard.Set("de_DE.UTF-8"))
  {
    SUCCEED("The de_DE.UTF-8 locale is not installed; locale-specific parsing test skipped.");
    return;
  }

  FixtureOptions options;
  options.bounds_counts = {4, 3, 2};
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "CLocaleParsing.flxtbl";
  const GeneratedFixture fixture = WriteFixture(inputFile, options);
  RequireValidImport(inputFile, fixture, {3, 2, 1});
}

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: Header Cache Refreshes", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();
  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "CacheRefresh.flxtbl";
  FixtureOptions options;
  options.bounds_counts = {4, 3, 2};
  WriteFixture(inputFile, options);

  DataStructure dataStructure;
  const ReadOnScaleTableFileFilter filter;
  const auto firstPreflight = filter.preflight(dataStructure, CreateArguments(inputFile));
  const auto& firstAction = GetGeometryAction(firstPreflight);
  REQUIRE(firstAction.xDims() == 4);
  REQUIRE(firstAction.yDims() == 3);
  REQUIRE(firstAction.zDims() == 2);

  const auto originalWriteTime = fs::last_write_time(inputFile);
  options.bounds_counts = {6, 5, 4};
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
  auto* filterListPtr = app->getFilterList();
  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";
  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadOnScaleTableFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadOnScaleTableFileFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());
      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);
      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<ReadOnScaleTableFileFilter>::uuid);

      const Arguments args = pipelineFilterPtr->getArguments();
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

TEST_CASE("SimplnxCore::ReadOnScaleTableFileFilter: validates fallback metadata", "[SimplnxCore][ReadOnScaleTableFileFilter]")
{
  const bool badSpacing = GENERATE(false, true);
  const fs::path file = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadOnScaleTableFile" / "InvalidFallback.flxtbl";
  WriteFixture(file, FixtureOptions{});
  auto args = CreateArguments(file);
  const auto key = badSpacing ? ReadOnScaleTableFileFilter::k_Spacing_Key : ReadOnScaleTableFileFilter::k_Origin_Key;
  args.insertOrAssign(key, std::vector<float32>{1, 1, badSpacing ? 0.0F : std::numeric_limits<float32>::infinity()});
  DataStructure ds;
  const auto result = ReadOnScaleTableFileFilter{}.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  const auto& error = result.outputActions.errors().front();
  CHECK(error.code == (badSpacing ? ImageGeom::k_InvalidSpacing : ImageGeom::k_InvalidOrigin));
  CHECK(error.message.find(badSpacing ? "Invalid Fallback Spacing" : "Invalid Fallback Origin") != std::string::npos);
  CHECK(error.message.find(file.string()) != std::string::npos);
  CHECK(error.message.find(badSpacing ? "Z axis must be finite and greater than zero. Received 0." : "Z axis must be finite. Received inf.") != std::string::npos);
}
