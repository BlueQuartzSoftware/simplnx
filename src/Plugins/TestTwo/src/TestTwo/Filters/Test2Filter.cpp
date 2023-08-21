#include "Test2Filter.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";
} // namespace

Test2Filter::Test2Filter() = default;

Test2Filter::~Test2Filter() = default;

//------------------------------------------------------------------------------
std::string Test2Filter::name() const
{
  return FilterTraits<Test2Filter>::name;
}

//------------------------------------------------------------------------------
std::string Test2Filter::className() const
{
  return FilterTraits<Test2Filter>::className;
}

//------------------------------------------------------------------------------
complex::Uuid Test2Filter::uuid() const
{
  return FilterTraits<Test2Filter>::uuid;
}

//------------------------------------------------------------------------------
std::string Test2Filter::humanName() const
{
  return "Test Filter 2";
}

//------------------------------------------------------------------------------
std::vector<std::string> Test2Filter::defaultTags() const
{
  return {className(), "Example", "Test"};
}

//------------------------------------------------------------------------------
complex::Parameters Test2Filter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<Int32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0));
  params.insert(std::make_unique<StringParameter>(k_Param2, "Parameter 2", "The 2nd parameter", "test string"));
  params.insert(std::make_unique<ChoicesParameter>(k_Param3, "Parameter 3", "The 3rd parameter", 0, ChoicesParameter::Choices{"foo", "bar", "baz"}));
  return params;
}

//------------------------------------------------------------------------------
complex::IFilter::UniquePointer Test2Filter::clone() const
{
  return std::make_unique<Test2Filter>();
}

//------------------------------------------------------------------------------
complex::IFilter::PreflightResult Test2Filter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}

//------------------------------------------------------------------------------
complex::Result<> Test2Filter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  return {};
}
