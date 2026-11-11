#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Pipeline/PlaceholderFilter.hpp"
#include "simplnx/Plugin/PluginLoader.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <utility>

using namespace nx::core;

namespace
{
class TestFilter : public IFilter
{
public:
  static constexpr Uuid k_ID = *Uuid::FromString("d01ad5c2-6897-4380-89f1-6d0760b78a22");

  TestFilter() = default;
  ~TestFilter() override = default;

  std::string name() const override
  {
    return "TestFilter";
  }

  std::string className() const override
  {
    return "TestFilter";
  }

  nx::core::Uuid uuid() const override
  {
    return k_ID;
  }

  std::string humanName() const override
  {
    return "Test Filter";
  }

  std::vector<std::string> defaultTags() const override
  {
    return {};
  }

  nx::core::Parameters parameters() const override
  {
    return Parameters();
  }

  VersionType parametersVersion() const override
  {
    return 1;
  }

  UniquePointer clone() const override
  {
    return std::make_unique<TestFilter>();
  }

protected:
  PreflightResult preflightImpl(const nx::core::DataStructure& data, const nx::core::Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override
  {
    return {};
  }
  nx::core::Result<> executeImpl(nx::core::DataStructure& data, const nx::core::Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const override
  {
    return {};
  }
};

class TestPlugin : public AbstractPlugin
{
public:
  static constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("09721f74-c282-44ac-ada7-1db1a23435dd");

  TestPlugin()
  : AbstractPlugin(k_ID, "TestPlugin", "", "")
  {
    addFilter([]() { return std::make_unique<TestFilter>(); });
  }
  ~TestPlugin() override = default;

  TestPlugin(const TestPlugin&) = delete;
  TestPlugin(TestPlugin&&) = delete;

  TestPlugin& operator=(const TestPlugin&) = delete;
  TestPlugin& operator=(TestPlugin&&) = delete;

  SIMPLMapType getSimplToSimplnxMap() const override
  {
    return {};
  }
};
} // namespace

TEST_CASE("Save Filters To Json")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterList = app->getFilterList();
  REQUIRE(filterList != nullptr);

  const auto& handles = filterList->getFilterHandles();
  REQUIRE(!handles.empty());

  for(const auto& handle : handles)
  {
    auto coreFilter = filterList->createFilter(handle);
    REQUIRE(coreFilter != nullptr);

    Arguments args;
    auto params = coreFilter->parameters();
    for(const auto& [name, param] : params)
    {
      args.insert(name, param->defaultValue());
    }

    INFO(fmt::format("Filter '{}' did not serialize to json properly!", coreFilter->humanName()))
    REQUIRE_NOTHROW(coreFilter->toJson(args));
  }
}

TEST_CASE("Save Pipeline To Json")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterList = app->getFilterList();
  REQUIRE(filterList != nullptr);

  const auto& handles = filterList->getFilterHandles();
  REQUIRE(!handles.empty());

  Pipeline pipeline;
  for(const auto& handle : handles)
  {
    auto coreFilter = filterList->createFilter(handle);
    REQUIRE(coreFilter != nullptr);

    Arguments args;
    auto params = coreFilter->parameters();
    for(const auto& [name, param] : params)
    {
      args.insert(name, param->defaultValue());
    }

    REQUIRE(pipeline.push_back(handle, args));
  }

  INFO(fmt::format("The pipeline containing every filter did not serialize to json properly!"))
  REQUIRE_NOTHROW(pipeline.toJson());
}

