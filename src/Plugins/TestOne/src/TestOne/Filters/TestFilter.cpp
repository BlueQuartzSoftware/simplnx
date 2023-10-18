#include "TestFilter.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

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
complex::Uuid TestFilter::uuid() const
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
complex::Parameters TestFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<Float32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0.1234f));
  params.insert(std::make_unique<BoolParameter>(k_Param2, "Parameter 2", "The 2nd parameter", false));
  params.insert(std::make_unique<GeneratedFileListParameter>(
      k_Param3, "Input File List", "The values that are used to generate the input file list. See GeneratedFileListParameter for more information.", GeneratedFileListParameter::ValueType{}));
  return params;
}

//------------------------------------------------------------------------------
complex::IFilter::UniquePointer TestFilter::clone() const
{
  return std::make_unique<TestFilter>();
}

//------------------------------------------------------------------------------
complex::IFilter::PreflightResult TestFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}

//------------------------------------------------------------------------------
complex::Result<> TestFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  return {};
}
