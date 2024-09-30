#include "TestFilter.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";
} // namespace

TestFilter::TestFilter() = default;

TestFilter::~TestFilter() = default;

//------------------------------------------------------------------------------
std::string TestFilter::name() const
{
  return FilterTraits<TestFilter>::name;
}

//------------------------------------------------------------------------------
std::string TestFilter::className() const
{
  return FilterTraits<TestFilter>::className;
}

//------------------------------------------------------------------------------
nx::core::Uuid TestFilter::uuid() const
{
  return FilterTraits<TestFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string TestFilter::humanName() const
{
  return "Test Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> TestFilter::defaultTags() const
{
  return {className(), "Example", "Test"};
}

//------------------------------------------------------------------------------
nx::core::Parameters TestFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<Float32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0.1234f));
  params.insert(std::make_unique<BoolParameter>(k_Param2, "Parameter 2", "The 2nd parameter", false));
  params.insert(std::make_unique<GeneratedFileListParameter>(
      k_Param3, "Input File List", "The values that are used to generate the input file list. See GeneratedFileListParameter for more information.", GeneratedFileListParameter::ValueType{}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType TestFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
nx::core::IFilter::UniquePointer TestFilter::clone() const
{
  return std::make_unique<TestFilter>();
}

//------------------------------------------------------------------------------
nx::core::IFilter::PreflightResult TestFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  return {};
}

//------------------------------------------------------------------------------
nx::core::Result<> TestFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  return {};
}
