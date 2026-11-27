#include "SimplnxCore/Filters/CreateAttributeMatrixFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
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

TEST_CASE("SimplnxCore::CreateAttributeMatrixFilter(Instantiate)", "[SimplnxCore][CreateAttributeMatrixFilter]")
{
  unit_test::LoadPlugins();

  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType kTupleDims = {{static_cast<double>(k_NumTuples)}};
  static const DataPath kDataPath({"foo"});

  CreateAttributeMatrixFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insert(CreateAttributeMatrixFilter::k_DataObjectPath, std::make_any<DataPath>(kDataPath));
  args.insert(CreateAttributeMatrixFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(kTupleDims));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateAttributeMatrixFilter(Invalid Parameters)", "[SimplnxCore][CreateAttributeMatrixFilter]")
{
  unit_test::LoadPlugins();

  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType kTupleDims = {{static_cast<double>(k_NumTuples)}};

  CreateAttributeMatrixFilter filter;
  DataStructure dataStructure;
  Arguments args;

  SECTION("Section 1")
  {
    args.insert(CreateAttributeMatrixFilter::k_DataObjectPath, std::make_any<DataPath>(DataPath{}));
    args.insert(CreateAttributeMatrixFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(kTupleDims));
  }

  SECTION("Section 2")
  {
    AttributeMatrix* attMat1Ptr = AttributeMatrix::Create(dataStructure, "AttributeMatrix1", {1ULL});
    args.insert(CreateAttributeMatrixFilter::k_DataObjectPath, std::make_any<DataPath>(DataPath({"AttributeMatrix1", "AttributeMatrix2"})));
    args.insert(CreateAttributeMatrixFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(kTupleDims));
  }

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateAttributeMatrixFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CreateAttributeMatrixFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateAttributeMatrixFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CreateAttributeMatrixFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<CreateAttributeMatrixFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      // Complex type (DynamicTableFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(CreateAttributeMatrixFilter::k_DataObjectPath) == DataPath({"DataContainer", "CellData"}));
    }
  }
}

TEST_CASE("SimplnxCore::CreateAttributeMatrix: Dimension RED fractional", "[SimplnxCore][CreateAttributeMatrix][DimensionRed]")
{
  CreateAttributeMatrixFilter filter;
  DataStructure dataStructure;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreateAttributeMatrixFilter::k_DataObjectPath, std::make_any<DataPath>(DataPath({"Dimensions"})));
  args.insertOrAssign(CreateAttributeMatrixFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.5}}));
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE(result.output_actions.errors()[0].code == -79000);
}

TEST_CASE("SimplnxCore::CreateAttributeMatrix: Dimension fixed-only boundaries and zero", "[SimplnxCore][CreateAttributeMatrix]")
{
  const float64 value = GENERATE(-1.0, 1.5, std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity(), std::ldexp(1.0, 64), 0.0);
  CreateAttributeMatrixFilter filter;
  DataStructure dataStructure;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreateAttributeMatrixFilter::k_DataObjectPath, std::make_any<DataPath>(DataPath({"Dimensions"})));
  args.insertOrAssign(CreateAttributeMatrixFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{value}}));
  auto result = filter.preflight(dataStructure, args);
  if(value == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
    auto execute = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Dimensions"})));
    REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Dimensions"})).getShape() == ShapeType{0});
    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors()[0].code == -79000);
    REQUIRE(result.output_actions.errors()[0].message.find("index 0") != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::CreateAttributeMatrix: Dimension product and zero order", "[SimplnxCore][CreateAttributeMatrix]")
{
  const bool zero = GENERATE(false, true);
  CreateAttributeMatrixFilter filter;
  DataStructure dataStructure;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreateAttributeMatrixFilter::k_DataObjectPath, std::make_any<DataPath>(DataPath({"Dimensions"})));
  args.insertOrAssign(CreateAttributeMatrixFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(zero ? DynamicTableParameter::ValueType{{4294967296.0, 4294967296.0, 0}} :
                                                                                                                           DynamicTableParameter::ValueType{{4294967296.0, 4294967296.0}}));
  auto result = filter.preflight(dataStructure, args);
  if(zero)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors()[0].code == -79001);
    REQUIRE(result.output_actions.errors()[0].message.find("product") != std::string::npos);
  }
}
