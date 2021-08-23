#include "IPipelineNode.hpp"

using namespace complex;


IPipelineNode::IdType IPipelineNode::CreateId()
{
  return Uuid();
}

IPipelineNode::IPipelineNode()
: m_Id(CreateId())
{
}

IPipelineNode::~IPipelineNode() = default;

IPipelineNode::IdType IPipelineNode::getId() const
{
  return m_Id;
}

void IPipelineNode::markDirty()
{
  m_IsDirty = true;
}

void IPipelineNode::markNotDirty()
{
  m_IsDirty = false;
}

bool IPipelineNode::isDirty() const
{
  return m_IsDirty;
}

const DataStructure& IPipelineNode::getDataStructure() const
{
  return m_DataStructure;
}

void IPipelineNode::setDataStructure(const DataStructure& ds)
{
  m_DataStructure = ds;
}

void IPipelineNode::clearDataStructure()
{
  m_DataStructure = DataStructure();
  m_IsDirty = true;
}
