#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"
#include "simplnx/Plugin/PluginLoader.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <typeinfo>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const FilterHandle k_BadHandle(Uuid{}, Uuid{});
// TestOne Plugin
constexpr Uuid k_Test1PluginId = *Uuid::FromString("01ff618b-781f-4ac0-b9ac-43f26ce1854f");
constexpr Uuid k_Test1FilterId = *Uuid::FromString("5502c3f7-37a8-4a86-b003-1c856be02491");
const FilterHandle k_Test1FilterHandle(k_Test1FilterId, k_Test1PluginId);

// TestTwo Plugin
constexpr Uuid k_Test2PluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f33ce1854e");
constexpr Uuid k_Test2FilterId = *Uuid::FromString("ad9cf22b-bc5e-41d6-b02e-bb49ffd12c04");
const FilterHandle k_Test2FilterHandle(k_Test2FilterId, k_Test2PluginId);

const Uuid k_CreateDataGroupId = *Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101");
const Uuid k_CorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const FilterHandle k_CreateDataGroupHandle(k_CreateDataGroupId, k_CorePluginId);

const DataPath k_DeferredActionPath({"foo"});

constexpr Uuid k_LegacyConversionPluginId = *Uuid::FromString("254739dd-c1c4-40a7-bcf8-58e8b79b8bdb");
constexpr Uuid k_LegacyConversionFilterId = *Uuid::FromString("477e43df-5a63-42bf-8b06-1c2407c8b67d");
constexpr Uuid k_LegacyConversionSimplId = *Uuid::FromString("a5b7ca67-2aeb-54be-8d60-17d29e47a497");
constexpr StringLiteral k_LegacyValueKey = "legacy_value";
constexpr float32 k_LegacyDefaultValue = 42.0F;
constexpr float32 k_LegacyConvertedValue = 7.0F;
constexpr int32 k_LegacyConversionError = -7654;
constexpr int32 k_LegacyConversionWarning = -7655;

class LegacyConversionTestFilter : public IFilter
{
public:
  std::string name() const override
  {
    return "LegacyConversionTestFilter";
  }

  std::string className() const override
  {
    return "LegacyConversionTestFilter";
  }

  Uuid uuid() const override
  {
    return k_LegacyConversionFilterId;
  }

  std::string humanName() const override
  {
    return "Legacy Conversion Test Filter";
  }

  std::vector<std::string> defaultTags() const override
  {
    return {};
  }

  Parameters parameters() const override
  {
    Parameters params;
    params.insert(std::make_unique<Float32Parameter>(k_LegacyValueKey, "Legacy Value", "A value used to verify legacy conversion behavior.", k_LegacyDefaultValue));
    return params;
  }

  VersionType parametersVersion() const override
  {
    return 1;
  }

  UniquePointer clone() const override
  {
    return std::make_unique<LegacyConversionTestFilter>();
  }

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override
  {
    return {};
  }

  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override
  {
    return {};
  }
};

Result<Arguments> ConvertLegacyTestFilter(const nlohmann::json& json)
{
  const std::string mode = json.at("ConversionMode").get<std::string>();
  if(mode == "invalid")
  {
    return MakeErrorResult<Arguments>(k_LegacyConversionError, "The test converter rejected its legacy parameters.");
  }
  if(mode == "json_exception")
  {
    static_cast<void>(json.at("MissingLegacyValue"));
  }
  if(mode == "std_exception")
  {
    throw std::runtime_error("The test converter threw a standard exception.");
  }

  Arguments args = LegacyConversionTestFilter().getDefaultArguments();
  args.insertOrAssign(k_LegacyValueKey, std::make_any<float32>(k_LegacyConvertedValue));
  Result<Arguments> result{std::move(args)};
  if(mode == "warning")
  {
    result.warnings().push_back(Warning{k_LegacyConversionWarning, "The test converter accepted a tolerated legacy value."});
  }
  return result;
}

class LegacyConversionTestPlugin : public AbstractPlugin
{
public:
  LegacyConversionTestPlugin()
  : AbstractPlugin(k_LegacyConversionPluginId, "LegacyConversionTestPlugin", "", "")
  {
    addFilter([]() { return std::make_unique<LegacyConversionTestFilter>(); });
  }

  SIMPLMapType getSimplToSimplnxMap() const override
  {
    return {{k_LegacyConversionSimplId, {k_LegacyConversionFilterId, ConvertLegacyTestFilter}}};
  }
};

