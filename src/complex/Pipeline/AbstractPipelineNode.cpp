#include "AbstractPipelineNode.hpp"

#include <algorithm>
#include <bitset>
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

bool AbstractPipelineNode::isExecuting() const
{
  return (m_Status & Status::Executing);
}

bool AbstractPipelineNode::hasErrors() const
{
  return (m_Status & Status::Error);
}

bool AbstractPipelineNode::hasWarnings() const
{
  return (m_Status & Status::Warning);
}

bool AbstractPipelineNode::isDisabled() const
{
  return (m_Status & Status::Disabled);
}

bool AbstractPipelineNode::isEnabled() const
{
  return !isDisabled();
}

void AbstractPipelineNode::setDisabled(bool disabled)
{
  std::bitset<8> statusBits(m_Status);
  
  if(disabled)
  {
    statusBits |= Status::Disabled;
  }
  else
  {
    std::bitset<8> mask(Status::Disabled);
    mask.flip();
    statusBits &= mask;
  }

  m_Status = static_cast<Status>(statusBits.to_ulong());
  notify(std::make_shared<NodeStatusMessage>(this, m_Status));
}

void AbstractPipelineNode::setEnabled(bool enabled)
{
  return setDisabled(!enabled);
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

void AbstractPipelineNode::setHasWarnings(bool value)
{
  std::bitset<8> statusBits(m_Status);

  if(value)
  {
    statusBits |= Status::Warning;
  }
  else
  {
    std::bitset<8> mask(Status::Warning);
    mask.flip();
    statusBits &= mask;
  }

  m_Status = static_cast<Status>(statusBits.to_ulong());
  notify(std::make_shared<NodeStatusMessage>(this, m_Status));
}

void AbstractPipelineNode::setHasErrors(bool value)
{
  std::bitset<8> statusBits(m_Status);

  if(value)
  {
    statusBits |= Status::Error;
  }
  else
  {
    std::bitset<8> mask(Status::Error);
    mask.flip();
    statusBits &= mask;
  }

  m_Status = static_cast<Status>(statusBits.to_ulong());
  notify(std::make_shared<NodeStatusMessage>(this, m_Status));
}

void AbstractPipelineNode::setIsExecuting(bool value)
{
  std::bitset<8> statusBits(m_Status);

  if(value)
  {
    statusBits |= Status::Executing;
  }
  else
  {
    std::bitset<8> mask(Status::Executing);
    mask.flip();
    statusBits &= mask;
  }

  m_Status = static_cast<Status>(statusBits.to_ulong());
  notify(std::make_shared<NodeStatusMessage>(this, m_Status));
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

void AbstractPipelineNode::setPreflightStructure(const DataStructure& ds, bool success)
{
  m_PreflightStructure = ds;
  m_IsPreflighted = success;
}

void AbstractPipelineNode::clearDataStructure()
{
  m_DataStructure = DataStructure();
  m_Status = Status::None;
}

void AbstractPipelineNode::clearPreflightStructure()
{
  m_DataStructure = DataStructure();
  m_PreflightStructure = DataStructure();
  m_Status = Status::None;
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
