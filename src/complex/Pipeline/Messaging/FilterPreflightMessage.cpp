#include "FilterPreflightMessage.hpp"

#include "complex/Filter/Output.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"

using namespace complex;

FilterPreflightMessage::FilterPreflightMessage(PipelineFilter* filterNode, const std::vector<complex::Warning>& warnings, const std::vector<complex::Error>& errors)
: AbstractPipelineMessage(filterNode)
, m_Warnings(warnings)
, m_Errors(errors)
{
}

FilterPreflightMessage::~FilterPreflightMessage() = default;

complex::PipelineFilter* FilterPreflightMessage::getFilterNode() const
{
  return dynamic_cast<PipelineFilter*>(getNode());
}

std::vector<complex::Warning> FilterPreflightMessage::getWarnings() const
{
  return m_Warnings;
}

std::vector<complex::Error> FilterPreflightMessage::getErrors() const
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