nlohmann::json CreateLegacyConversionPipeline(std::string_view mode, bool includeHumanName = true)
{
  nlohmann::json filterJson = {{"Filter_Uuid", k_LegacyConversionSimplId.str()}, {"Filter_Name", "LegacyConversionTestFilter"}, {"ConversionMode", mode}};
  if(includeHumanName)
  {
    filterJson["Filter_Human_Label"] = "Legacy Conversion Human Name";
  }

  return {{"PipelineBuilder", {{"Name", "Legacy Conversion Test Pipeline"}, {"Number_Filters", 1}, {"Version", 6}}}, {"0", std::move(filterJson)}};
}

FilterList CreateLegacyConversionFilterList()
{
  FilterList filterList;
  auto addPluginResult = filterList.addPlugin(std::make_shared<InMemoryPluginLoader>(std::make_shared<LegacyConversionTestPlugin>()));
  if(addPluginResult.invalid())
  {
    throw std::runtime_error(addPluginResult.errors().front().message);
  }
  return filterList;
}

class DeferredActionTestFilter : public IFilter
{
public:
  DeferredActionTestFilter() = default;

  ~DeferredActionTestFilter() noexcept override = default;

  DeferredActionTestFilter(const DeferredActionTestFilter&) = delete;
  DeferredActionTestFilter(DeferredActionTestFilter&&) noexcept = delete;

  DeferredActionTestFilter& operator=(const DeferredActionTestFilter&) = delete;
  DeferredActionTestFilter& operator=(DeferredActionTestFilter&&) noexcept = delete;

  std::string name() const override
  {
    return "DeferredActionTestFilter";
  }

  std::string className() const override
  {
    return "DeferredActionTestFilter";
  }

  Uuid uuid() const override
  {
    static constexpr Uuid uuid = *Uuid::FromString("907b9eb7-c48e-4666-9d9d-21cc70f426f1");
    return uuid;
  }

  std::string humanName() const override
  {
    return "Deferred Action Test Filter";
  }

  Parameters parameters() const override
  {
    return {};
  }

  VersionType parametersVersion() const override
  {
    return 1;
  }

  UniquePointer clone() const override
  {
    return std::make_unique<DeferredActionTestFilter>();
  }

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override
  {
    OutputActions outputActions;
    outputActions.appendAction(std::make_unique<CreateArrayAction>(DataType::int32, std::vector<usize>{10}, std::vector<usize>{1}, k_DeferredActionPath));
    outputActions.appendDeferredAction(std::make_unique<DeleteDataAction>(k_DeferredActionPath));
    return {std::move(outputActions)};
  }

  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override
  {
    // object should exist because the delete should happen after execute
    if(dataStructure.getData(k_DeferredActionPath) == nullptr)
    {
      return MakeErrorResult(-1, fmt::format("DataPath '{}' must exist", k_DeferredActionPath.toString()));
    }
    return {};
  }
};
} // namespace

TEST_CASE("PipelineTest: Invalid SIMPL conversion creates a visible unknown filter")
{
  FilterList filterList = CreateLegacyConversionFilterList();
  const nlohmann::json pipelineJson = CreateLegacyConversionPipeline("invalid");

  Result<std::unique_ptr<PipelineFilter>> filterResult = PipelineFilter::FromSIMPLJson(pipelineJson["0"], filterList);
  REQUIRE(filterResult.invalid());
  REQUIRE(filterResult.errors().size() == 1);
  CHECK(filterResult.errors()[0].code == k_LegacyConversionError);
  CHECK(filterResult.errors()[0].message.find("Legacy Conversion Human Name") != std::string::npos);
  CHECK(filterResult.errors()[0].message.find("The test converter rejected its legacy parameters.") != std::string::npos);

  Result<Pipeline> pipelineResult = Pipeline::FromSIMPLJson(pipelineJson, &filterList);
  SIMPLNX_RESULT_REQUIRE_VALID(pipelineResult);

  Pipeline& pipeline = pipelineResult.value();
  REQUIRE(pipeline.size() == 1);
  auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
  REQUIRE(pipelineFilter != nullptr);
  REQUIRE(pipelineFilter->getFilter() == nullptr);
  REQUIRE(pipelineFilter->getArguments().empty());
  REQUIRE_FALSE(LegacyConversionTestFilter().getDefaultArguments().empty());
  CHECK(pipelineFilter->getComments().find("Legacy Conversion Human Name") != std::string::npos);
  CHECK(pipelineFilter->getComments().find("The test converter rejected its legacy parameters.") != std::string::npos);
}

