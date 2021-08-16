#include "TestFilter2.hpp"

#include "complex/Core/Parameters/ChoicesParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/Core/Parameters/StringParameter.hpp"

using namespace complex;

namespace
{
constexpr const char k_Param1[] = "param1";
constexpr const char k_Param2[] = "param2";
constexpr const char k_Param3[] = "param3";
} // namespace

namespace complex
{

TestFilter2::TestFilter2()
{
    m_params.insert(std::make_unique<Int32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0));
    m_params.insert(std::make_unique<StringParameter>(k_Param2, "Parameter 2", "The 2nd parameter", "test string"));
    m_params.insert(std::make_unique<ChoicesParameter>(k_Param3, "Parameter 3", "The 3rd parameter", 0, ChoicesParameter::Choices{"foo", "bar", "baz"}));
}

std::string TestFilter2::name() const
{
  return "TestFilter2";
}

Uuid TestFilter2::uuid() const
{
  constexpr Uuid uuid = *Uuid::FromString("1307bbbc-112d-4aaa-941f-58253787b17e");
  return uuid;
}

std::string TestFilter2::humanName() const
{
  return "Test Filter 2";
}

Parameters const& TestFilter2::parameters() const
{
  return m_params;
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
