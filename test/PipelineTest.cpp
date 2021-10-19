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
// TestOne Plugin
constexpr StringLiteral k_TestOnePluginId = "01ff618b-781f-4ac0-b9ac-43f26ce1854f";
constexpr StringLiteral k_TestFilterId = "5502c3f7-37a8-4a86-b003-1c856be02491";
const FilterHandle k_TestFilterHandle(Uuid::FromString(k_TestFilterId.str()).value(), Uuid::FromString(k_TestOnePluginId.str()).value());

// TestTwo Plugin
constexpr StringLiteral k_TestTwoPluginId = "05cc618b-781f-4ac0-b9ac-43f33ce1854e";
constexpr StringLiteral k_Test2FilterId = "ad9cf22b-bc5e-41d6-b02e-bb49ffd12c04";
const FilterHandle k_Test2FilterHandle(Uuid::FromString(k_Test2FilterId.str()).value(), Uuid::FromString(k_TestTwoPluginId.str()).value());

//
const FilterHandle k_CreateDataGroupHandle(Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ExportH5DataFilterHandle(Uuid::FromString("b3a95784-2ced-11ec-8d3d-0242ac130003").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ExampleFilter2Handle(Uuid::FromString("1307bbbc-112d-4aaa-941f-58253787b17e").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ImportTextFilterHandle(Uuid::FromString("25f7df3e-ca3e-4634-adda-732c0e56efd4").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_CreateDataArrayHandle(Uuid::FromString("67041f9b-bdc6-4122-acc6-c9fe9280e90d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ExampleFilter1Handle(Uuid::FromString("dd92896b-26ec-4419-b905-567e93e8f39d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());

} // namespace Constants
} // namespace complex

using namespace complex;

TEST_CASE("Execute Pipeline")
{
  Application app;
  fs::path pluginPath = fmt::format("{}/{}/", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
  app.loadPlugins(pluginPath, false);
  auto filterList = app.getFilterList();

  AbstractPlugin* test1Plugin = app.getPlugin("TestOne");
  REQUIRE(test1Plugin != nullptr);
  auto testFilter1 = filterList->createFilter(complex::Constants::k_TestFilterHandle);
  REQUIRE(testFilter1 != nullptr);
  FilterHandle tf1Handle(testFilter1->uuid(), test1Plugin->getId());

  AbstractPlugin* test2Plugin = app.getPlugin("TestTwo");
  REQUIRE(test2Plugin != nullptr);
  auto testFilter2 = filterList->createFilter(complex::Constants::k_Test2FilterHandle);
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
  app.loadPlugins(pluginPath, false);

  auto filterList = app.getFilterList();
  REQUIRE(filterList->size() != 0);

  AbstractPlugin* test1Plugin = app.getPlugin("TestOne");
  REQUIRE(test1Plugin != nullptr);

  auto testFilter1 = filterList->createFilter(complex::Constants::k_TestFilterHandle);
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
#endif

TEST_CASE("PipelineJson")
{
  Application app;
  fs::path pluginPath = fmt::format("{}/{}/", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
  app.loadPlugins(pluginPath, false);

  Pipeline pipeline("test");

  Arguments filter1Args;
  filter1Args.insert("param1", 1.2f);
  filter1Args.insert("param2", true);
  filter1Args.insert("param3", GeneratedFileListParameter::ValueType{});
  pipeline.push_back(Constants::k_TestFilterHandle, filter1Args);

  Arguments filter2Args;
  filter2Args.insert("param1", 42);
  filter2Args.insert("param2", std::string("foobarbaz"));
  filter2Args.insert("param3", ChoicesParameter::ValueType{});
  pipeline.push_back(Constants::k_Test2FilterHandle, filter2Args);

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