TEST_CASE("PipelineTest: SIMPL conversion warnings reach the pipeline caller")
{
  FilterList filterList = CreateLegacyConversionFilterList();
  const nlohmann::json pipelineJson = CreateLegacyConversionPipeline("warning");

  Result<std::unique_ptr<PipelineFilter>> filterResult = PipelineFilter::FromSIMPLJson(pipelineJson["0"], filterList);
  SIMPLNX_RESULT_REQUIRE_VALID(filterResult);
  REQUIRE(filterResult.warnings().size() == 1);
  CHECK(filterResult.warnings()[0].code == k_LegacyConversionWarning);
  CHECK(filterResult.warnings()[0].message == "The test converter accepted a tolerated legacy value.");

  Result<Pipeline> pipelineResult = Pipeline::FromSIMPLJson(pipelineJson, &filterList);
  SIMPLNX_RESULT_REQUIRE_VALID(pipelineResult);

  REQUIRE(pipelineResult.warnings().size() == 1);
  CHECK(pipelineResult.warnings()[0].code == k_LegacyConversionWarning);
  CHECK(pipelineResult.warnings()[0].message == "The test converter accepted a tolerated legacy value.");

  auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipelineResult.value().at(0));
  REQUIRE(pipelineFilter != nullptr);
  REQUIRE(pipelineFilter->getFilter() != nullptr);
  CHECK(pipelineFilter->getArguments().value<float32>(k_LegacyValueKey) == k_LegacyConvertedValue);
}

TEST_CASE("PipelineTest: SIMPL converter exceptions create a visible unknown filter")
{
  FilterList filterList = CreateLegacyConversionFilterList();

  SECTION("nlohmann JSON exception names the legacy human name")
  {
    Result<Pipeline> pipelineResult;
    REQUIRE_NOTHROW(pipelineResult = Pipeline::FromSIMPLJson(CreateLegacyConversionPipeline("json_exception"), &filterList));
    SIMPLNX_RESULT_REQUIRE_VALID(pipelineResult);

    auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipelineResult.value().at(0));
    REQUIRE(pipelineFilter != nullptr);
    REQUIRE(pipelineFilter->getFilter() == nullptr);
    CHECK(pipelineFilter->getComments().find("Legacy Conversion Human Name") != std::string::npos);
    CHECK(pipelineFilter->getComments().find("MissingLegacyValue") != std::string::npos);
  }

  SECTION("standard exception falls back to the legacy UUID")
  {
    Result<Pipeline> pipelineResult;
    REQUIRE_NOTHROW(pipelineResult = Pipeline::FromSIMPLJson(CreateLegacyConversionPipeline("std_exception", false), &filterList));
    SIMPLNX_RESULT_REQUIRE_VALID(pipelineResult);

    auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipelineResult.value().at(0));
    REQUIRE(pipelineFilter != nullptr);
    REQUIRE(pipelineFilter->getFilter() == nullptr);
    CHECK(pipelineFilter->getComments().find(k_LegacyConversionSimplId.str()) != std::string::npos);
    CHECK(pipelineFilter->getComments().find("The test converter threw a standard exception.") != std::string::npos);
  }
}

TEST_CASE("PipelineTest:Execute Pipeline")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const AbstractPlugin* test1Plugin = app->getPlugin(k_Test1PluginId);
  REQUIRE(test1Plugin != nullptr);
  auto testFilter1 = filterList->createFilter(k_Test1FilterHandle);
  REQUIRE(testFilter1 != nullptr);
  FilterHandle tf1Handle(testFilter1->uuid(), test1Plugin->getId());

  const AbstractPlugin* test2Plugin = app->getPlugin(k_Test2PluginId);
  REQUIRE(test2Plugin != nullptr);
  auto testFilter2 = filterList->createFilter(k_Test2FilterHandle);
  REQUIRE(testFilter2 != nullptr);
  FilterHandle tf2Handle(testFilter2->uuid(), test2Plugin->getId());

  Pipeline pipeline;
  REQUIRE(pipeline.execute());
  REQUIRE(pipeline.push_front(tf1Handle));

  PipelineFilter* filterNode = dynamic_cast<PipelineFilter*>(pipeline.at(0));
  REQUIRE(filterNode != nullptr);

  Arguments args;
  GeneratedFileListParameter::ValueType defaultValue;
  defaultValue.startIndex = 0;
  defaultValue.endIndex = 1;
  defaultValue.filePrefix = "test_";
  defaultValue.fileExtension = ".txt";
  defaultValue.inputPath = nx::core::unit_test::k_BinaryTestOutputDir;

  const std::vector<std::string> fileList = defaultValue.generate();
  // We need to actually have files available for the test to pass so generate some files
  for(const auto& filepath : fileList)
  {
    std::fstream file(filepath, std::ios_base::out);
    file << "TEST" << std::endl;
  }

  args.insert("param3", std::move(defaultValue));
  filterNode->setArguments(args);

  REQUIRE(pipeline.execute());
  REQUIRE(pipeline.push_back(tf2Handle));
}

