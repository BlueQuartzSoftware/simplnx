#include "catch2/catch.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/Core/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"

#include <typeinfo>

#include <nlohmann/json.hpp>

namespace complex
{

namespace Constants
{
const FilterHandle k_ImportTextHandle(*Uuid::FromString("25f7df3e-ca3e-4634-adda-732c0e56efd4"), Uuid());
const FilterHandle k_FilterHandle1(*Uuid::FromString("dd92896b-26ec-4419-b905-567e93e8f39d"), Uuid());
const FilterHandle k_FilterHandle2(*Uuid::FromString("1307bbbc-112d-4aaa-941f-58253787b17e"), Uuid());
const FilterHandle k_BadHandle(Uuid{}, Uuid{});
} // namespace Constants

} // namespace complex

using namespace complex;

TEST_CASE("Execute Pipeline")
{
  Application app;
  auto filterList = app.getFilterList();

  Pipeline pipeline;
  REQUIRE(pipeline.execute());
  REQUIRE(pipeline.push_front(Constants::k_FilterHandle1));
  REQUIRE(pipeline.execute());
  REQUIRE(!pipeline.push_back(Constants::k_BadHandle));
}

TEST_CASE("Complex Pipeline")
{
  Application app;

  FilterHandle handle = Constants::k_FilterHandle1;

  Pipeline pipeline;
  PipelineFilter* node = PipelineFilter::Create(handle);
  REQUIRE(node != nullptr);
  REQUIRE(node->getParameters().empty() == false);
  REQUIRE(pipeline.push_back(node));

  auto segment2 = new Pipeline();
  segment2->push_back(handle);
  REQUIRE(pipeline.push_back(segment2));
}

Pipeline CreatePipeline()
{
  Pipeline pipeline;
  auto node = PipelineFilter::Create(Constants::k_FilterHandle1);
  pipeline.push_back(node);
  pipeline.push_back(Constants::k_FilterHandle2);

  // Add additional segment to the main pipeline
  auto segment = new Pipeline();
  segment->push_back(Constants::k_ImportTextHandle);
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
