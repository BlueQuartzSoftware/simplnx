#include "ExampleFilter2.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";
constexpr StringLiteral k_Param4 = "param4";

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
  params.insertSeparator({"First Group of Parameters"});
  params.insert(std::make_unique<Int32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0));
  params.insert(std::make_unique<StringParameter>(k_Param2, "Parameter 2", "The 2nd parameter", "test string"));
  params.insertSeparator({"Second Group of Parameters"});
  params.insert(std::make_unique<ChoicesParameter>(k_Param3, "Parameter 3", "The 3rd parameter", 0, ChoicesParameter::Choices{"foo", "bar", "baz"}));
  params.insert(std::make_unique<GeneratedFileListParameter>(k_Param4, "Input File List", "Data needed to generate the input file list", GeneratedFileListParameter::ValueType{}));

  return params;
}

IFilter::UniquePointer ExampleFilter2::clone() const
{
  return std::make_unique<ExampleFilter2>();
}

Result<OutputActions> ExampleFilter2::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}

Result<> ExampleFilter2::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}
} // namespace complex