TEST_CASE("PipelineTest:Simplnx Pipeline")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  auto filterList = app->getFilterList();
  REQUIRE(filterList->size() != 0);

  const AbstractPlugin* test1Plugin = app->getPlugin(k_Test1PluginId);
  REQUIRE(test1Plugin != nullptr);

  auto testFilter1 = filterList->createFilter(k_Test1FilterHandle);
  REQUIRE(testFilter1 != nullptr);
  FilterHandle tf1Handle(testFilter1->uuid(), test1Plugin->getId());

  Pipeline pipeline;
  std::unique_ptr<PipelineFilter> node = PipelineFilter::Create(tf1Handle);
  REQUIRE(node != nullptr);
  REQUIRE(node->getParameters().empty() == false);
  REQUIRE(pipeline.push_back(std::move(node)));
  REQUIRE(pipeline.push_back(k_Test1FilterHandle));
  REQUIRE(pipeline.push_back(k_Test2FilterHandle));

  auto segment2 = std::make_shared<Pipeline>();
  segment2->push_back(tf1Handle);
  REQUIRE(pipeline.push_back(segment2));

  // Test Moving nodex
  {
    constexpr Pipeline::index_type fromIndex = 1;
    constexpr Pipeline::index_type toIndex = 3;

    auto movedNode = pipeline.at(fromIndex);

    REQUIRE(pipeline.move(fromIndex, toIndex));

    // Check node index
    {
      auto nodeIter = pipeline.find(movedNode);
      const auto nodeIndex = nodeIter - pipeline.begin();
      REQUIRE(nodeIndex == toIndex);
    }

    REQUIRE(pipeline.move(toIndex, fromIndex));

    // Check node index
    {
      auto nodeIter = pipeline.find(movedNode);
      const auto nodeIndex = nodeIter - pipeline.begin();
      REQUIRE(nodeIndex == fromIndex);
    }
  }
}

TEST_CASE("PipelineTest:PipelineJson")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  Pipeline pipeline("test");

  Arguments filter1Args;
  filter1Args.insert("param1", 1.2f);
  filter1Args.insert("param2", true);
  filter1Args.insert("param3", GeneratedFileListParameter::ValueType{});
  pipeline.push_back(k_Test1FilterHandle, filter1Args);

  Arguments filter2Args;
  filter2Args.insert("param1", 42);
  filter2Args.insert("param2", std::string("foobarbaz"));
  filter2Args.insert("param3_index", ChoicesParameter::ValueType{});
  pipeline.push_back(k_Test2FilterHandle, filter2Args);

  nlohmann::json pipelineJson = pipeline.toJson();

  Result<Pipeline> pipeline2Result = Pipeline::FromJson(pipelineJson);

  REQUIRE(pipeline2Result.valid());

  Pipeline pipeline2 = std::move(pipeline2Result.value());

  REQUIRE(pipeline2.getName() == pipeline.getName());
  REQUIRE(pipeline2.size() == pipeline.size());
  usize size = pipeline2.size();
  for(usize i = 0; i < size; i++)
  {
    const auto* originalNode = dynamic_cast<const PipelineFilter*>(pipeline.at(i));

    const AbstractPipelineNode* node = pipeline2.at(i);
    REQUIRE(node != nullptr);
    const auto* pipelineFilter = dynamic_cast<const PipelineFilter*>(node);
    REQUIRE(pipelineFilter != nullptr);
    const IFilter* filter = pipelineFilter->getFilter();
    REQUIRE(filter != nullptr);

    Uuid expectedUuid = originalNode->getFilter()->uuid();
    REQUIRE(filter->uuid() == expectedUuid);

    Arguments expectedArgs = originalNode->getArguments();

    Arguments args = pipelineFilter->getArguments();

    REQUIRE(args.size() == expectedArgs.size());

    for(const auto& [key, expectedValue] : expectedArgs)
    {
      REQUIRE(args.contains(key));
      const std::any& value = args.at(key);
      REQUIRE(value.has_value());
      REQUIRE(value.type() == expectedValue.type());
    }
  }

  nlohmann::json pipeline2Json = pipeline2.toJson();

  REQUIRE(pipelineJson == pipeline2Json);
}

