#include "FilterPreflightMessage.hpp"

#include "simplnx/Filter/Output.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"

using namespace nx::core;

FilterPreflightMessage::FilterPreflightMessage(PipelineFilter* filterNode, const std::vector<nx::core::Warning>& warnings, const std::vector<nx::core::Error>& errors)
: AbstractPipelineMessage(filterNode)
, m_Warnings(warnings)
, m_Errors(errors)
{
}

FilterPreflightMessage::~FilterPreflightMessage() = default;

nx::core::PipelineFilter* FilterPreflightMessage::getFilterNode() const
{
  return dynamic_cast<PipelineFilter*>(getNode());
}

std::vector<nx::core::Warning> FilterPreflightMessage::getWarnings() const
{
  return m_Warnings;
}

std::vector<nx::core::Error> FilterPreflightMessage::getErrors() const
{
  return m_Errors;
}

std::string FilterPreflightMessage::toString() const
{
  std::string output = "Preflight: " + getFilterNode()->getName();

  for(const auto& warning : getWarnings())
  {
    output += "\n" + std::to_string(warning.code) + warning.message;
  }

  for(const auto& error : getErrors())
  {
    output += "\n" + std::to_string(error.code) + error.message;
  }

  return output;
}