TEST_CASE("PlaceholderFilter")
{
  auto app = Application::GetOrCreateInstance();
  FilterList* filterList = app->getFilterList();
  filterList->addPlugin(std::make_shared<InMemoryPluginLoader>(std::make_shared<TestPlugin>()));
  REQUIRE(filterList->containsPlugin(TestPlugin::k_ID));
  auto filterHandles = filterList->getFilterHandles();
  auto testFilterIter = std::find_if(filterHandles.cbegin(), filterHandles.cend(), [](const FilterHandle& handle) { return handle.getFilterId() == TestFilter::k_ID; });
  REQUIRE(testFilterIter != filterHandles.cend());

  Pipeline pipeline;
  pipeline.insertAt(0, *testFilterIter);

  nlohmann::json pipelineJson = pipeline.toJson();

  filterList->removePlugin(TestPlugin::k_ID);
  REQUIRE(!filterList->containsPlugin(TestPlugin::k_ID));

  auto placeholderPipelineResult = Pipeline::FromJson(pipelineJson, true);
  SIMPLNX_RESULT_REQUIRE_VALID(placeholderPipelineResult);

  Pipeline placeholderPipeline = std::move(placeholderPipelineResult.value());
  REQUIRE(placeholderPipeline.size() == 1);
  REQUIRE(placeholderPipeline.containsPlaceholder());

  AbstractPipelineNode* node = placeholderPipeline.at(0);
  REQUIRE(node != nullptr);
  REQUIRE(node->getType() == AbstractPipelineNode::NodeType::Filter);

  auto* filterNode = dynamic_cast<AbstractPipelineFilter*>(node);
  REQUIRE(filterNode != nullptr);
  REQUIRE(filterNode->getFilterType() == AbstractPipelineFilter::FilterType::Placeholder);

  auto* placeholderFilter = dynamic_cast<PlaceholderFilter*>(filterNode);
  REQUIRE(placeholderFilter != nullptr);

  nlohmann::json placeholderPipelineJson = placeholderPipeline.toJson();

  REQUIRE(placeholderPipelineJson == pipelineJson);
}

TEST_CASE("Pipeline persistent state: filter deep copy", "[simplnx][PipelineCopyState]")
{
  const bool isDisabled = GENERATE(false, true);
  const bool hasFilter = GENERATE(false, true);
  CAPTURE(isDisabled, hasFilter);

  IFilter::UniquePointer filterPtr;
  if(hasFilter)
  {
    filterPtr = std::make_unique<TestFilter>();
  }
  PipelineFilter source(std::move(filterPtr));
  source.setComments("Keep this filter comment.\nKeep this second line.");
  source.setDisabled(isDisabled);

  auto copyPtr = source.deepCopy();
  auto* filterCopyPtr = dynamic_cast<PipelineFilter*>(copyPtr.get());
  REQUIRE(filterCopyPtr != nullptr);
  CHECK(filterCopyPtr != &source);
  CHECK(filterCopyPtr->isDisabled() == isDisabled);
  CHECK(filterCopyPtr->getComments() == "Keep this filter comment.\nKeep this second line.");
  CHECK(filterCopyPtr->toJson().at("isDisabled") == isDisabled);
  CHECK(filterCopyPtr->toJson().at("comments") == "Keep this filter comment.\nKeep this second line.");
  if(hasFilter)
  {
    REQUIRE(filterCopyPtr->getFilter() != nullptr);
    CHECK(filterCopyPtr->getFilter() != source.getFilter());
    CHECK(filterCopyPtr->getFilter()->uuid() == TestFilter::k_ID);
  }
  else
  {
    CHECK(filterCopyPtr->getFilter() == nullptr);
    CHECK(filterCopyPtr->toJson().at("filter").at("name") == "UnknownFilter");
  }

  filterCopyPtr->setDisabled(!isDisabled);
  filterCopyPtr->setComments("Copy comment changed independently.");
  CHECK(source.isDisabled() == isDisabled);
  CHECK(source.getComments() == "Keep this filter comment.\nKeep this second line.");
  CHECK(source.toJson().at("isDisabled") == isDisabled);
}

