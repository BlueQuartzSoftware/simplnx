#include "catch2/catch.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Filter//FilterHandle.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"

#include "complex/unit_test/complex_test_dirs.hpp"

#include <filesystem>
#include <iostream>
#include <typeinfo>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

using namespace complex;

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
} // namespace

TEST_CASE("Execute Pipeline")
{
  Application app;
  app.loadPlugins(unit_test::k_BuildDir.view());
  auto filterList = app.getFilterList();

  const AbstractPlugin* test1Plugin = app.getPlugin(k_Test1PluginId);
  REQUIRE(test1Plugin != nullptr);
  auto testFilter1 = filterList->createFilter(k_Test1FilterHandle);
  REQUIRE(testFilter1 != nullptr);
  FilterHandle tf1Handle(testFilter1->uuid(), test1Plugin->getId());

  const AbstractPlugin* test2Plugin = app.getPlugin(k_Test2PluginId);
  REQUIRE(test2Plugin != nullptr);
  auto testFilter2 = filterList->createFilter(k_Test2FilterHandle);
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
  app.loadPlugins(unit_test::k_BuildDir.view());

  auto filterList = app.getFilterList();
  REQUIRE(filterList->size() != 0);

  const AbstractPlugin* test1Plugin = app.getPlugin(k_Test1PluginId);
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

TEST_CASE("PipelineJson")
{
  Application app;
  app.loadPlugins(unit_test::k_BuildDir.view());

  Pipeline pipeline("test");

  Arguments filter1Args;
  filter1Args.insert("param1", 1.2f);
  filter1Args.insert("param2", true);
  filter1Args.insert("param3", GeneratedFileListParameter::ValueType{});
  pipeline.push_back(k_Test1FilterHandle, filter1Args);

  Arguments filter2Args;
  filter2Args.insert("param1", 42);
  filter2Args.insert("param2", std::string("foobarbaz"));
  filter2Args.insert("param3", ChoicesParameter::ValueType{});
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
