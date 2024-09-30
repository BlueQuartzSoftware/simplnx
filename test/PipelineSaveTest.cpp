#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PlaceholderFilter.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

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
  PreflightResult preflightImpl(const nx::core::DataStructure& data, const nx::core::Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override
  {
    return {};
  }
  nx::core::Result<> executeImpl(nx::core::DataStructure& data, const nx::core::Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                 const std::atomic_bool& shouldCancel) const override
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
  app->loadPlugins(unit_test::k_BuildDir.view());
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
  app->loadPlugins(unit_test::k_BuildDir.view());
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
