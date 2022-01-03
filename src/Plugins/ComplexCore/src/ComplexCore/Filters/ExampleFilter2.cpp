#include "ExampleFilter2.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
//#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";
constexpr StringLiteral k_Param4 = "param4";
constexpr StringLiteral k_Param5 = "param5";
constexpr StringLiteral k_Param6 = "param6";
constexpr StringLiteral k_Param7 = "param7";
constexpr StringLiteral k_Param8 = "param8";
constexpr StringLiteral k_Param9 = "param9";
constexpr StringLiteral k_Param10 = "param10";
constexpr StringLiteral k_Param11 = "param11";
constexpr StringLiteral k_Param12 = "param12";
constexpr StringLiteral k_Param13 = "param13";
constexpr StringLiteral k_Param14 = "param14";
constexpr StringLiteral k_Param15 = "param15";
constexpr StringLiteral k_Param16 = "param16";
} // namespace

namespace complex
{
std::string ExampleFilter2::name() const
{
  return FilterTraits<ExampleFilter2>::name;
}

std::string ExampleFilter2::className() const
{
  return FilterTraits<ExampleFilter2>::className;
}

Uuid ExampleFilter2::uuid() const
{
  return FilterTraits<ExampleFilter2>::uuid;
}

std::string ExampleFilter2::humanName() const
{
  return "Example Filter 2";
}

Parameters ExampleFilter2::parameters() const
{
  Parameters params;
  params.insertSeparator({"1rst Group of Parameters"});

  params.insertSeparator({"2nd Group of Parameters"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_Param5, "Array Creation", "", ArrayCreationParameter::ValueType{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_Param6, "Array Selection", "", ArraySelectionParameter::ValueType{}));

  params.insertSeparator({"3rd Group of Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Param7, "Bool Parameter", "", true));
  params.insert(std::make_unique<ChoicesParameter>(k_Param3, "ChoicesParameter", "", 0, ChoicesParameter::Choices{"foo", "bar", "baz"}));

  params.insertSeparator({"4th Group of Parameters"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_Param8, "DataGroupCreationParameter", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_Param9, "DataGroupSelectionParameter", "", DataPath{}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_Param10, "DataPathSelectionParameter", "", DataPath{}));

  params.insert(std::make_unique<GeometrySelectionParameter>(k_Param11, "GeometrySelectionParameter", "", DataPath{}, std::set<DataObject::Type>{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_Param12, "MultiArraySelectionParameter", "", MultiArraySelectionParameter::ValueType{}));

  return params;
}

IFilter::UniquePointer ExampleFilter2::clone() const
{
  return std::make_unique<ExampleFilter2>();
}

IFilter::PreflightResult ExampleFilter2::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}

Result<> ExampleFilter2::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineFilter, const MessageHandler& messageHandler) const
{
  return {};
}
} // namespace complex
