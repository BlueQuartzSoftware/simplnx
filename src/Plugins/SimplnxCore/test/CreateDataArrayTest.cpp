#include "SimplnxCore/Filters/CreateDataArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include <cmath>
#include <limits>

using namespace nx::core;
namespace fs = std::filesystem;

TEST_CASE("SimplnxCore::CreateDataArrayFilter(Instantiate)", "[SimplnxCore][CreateDataArrayFilter]")
{
  UnitTest::LoadPlugins();

  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};

  static const DataPath k_DataPath({"foo"});

  CreateDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(k_NComp));
  args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
  args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateDataArrayFilter(Invalid Parameters)", "[SimplnxCore][CreateDataArrayFilter]")
{
  UnitTest::LoadPlugins();

  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};
  static const DataPath k_DataPath({"foo"});

  CreateDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  SECTION("Section1")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint16));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(k_NComp));
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("-1"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section2")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(k_NComp));
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("1024"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section3")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(0));
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section4")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));

    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(0.0)}};
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section5")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));
    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(1.0)}};
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>(""));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section6")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));
    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(1.0)}};
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("1000"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint8));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("-1"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int16));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("70000"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint16));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("-1"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("4294967297"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("-4294967297"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateDataArrayFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CreateDataArrayFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateDataArrayFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CreateDataArrayFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CreateDataArrayFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (ScalarTypeParameterToNumericTypeConverter) - verified by successful pipeline loading
      CHECK(args.value<uint64>(CreateDataArrayFilter::k_NumComps_Key) == 5);
      CHECK(args.value<std::string>(CreateDataArrayFilter::k_InitializationValue_Key) == "TestName");
      // Complex type (DataArrayCreationFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}

TEST_CASE("SimplnxCore::CreateDataArray: Dimension RED fractional", "[SimplnxCore][CreateDataArray][DimensionRed]")
{
  CreateDataArrayFilter filter;
  DataStructure ds;
  auto args = filter.getDefaultArguments();

  args.insertOrAssign(CreateDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"Dimensions"})));
  args.insertOrAssign(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.5}}));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors()[0].code == -79000);
}

TEST_CASE("SimplnxCore::CreateDataArray: Dimension RED zero components", "[SimplnxCore][CreateDataArray][DimensionRed]")
{
  CreateDataArrayFilter filter;
  DataStructure ds;
  auto args = filter.getDefaultArguments();

  args.insertOrAssign(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(0));
  args.insertOrAssign(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"ZeroComponents"})));
  args.insertOrAssign(CreateDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1}}));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors()[0].code == -79003);
}

TEST_CASE("SimplnxCore::CreateDataArray: Dimension fixed-only tuples and inherited shapes", "[SimplnxCore][CreateDataArray]")
{
  CreateDataArrayFilter filter;
  SECTION("Consumed explicit dimensions")
  {
    const float64 value = GENERATE(-1.0, std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity(), std::ldexp(1.0, 64), 0.0, 1.5);
    CAPTURE(value);
    DataStructure ds;
    auto args = filter.getDefaultArguments();

    args.insertOrAssign(CreateDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
    args.insertOrAssign(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"Dimensions"})));
    args.insertOrAssign(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{value}}));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors()[0].code == (value == 0 ? -78603 : -79000));
    REQUIRE(result.outputActions.errors()[0].message.find("index 0") != std::string::npos);
    REQUIRE(result.outputActions.errors()[0].message.find("finite whole numbers") != std::string::npos);
  }
  SECTION("Invalid ignored table, including an inherited empty matrix")
  {
    const usize count = GENERATE(usize{0}, usize{2});
    const bool useDimensions = GENERATE(false, true);
    DataStructure ds;
    REQUIRE(AttributeMatrix::Create(ds, "Parent", {count}) != nullptr);
    auto args = filter.getDefaultArguments();

    const DataPath path({"Parent", "Dimensions"});
    args.insertOrAssign(CreateDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(useDimensions));
    args.insertOrAssign(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(path));
    args.insertOrAssign(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{-1}}));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    auto applied = result.outputActions.value().applyAll(ds, IDataAction::Mode::Preflight);
    SIMPLNX_RESULT_REQUIRE_VALID(applied);
    REQUIRE_NOTHROW(ds.getDataRefAs<IArray>(path));
    REQUIRE(ds.getDataRefAs<IArray>(path).getTupleShape() == ShapeType{count});
    UnitTest::CheckArraysInheritTupleDims(ds);
  }
}

TEST_CASE("SimplnxCore::CreateDataArray: Dimension tuple product overflow", "[SimplnxCore][CreateDataArray]")
{
  CreateDataArrayFilter filter;
  DataStructure ds;
  auto args = filter.getDefaultArguments();

  args.insertOrAssign(CreateDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"Overflow"})));
  args.insertOrAssign(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{4294967296.0, 4294967296.0}}));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors()[0].code == -79001);
  REQUIRE(result.outputActions.errors()[0].message.find("product") != std::string::npos);
}