TEST_CASE("Pipeline persistent state: deep copy and segments", "[simplnx][PipelineCopyState]")
{
  const bool isDisabled = GENERATE(false, true);
  CAPTURE(isDisabled);
  Pipeline source("Persistent state source");
  source.setDisabled(isDisabled);

  auto firstPtr = std::make_shared<PipelineFilter>(std::make_unique<TestFilter>());
  firstPtr->setDisabled(true);
  firstPtr->setComments("First filter stays disabled.");
  REQUIRE(source.push_back(firstPtr));

  auto nestedPtr = std::make_shared<Pipeline>("Nested source");
  nestedPtr->setDisabled(true);
  auto nestedFilterPtr = std::make_shared<PipelineFilter>(std::make_unique<TestFilter>());
  nestedFilterPtr->setComments("Nested filter stays enabled.");
  REQUIRE(nestedPtr->push_back(nestedFilterPtr));
  REQUIRE(source.push_back(nestedPtr));

  auto lastPtr = std::make_shared<PipelineFilter>(nullptr);
  lastPtr->setDisabled(true);
  lastPtr->setComments("Null filter stays disabled.");
  REQUIRE(source.push_back(lastPtr));

  std::unique_ptr<AbstractPipelineNode> copyPtr;
  usize expectedSize = 0;
  SECTION("deep copy includes every node")
  {
    copyPtr = source.deepCopy();
    expectedSize = 3;
  }
  SECTION("mutable segment excludes a valid end iterator")
  {
    copyPtr = source.copySegment(source.begin(), source.begin() + 2);
    expectedSize = 2;
  }
  SECTION("const segment includes a valid end iterator")
  {
    const Pipeline& sourceRef = source;
    copyPtr = sourceRef.copySegment(sourceRef.begin(), sourceRef.begin() + 1);
    expectedSize = 2;
  }
  SECTION("mutable segment reaches the end sentinel")
  {
    copyPtr = source.copySegment(source.begin(), source.end());
    expectedSize = 3;
  }
  SECTION("const segment reaches the end sentinel")
  {
    const Pipeline& sourceRef = source;
    copyPtr = sourceRef.copySegment(sourceRef.begin(), sourceRef.end());
    expectedSize = 3;
  }
  SECTION("mutable equal iterators give an empty segment")
  {
    copyPtr = source.copySegment(source.begin(), source.begin());
  }
  SECTION("const equal valid iterators give one node")
  {
    const Pipeline& sourceRef = source;
    copyPtr = sourceRef.copySegment(sourceRef.begin(), sourceRef.begin());
    expectedSize = 1;
  }
  SECTION("const equal end sentinels give an empty segment")
  {
    const Pipeline& sourceRef = source;
    copyPtr = sourceRef.copySegment(sourceRef.end(), sourceRef.end());
  }

  auto* pipelineCopyPtr = dynamic_cast<Pipeline*>(copyPtr.get());
  REQUIRE(pipelineCopyPtr != nullptr);
  REQUIRE(pipelineCopyPtr->size() == expectedSize);
  CHECK(pipelineCopyPtr->getName() == "Persistent state source");
  CHECK(pipelineCopyPtr->isDisabled() == isDisabled);
  CHECK(pipelineCopyPtr->toJson().at("isDisabled") == isDisabled);
  CHECK(pipelineCopyPtr->getParentPipeline() == nullptr);

  if(expectedSize > 0)
  {
    auto* firstCopyPtr = dynamic_cast<PipelineFilter*>(pipelineCopyPtr->at(0));
    REQUIRE(firstCopyPtr != nullptr);
    CHECK(firstCopyPtr != firstPtr.get());
    CHECK(firstCopyPtr->getParentPipeline() == pipelineCopyPtr);
    CHECK(firstCopyPtr->isDisabled());
    CHECK(firstCopyPtr->getComments() == "First filter stays disabled.");
    CHECK(pipelineCopyPtr->toJson().at("pipeline").at(0).at("comments") == "First filter stays disabled.");
    firstCopyPtr->setDisabled(false);
    firstCopyPtr->setComments("Copied first filter changed.");
  }
  if(expectedSize > 1)
  {
    auto* nestedCopyPtr = dynamic_cast<Pipeline*>(pipelineCopyPtr->at(1));
    REQUIRE(nestedCopyPtr != nullptr);
    REQUIRE(nestedCopyPtr->size() == 1);
    CHECK(nestedCopyPtr != nestedPtr.get());
    CHECK(nestedCopyPtr->isDisabled());
    CHECK(nestedCopyPtr->getName() == "Nested source");
    CHECK(nestedCopyPtr->getParentPipeline() == pipelineCopyPtr);
    auto* nestedFilterCopyPtr = dynamic_cast<PipelineFilter*>(nestedCopyPtr->at(0));
    REQUIRE(nestedFilterCopyPtr != nullptr);
    CHECK(nestedFilterCopyPtr != nestedFilterPtr.get());
    CHECK(nestedFilterCopyPtr->getParentPipeline() == nestedCopyPtr);
    CHECK_FALSE(nestedFilterCopyPtr->isDisabled());
    CHECK(nestedFilterCopyPtr->getComments() == "Nested filter stays enabled.");
    nestedCopyPtr->setDisabled(false);
    nestedFilterCopyPtr->setDisabled(true);
    nestedFilterCopyPtr->setComments("Copied nested filter changed.");
  }
  if(expectedSize > 2)
  {
    auto* lastCopyPtr = dynamic_cast<PipelineFilter*>(pipelineCopyPtr->at(2));
    REQUIRE(lastCopyPtr != nullptr);
    CHECK(lastCopyPtr != lastPtr.get());
    CHECK(lastCopyPtr->getFilter() == nullptr);
    CHECK(lastCopyPtr->isDisabled());
    CHECK(lastCopyPtr->getComments() == "Null filter stays disabled.");
    lastCopyPtr->setDisabled(false);
    lastCopyPtr->setComments("Copied null filter changed.");
  }
  pipelineCopyPtr->setDisabled(!isDisabled);
  CHECK(source.isDisabled() == isDisabled);
  CHECK(firstPtr->isDisabled());
  CHECK(firstPtr->getComments() == "First filter stays disabled.");
  CHECK(nestedPtr->isDisabled());
  CHECK_FALSE(nestedFilterPtr->isDisabled());
  CHECK(nestedFilterPtr->getComments() == "Nested filter stays enabled.");
  CHECK(lastPtr->isDisabled());
  CHECK(lastPtr->getComments() == "Null filter stays disabled.");
  CHECK(firstPtr->getParentPipeline() == &source);
  CHECK(nestedPtr->getParentPipeline() == &source);
}

