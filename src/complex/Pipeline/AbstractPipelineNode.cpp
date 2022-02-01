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

bool AbstractPipelineNode::hasParentPipeline() const
{
  return m_Parent != nullptr;
}

bool AbstractPipelineNode::isExecuting() const
{
  return static_cast<bool>(m_Status & Status::Executing);
}

bool AbstractPipelineNode::hasBeenExecuted() const
{
  return static_cast<bool>(m_Status & Status::Executed);
}

bool AbstractPipelineNode::hasErrors() const
{
  return static_cast<bool>(m_Status & Status::Error);
}

bool AbstractPipelineNode::hasWarnings() const
{
  return static_cast<bool>(m_Status & Status::Warning);
}

bool AbstractPipelineNode::isDisabled() const
{
  return static_cast<bool>(m_Status & Status::Disabled);
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
  // If Status has not changed, do nothing
  if(m_Status == status)
  {
    return;
  }

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

  setStatus(static_cast<Status>(statusBits.to_ulong()));
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

  setStatus(static_cast<Status>(statusBits.to_ulong()));
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

  setStatus(static_cast<Status>(statusBits.to_ulong()));
}

void AbstractPipelineNode::setHasBeenExecuted(bool value)
{
  std::bitset<8> statusBits(m_Status);

  if(value)
  {
    statusBits |= Status::Executed;
  }
  else
  {
    std::bitset<8> mask(Status::Executed);
    mask.flip();
    statusBits &= mask;
  }

  setStatus(static_cast<Status>(statusBits.to_ulong()));
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

void AbstractPipelineNode::endExecution(DataStructure& dataStructure)
{
  setDataStructure(dataStructure);
  setIsExecuting(false);
  setHasBeenExecuted(hasParentPipeline());
}

void AbstractPipelineNode::notify(const std::shared_ptr<AbstractPipelineMessage>& msg)
{
  m_Signal(this, msg);
}

AbstractPipelineNode::SignalType& AbstractPipelineNode::getSignal()
{
  return m_Signal;
}

AbstractPipelineNode::PipelineRunStateSignalType& AbstractPipelineNode::getPipelineRunStateSignal()
{
  return m_PipelineRunStateSignal;
}
void AbstractPipelineNode::sendPipelineRunStateMessage(AbstractPipelineNode::RunState value)
{
  m_PipelineRunStateSignal(this, value);
}

AbstractPipelineNode::FilterRunStateSignalType& AbstractPipelineNode::getFilterRunStateSignal()
{
  return m_FilterRunStateSignal;
}
void AbstractPipelineNode::sendFilterRunStateMessage(int32_t filterIndex, AbstractPipelineNode::RunState value)
{
  m_FilterRunStateSignal(this, filterIndex, value);
}

AbstractPipelineNode::FilterProgressSignalType& AbstractPipelineNode::getFilterProgressSignal()
{
  return m_FilterProgressSignal;
}
void AbstractPipelineNode::sendFilterProgressMessage(int32_t filterIndex, int32_t progress, const std::string& message)
{
  m_FilterProgressSignal(this, filterIndex, progress, message);
}

AbstractPipelineNode::FilterUpdateSignalType& AbstractPipelineNode::getFilterUpdateSignal()
{
  return m_FilterUpdateSignal;
}
void AbstractPipelineNode::sendFilterUpdateMessage(int32_t filterIndex, const std::string& message)
{
  m_FilterUpdateSignal(this, filterIndex, message);
}

AbstractPipelineNode::PipelineFaultSignalType& AbstractPipelineNode::getPipelineFaultSignal()
{
  return m_PipelineFaultSignal;
}
void AbstractPipelineNode::sendPipelineFaultMessage(AbstractPipelineNode::FaultState state)
{
  m_PipelineFaultSignal(this, state);
}

AbstractPipelineNode::FilterFaultSignalType& AbstractPipelineNode::getFilterFaultSignal()
{
  return m_FilterFaultSignal;
}
void AbstractPipelineNode::sendFilterFaultMessage(int32_t filterIndex, AbstractPipelineNode::FaultState state)
{
  m_FilterFaultSignal(this, filterIndex, state);
}

AbstractPipelineNode::FilterFaultDetailSignalType& AbstractPipelineNode::getFilterFaultDetailSignal()
{
  return m_FilterFaultDetailSignal;
}
void AbstractPipelineNode::sendFilterFaultDetailMessage(int32_t filterIndex, const WarningCollection& warnings, const ErrorCollection& errors)
{
  m_FilterFaultDetailSignal(this, filterIndex, warnings, errors);
}

std::unique_ptr<Pipeline> AbstractPipelineNode::getPrecedingPipeline() const
{
  const auto* currentPipeline = this;
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
