#include "AbstractPipelineNode.hpp"

#include <algorithm>
#include <cstdlib>

#include "complex/Pipeline/Messaging/NodeStatusMessage.hpp"
#include "complex/Pipeline/Messaging/PipelineNodeObserver.hpp"

using namespace complex;

AbstractPipelineNode::AbstractPipelineNode(Pipeline* parent)
: m_Parent(parent)
{
}

AbstractPipelineNode::~AbstractPipelineNode() = default;

Pipeline* AbstractPipelineNode::getParentPipeline() const
{
  return m_Parent;
}

void AbstractPipelineNode::setParentPipeline(Pipeline* parent)
{
  m_Parent = parent;
}

void AbstractPipelineNode::markDirty()
{
  m_Status = Status::Dirty;
}

bool AbstractPipelineNode::isDirty() const
{
  return m_Status == Status::Dirty;
}

AbstractPipelineNode::Status AbstractPipelineNode::getStatus() const
{
  return m_Status;
}

void AbstractPipelineNode::setStatus(Status status)
{
  m_Status = status;
  notify(std::make_shared<NodeStatusMessage>(this, status));
}

const DataStructure& AbstractPipelineNode::getDataStructure() const
{
  return m_DataStructure;
}

void AbstractPipelineNode::setDataStructure(const DataStructure& ds)
{
  m_DataStructure = ds;
}

const DataStructure& AbstractPipelineNode::getPreflightStructure() const
{
  return m_PreflightStructure;
}

void AbstractPipelineNode::setPreflightStructure(const DataStructure& ds)
{
  m_PreflightStructure = ds;
  m_IsPreflighted = true;
}

void AbstractPipelineNode::clearDataStructure()
{
  m_DataStructure = DataStructure();
  m_Status = Status::Dirty;
}

void AbstractPipelineNode::clearPreflightStructure()
{
  m_DataStructure = DataStructure();
  m_PreflightStructure = DataStructure();
  m_Status = Status::Dirty;
  m_IsPreflighted = false;
}

bool AbstractPipelineNode::isPreflighted() const
{
  return m_IsPreflighted;
}

void AbstractPipelineNode::notify(const std::shared_ptr<AbstractPipelineMessage>& msg)
{
  m_Signal(this, msg);
}

AbstractPipelineNode::SignalType& AbstractPipelineNode::getSignal()
{
  return m_Signal;
}
