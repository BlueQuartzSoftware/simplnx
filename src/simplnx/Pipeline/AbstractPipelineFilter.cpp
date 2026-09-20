#include "AbstractPipelineFilter.hpp"

using namespace nx::core;

AbstractPipelineFilter::~AbstractPipelineFilter() noexcept = default;

AbstractPipelineNode::NodeType AbstractPipelineFilter::getType() const
{
  return NodeType::Filter;
}

void AbstractPipelineFilter::setIndex(int32 index)
{
  m_Index = index;
  m_HasAssignedIndex = true;
}

std::optional<int32> AbstractPipelineFilter::getAssignedIndex() const
{
  if(!m_HasAssignedIndex)
  {
    return std::nullopt;
  }
  return m_Index;
}

AbstractPipelineFilter::AbstractPipelineFilter() = default;
