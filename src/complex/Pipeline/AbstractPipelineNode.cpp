#include "AbstractPipelineNode.hpp"

#include <algorithm>
#include <cstdlib>

#include "complex/Pipeline/Messaging/NodeStatusMessage.hpp"
#include "complex/Pipeline/Messaging/PipelineNodeObserver.hpp"
#include "complex/Pipeline/Pipeline.hpp"

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

void AbstractPipelineNode::setDataStructure(const DataStructure& ds, bool success)
{
  m_DataStructure = ds;

  if(success)
  {
    setStatus(Status::Completed);
  }
  else
  {
    markDirty();
  }
}

const DataStructure& AbstractPipelineNode::getPreflightStructure() const
{
  return m_PreflightStructure;
}

void AbstractPipelineNode::setPreflightStructure(const DataStructure& ds, bool success)
{
  m_PreflightStructure = ds;
  m_IsPreflighted = success;

  if(!success)
  {
    m_Status = Status::Dirty;
  }
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

std::unique_ptr<Pipeline> AbstractPipelineNode::getPrecedingPipeline() const
{
  auto currentPipeline = this;
  auto pipeline = getPrecedingPipelineSegment();
  if(pipeline == nullptr)
  {
    return nullptr;
  }

  while((currentPipeline = currentPipeline->getParentPipeline()) != nullptr)
  {
    auto segment = currentPipeline->getPrecedingPipelineSegment();
    if(segment == nullptr)
    {
      break;
    }
    pipeline->push_front(std::move(segment));
  }
  return std::move(pipeline);
}

std::unique_ptr<Pipeline> AbstractPipelineNode::getPrecedingPipelineSegment() const
{
  Pipeline* parentPipeline = getParentPipeline();
  if(parentPipeline == nullptr)
  {
    return nullptr;
  }
  if(parentPipeline->isEmpty())
  {
    auto name = parentPipeline->getName();
    auto filterList = parentPipeline->getFilterList();
    return std::make_unique<Pipeline>(name, filterList);
  }

  auto iter = parentPipeline->find(this);
  return std::move(parentPipeline->copySegment(parentPipeline->begin(), iter));
}
