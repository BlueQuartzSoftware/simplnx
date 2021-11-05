#include "ExampleFilter1.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";
} // namespace

namespace complex
{
std::string ExampleFilter1::name() const
{
  return FilterTraits<ExampleFilter1>::name;
}

std::string ExampleFilter1::className() const
{
  return FilterTraits<ExampleFilter1>::className;
}

Uuid ExampleFilter1::uuid() const
{
  return FilterTraits<ExampleFilter1>::uuid;
}

std::string ExampleFilter1::humanName() const
{
  return "Example Filter 1";
}

Parameters ExampleFilter1::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<Float32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0.1234f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Param2, "Use Value", "The 2nd parameter", true));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_Param3, "Parameter 3", "MultiDataArraySelection Parameter", MultiArraySelectionParameter::ValueType{}));

  params.linkParameters(k_Param2, k_Param1, true);
  return params;
}

IFilter::UniquePointer ExampleFilter1::clone() const
{
  return std::make_unique<ExampleFilter1>();
}

IFilter::PreflightResult ExampleFilter1::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}

Result<> ExampleFilter1::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  return {};
}
} // namespace complex