TEST_CASE("Pipeline persistent state: value operations", "[simplnx][PipelineCopyState]")
{
  const bool isDisabled = GENERATE(false, true);
  CAPTURE(isDisabled);
  Pipeline source("Value source");
  source.setDisabled(isDisabled);
  auto childPtr = std::make_shared<PipelineFilter>(std::make_unique<TestFilter>());
  childPtr->setDisabled(true);
  childPtr->setComments("Value operations keep shared children.");
  REQUIRE(source.push_back(childPtr));

  std::unique_ptr<Pipeline> destinationPtr;
  bool isCopy = false;
  SECTION("copy constructor")
  {
    destinationPtr = std::make_unique<Pipeline>(source);
    isCopy = true;
  }
  SECTION("move constructor")
  {
    destinationPtr = std::make_unique<Pipeline>(std::move(source));
  }
  SECTION("copy assignment replaces the destination flag")
  {
    destinationPtr = std::make_unique<Pipeline>("Old destination");
    destinationPtr->setDisabled(!isDisabled);
    *destinationPtr = source;
    isCopy = true;
  }
  SECTION("move assignment replaces the destination flag")
  {
    destinationPtr = std::make_unique<Pipeline>("Old destination");
    destinationPtr->setDisabled(!isDisabled);
    *destinationPtr = std::move(source);
  }

  REQUIRE(destinationPtr != nullptr);
  REQUIRE(destinationPtr->size() == 1);
  CHECK(destinationPtr->getName() == "Value source");
  CHECK(destinationPtr->isDisabled() == isDisabled);
  CHECK(destinationPtr->toJson().at("isDisabled") == isDisabled);
  CHECK(destinationPtr->at(0) == childPtr.get());
  CHECK(childPtr->isDisabled());
  CHECK(childPtr->getComments() == "Value operations keep shared children.");
  destinationPtr->setDisabled(!isDisabled);
  if(isCopy)
  {
    REQUIRE(source.size() == 1);
    CHECK(source.at(0) == childPtr.get());
    CHECK(source.isDisabled() == isDisabled);
  }
}

TEST_CASE("Pipeline persistent state: placeholder JSON loading", "[simplnx][PipelineCopyState]")
{
  const bool isDisabled = GENERATE(false, true);
  CAPTURE(isDisabled);
  FilterList filterList;
  filterList.addPlugin(std::make_shared<InMemoryPluginLoader>(std::make_shared<TestPlugin>()));

  Pipeline source("Unavailable filter source", &filterList);
  auto filterPtr = std::make_shared<PipelineFilter>(std::make_unique<TestFilter>());
  filterPtr->setDisabled(isDisabled);
  filterPtr->setComments("Keep unavailable filter settings.");
  REQUIRE(source.push_back(filterPtr));
  const nlohmann::json savedJson = source.toJson();
  filterList.removePlugin(TestPlugin::k_ID);

  auto loadedResult = Pipeline::FromJson(savedJson, &filterList, true);
  SIMPLNX_RESULT_REQUIRE_VALID(loadedResult);
  auto& loadedRef = loadedResult.value();
  REQUIRE(loadedRef.size() == 1);
  auto* placeholderPtr = dynamic_cast<PlaceholderFilter*>(loadedRef.at(0));
  REQUIRE(placeholderPtr != nullptr);
  CHECK(placeholderPtr->isDisabled() == isDisabled);
  CHECK(placeholderPtr->toJson().at("isDisabled") == isDisabled);
  CHECK(placeholderPtr->getJson().at("comments") == "Keep unavailable filter settings.");
  CHECK(loadedRef.toJson() == savedJson);

  auto directPtr = PlaceholderFilter::Create(savedJson.at("pipeline").at(0));
  REQUIRE(directPtr != nullptr);
  CHECK(directPtr->isDisabled() == isDisabled);
  CHECK(directPtr->toJson().at("isDisabled") == isDisabled);
}

