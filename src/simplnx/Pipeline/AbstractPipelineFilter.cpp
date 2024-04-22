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
}

AbstractPipelineFilter::AbstractPipelineFilter() = default;
