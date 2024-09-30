#include "Test2Filter.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

using namespace nx::core;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3_index";
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
nx::core::Uuid Test2Filter::uuid() const
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
nx::core::Parameters Test2Filter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<Int32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0));
  params.insert(std::make_unique<StringParameter>(k_Param2, "Parameter 2", "The 2nd parameter", "test string"));
  params.insert(std::make_unique<ChoicesParameter>(k_Param3, "Parameter 3", "The 3rd parameter", 0, ChoicesParameter::Choices{"foo", "bar", "baz"}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType Test2Filter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
nx::core::IFilter::UniquePointer Test2Filter::clone() const
{
  return std::make_unique<Test2Filter>();
}

//------------------------------------------------------------------------------
nx::core::IFilter::PreflightResult Test2Filter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  return {};
}

//------------------------------------------------------------------------------
nx::core::Result<> Test2Filter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  return {};
}