TEST_CASE("Pipeline persistent state: placeholder copies current disabled state", "[simplnx][PipelineCopyState]")
{
  const bool savedDisabled = GENERATE(false, true);
  CAPTURE(savedDisabled);
  const nlohmann::json savedJson = {{"filter", {{"name", "UnavailableFilter"}, {"uuid", "d01ad5c2-6897-4380-89f1-6d0760b78a22"}}},
                                    {"args", nlohmann::json::object()},
                                    {"comments", "Keep the original unavailable filter JSON."},
                                    {"isDisabled", savedDisabled}};
  PlaceholderFilter source(savedJson);
  source.setDisabled(!savedDisabled);

  auto copyPtr = source.deepCopy();
  auto* placeholderCopyPtr = dynamic_cast<PlaceholderFilter*>(copyPtr.get());
  REQUIRE(placeholderCopyPtr != nullptr);
  CHECK(placeholderCopyPtr != &source);
  CHECK(placeholderCopyPtr->isDisabled() == !savedDisabled);
  CHECK(placeholderCopyPtr->getJson() == savedJson);
  CHECK(placeholderCopyPtr->toJson().at("isDisabled") == !savedDisabled);
  CHECK(placeholderCopyPtr->toJson().at("comments") == "Keep the original unavailable filter JSON.");
  CHECK(source.isDisabled() == !savedDisabled);

  placeholderCopyPtr->setDisabled(savedDisabled);
  CHECK(source.isDisabled() == !savedDisabled);
  CHECK(source.getJson() == savedJson);
  CHECK(source.toJson().at("isDisabled") == !savedDisabled);
}

TEST_CASE("Pipeline persistent state: placeholder keeps malformed disabled payload", "[simplnx][PipelineCopyState]")
{
  const nlohmann::json disabledJson = GENERATE(nlohmann::json("not-a-boolean"), nlohmann::json(nullptr));
  CAPTURE(disabledJson);
  const nlohmann::json savedJson = {
      {"filter", {{"name", "Raw unavailable filter"}}}, {"args", nlohmann::json::object()}, {"comments", "Keep this incomplete filter payload."}, {"isDisabled", disabledJson}};

  SECTION("direct placeholder keeps the raw payload and defaults to enabled")
  {
    std::unique_ptr<PlaceholderFilter> placeholderPtr;
    REQUIRE_NOTHROW(placeholderPtr = PlaceholderFilter::Create(savedJson));
    REQUIRE(placeholderPtr != nullptr);
    CHECK_FALSE(placeholderPtr->isDisabled());
    CHECK(placeholderPtr->getJson() == savedJson);
    CHECK(placeholderPtr->toJson().at("comments") == "Keep this incomplete filter payload.");
    CHECK(placeholderPtr->toJson().at("isDisabled") == false);
  }
  SECTION("pipeline fallback keeps malformed filter JSON")
  {
    FilterList filterList;
    const nlohmann::json pipelineJson = {{"name", "Raw fallback pipeline"}, {"pipeline", nlohmann::json::array({savedJson})}};
    auto loadedResult = Pipeline::FromJson(pipelineJson, &filterList, true);
    SIMPLNX_RESULT_REQUIRE_VALID(loadedResult);
    auto& loadedRef = loadedResult.value();
    REQUIRE(loadedRef.size() == 1);
    auto* placeholderPtr = dynamic_cast<PlaceholderFilter*>(loadedRef.at(0));
    REQUIRE(placeholderPtr != nullptr);
    CHECK_FALSE(placeholderPtr->isDisabled());
    CHECK(placeholderPtr->getJson() == savedJson);
    CHECK(placeholderPtr->toJson().at("comments") == "Keep this incomplete filter payload.");
    CHECK(placeholderPtr->toJson().at("isDisabled") == false);
  }
}
