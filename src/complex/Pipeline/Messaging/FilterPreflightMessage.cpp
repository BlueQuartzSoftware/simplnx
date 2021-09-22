#include "FilterPreflightMessage.hpp"

#include "complex/Filter/Output.hpp"
#include "complex/Pipeline/FilterNode.hpp"

using namespace complex;

FilterPreflightMessage::FilterPreflightMessage(FilterNode* filterNode, const std::vector<complex::Warning>& warnings, const std::vector<complex::Error>& errors)
: IPipelineMessage(filterNode)
, m_Warnings(warnings)
, m_Errors(errors)
{
}

FilterPreflightMessage::~FilterPreflightMessage() = default;

complex::FilterNode* FilterPreflightMessage::getFilterNode() const
{
  return m_FilteNode;
}

std::vector<complex::Warning> FilterPreflightMessage::getWarnings() const
{
  return m_Warnings;
}

std::vector<complex::Error> FilterPreflightMessage::getErrors() const
{
  return m_Errors;
}