TEST_CASE("PipelineTest:Rename Output")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  std::string group1Name = "Foo";
  std::string group2Name = "Bar";

  Arguments args1;
  args1.insert("data_object_path", std::make_any<DataPath>(DataPath({group1Name})));

  Arguments args2;
  args2.insert("data_object_path", std::make_any<DataPath>(DataPath({group1Name, group2Name})));

  Pipeline pipeline("Rename Test Pipeline");
  REQUIRE(pipeline.push_back(k_CreateDataGroupHandle, args1));
  REQUIRE(pipeline.push_back(k_CreateDataGroupHandle, args2));

  REQUIRE(pipeline.preflight());

  // Update arguments to test renaming
  {
    group1Name = "Bizz";
    args1.insertOrAssign("data_object_path", std::make_any<DataPath>(DataPath({group1Name})));

    auto* filterNode = dynamic_cast<PipelineFilter*>(pipeline.at(0));
    REQUIRE(filterNode != nullptr);
    filterNode->setArguments(args1);
  }

  REQUIRE(pipeline.preflight(false, true));

  // Check Node 2 arguments
  {
    auto* filterNode2 = dynamic_cast<PipelineFilter*>(pipeline.at(1));
    REQUIRE(filterNode2 != nullptr);
    auto args2Alt = filterNode2->getArguments();
    const auto& inPath = std::any_cast<DataPath>(args2Alt.at("data_object_path"));
    DataPath targetPath({group1Name, group2Name});
    REQUIRE(inPath == targetPath);
  }
}

TEST_CASE("PipelineTest:Not Renaming")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  std::string group1Name = "Foo";
  std::string group2Name = "Bar";

  Arguments args1;
  args1.insert("data_object_path", std::make_any<DataPath>(DataPath({group1Name})));

  Arguments args2;
  args2.insert("data_object_path", std::make_any<DataPath>(DataPath({group1Name, group2Name})));

  Pipeline pipeline("Rename Test Pipeline");
  REQUIRE(pipeline.push_back(k_CreateDataGroupHandle, args1));
  REQUIRE(pipeline.push_back(k_CreateDataGroupHandle, args2));

  REQUIRE(pipeline.preflight());

  // Update arguments to test renaming
  {
    group1Name = "Bizz";
    args1.insertOrAssign("data_object_path", std::make_any<DataPath>(DataPath({group1Name})));

    auto* filterNode = dynamic_cast<PipelineFilter*>(pipeline.at(0));
    REQUIRE(filterNode != nullptr);
    filterNode->setArguments(args1);
  }

  REQUIRE(pipeline.preflight(false, false));

  // Check Node 2 arguments
  {
    auto* filterNode2 = dynamic_cast<PipelineFilter*>(pipeline.at(1));
    REQUIRE(filterNode2 != nullptr);
    auto args2Alt = filterNode2->getArguments();
    const auto& inPath = std::any_cast<DataPath>(args2Alt.at("data_object_path"));
    DataPath targetPath({group1Name, group2Name});
    REQUIRE(inPath != targetPath);
  }
}

TEST_CASE("PipelineTest:PipelineDeferredActionTest")
{
  Pipeline pipeline;

  REQUIRE(pipeline.push_back(std::make_unique<DeferredActionTestFilter>()));

  DataStructure dataStructure;

  REQUIRE(pipeline.preflight(dataStructure, false));

  // should be deleted in preflight
  DataObject* preflightObject = dataStructure.getData(k_DeferredActionPath);
  REQUIRE(preflightObject == nullptr);

  // DeferredActionTestFilter checks the k_DeferredActionPath hasn't been deleted until after execute
  REQUIRE(pipeline.execute(dataStructure, false));

  // after execute the object will be deleted
  DataObject* executeObject = dataStructure.getData(k_DeferredActionPath);
  REQUIRE(executeObject == nullptr);
}
