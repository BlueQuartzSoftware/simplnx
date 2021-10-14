#include "catch2/catch.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/Core/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"

#include "complex/unit_test/complex_test_dirs.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

#include <typeinfo>

#include <nlohmann/json.hpp>

namespace complex
{
namespace Constants
{
const FilterHandle k_BadHandle(Uuid{}, Uuid{});
} // namespace Constants
} // namespace complex

using namespace complex;

void PrintAllFilters()
{
  std::cout << "#---------------------------------------------------------------------------" << std::endl;
  Application* app = Application::Instance();
  FilterList* filterList = app->getFilterList();
  std::cout << "Filter Count: " << filterList->getFilterHandles().size() << std::endl;

  std::unordered_set<FilterHandle> filterHandles = filterList->getFilterHandles();

  std::cout << "----- Available Filters --------" << std::endl;
  Pipeline pipeline;
  for(const auto& filterHandle : filterHandles)
  {
    FilterHandle::PluginIdType pluginId = filterHandle.getPluginId();
    AbstractPlugin* pluginPtr = filterList->getPluginById(pluginId);
    if(nullptr != pluginPtr)
    {
      std::cout << pluginPtr->getName() << "\t" << filterHandle.getFilterName() << std::endl;
    }
    pipeline.push_back(filterHandle);
  }
}

TEST_CASE("Execute Pipeline")
{
  Application app;
  fs::path pluginPath = fmt::format("{}/{}/", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
  app.loadPlugins(pluginPath);
  auto filterList = app.getFilterList();

  AbstractPlugin* test1Plugin = app.getPlugin("TestOne");
  REQUIRE(test1Plugin != nullptr);
  auto testFilter1 = filterList->createFilter("TestFilter");
  REQUIRE(testFilter1 != nullptr);
  FilterHandle tf1Handle(testFilter1->uuid(), test1Plugin->getId());

  AbstractPlugin* test2Plugin = app.getPlugin("TestTwo");
  REQUIRE(test2Plugin != nullptr);
  auto testFilter2 = filterList->createFilter("Test2Filter");
  REQUIRE(testFilter2 != nullptr);
  FilterHandle tf2Handle(testFilter2->uuid(), test2Plugin->getId());

  Pipeline pipeline;
  REQUIRE(pipeline.execute());
  REQUIRE(pipeline.push_front(tf1Handle));
  REQUIRE(pipeline.execute());
  REQUIRE(pipeline.push_back(tf2Handle));
}

TEST_CASE("Complex Pipeline")
{
  Application app;
  fs::path pluginPath = fmt::format("{}/{}/", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
  app.loadPlugins(pluginPath);
  auto filterList = app.getFilterList();
  AbstractPlugin* test1Plugin = app.getPlugin("TestOne");
  REQUIRE(test1Plugin != nullptr);

  auto testFilter1 = filterList->createFilter("TestFilter");
  REQUIRE(testFilter1 != nullptr);
  FilterHandle tf1Handle(testFilter1->uuid(), test1Plugin->getId());

  Pipeline pipeline;
  PipelineFilter* node = PipelineFilter::Create(tf1Handle);
  REQUIRE(node != nullptr);
  REQUIRE(node->getParameters().empty() == false);
  REQUIRE(pipeline.push_back(node));

  auto segment2 = new Pipeline();
  segment2->push_back(tf1Handle);
  REQUIRE(pipeline.push_back(segment2));
}

#if 0
Pipeline CreatePipeline()
{
  Application app;
  app.loadPlugins(fs::path(complex::unit_test::k_PluginDir));
  auto filterList = app.getFilterList();


  AbstractPlugin* test1Plugin = app.getPlugin("TestOne");
  REQUIRE(test1Plugin != nullptr);
  auto testFilter1 = filterList->createFilter("TestFilter");
  REQUIRE(testFilter1 != nullptr);
  FilterHandle tf1Handle (testFilter1->uuid(), test1Plugin->getId());

  AbstractPlugin* test2Plugin = app.getPlugin("TestTwo");
  REQUIRE(test2Plugin != nullptr);
  auto testFilter2 = filterList->createFilter("Test2Filter");
  REQUIRE(testFilter2 != nullptr);
  FilterHandle tf2Handle (testFilter2->uuid(), test2Plugin->getId());


  Pipeline pipeline;
  auto node = PipelineFilter::Create(tf1Handle);
  pipeline.push_back(node);
  pipeline.push_back(tf2Handle);

  // Add additional segment to the main pipeline
  auto segment = new Pipeline();
  segment->push_back(tf1Handle);
  pipeline.push_back(segment);

  // Set Filter1
  {
    auto parameters = node->getParameters();
    Arguments args;
    args.insert("param1", 0.5f);
    args.insert("param2", false);
    node->setArguments(args);
  }
  // Set Filter2
  {
    auto node2 = dynamic_cast<PipelineFilter*>(pipeline[1]);
    auto parameters = node2->getParameters();
    Arguments args;
    args.insert("param1", 5);
    args.insert("param2", "Bar");
    args.insert("param3", 1);
    node2->setArguments(args);
  }

  REQUIRE(pipeline.execute());

  return pipeline;
}

TEST_CASE("PipelineJson")
{
  Application app;

  Pipeline pipeline("test");

  Arguments filter1Args;
  filter1Args.insert("param1", 1.2f);
  filter1Args.insert("param2", true);
  filter1Args.insert("param3", GeneratedFileListParameter::ValueType{});
  pipeline.push_back(Constants::k_FilterHandle1, filter1Args);

  Arguments filter2Args;
  filter2Args.insert("param1", 42);
  filter2Args.insert("param2", std::string("foobarbaz"));
  filter2Args.insert("param3", std::make_any<uint64>(13));
  pipeline.push_back(Constants::k_FilterHandle2, filter2Args);

  nlohmann::json pipelineJson = pipeline.toJson();

  Pipeline pipeline2 = Pipeline::FromJson(pipelineJson);

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
