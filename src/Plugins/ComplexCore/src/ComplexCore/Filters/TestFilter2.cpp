#include "TestFilter2.hpp"

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

namespace complex
{
std::string TestFilter2::name() const
{
  return FilterTraits<TestFilter2>::name;
}

Uuid TestFilter2::uuid() const
{
  return FilterTraits<TestFilter2>::uuid;
}

std::string TestFilter2::humanName() const
{
  return "Test Filter 2";
}

Parameters TestFilter2::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<Int32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0));
  params.insert(std::make_unique<StringParameter>(k_Param2, "Parameter 2", "The 2nd parameter", "test string"));
  params.insert(std::make_unique<ChoicesParameter>(k_Param3, "Parameter 3", "The 3rd parameter", 0, ChoicesParameter::Choices{"foo", "bar", "baz"}));
  return params;
}

IFilter::UniquePointer TestFilter2::clone() const
{
  return std::make_unique<TestFilter2>();
}

Result<OutputActions> TestFilter2::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}

Result<> TestFilter2::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}
} // namespace complex
