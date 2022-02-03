#include "AbstractPipelineNode.hpp"

#include "complex/Pipeline/Messaging/NodeStatusMessage.hpp"
#include "complex/Pipeline/Pipeline.hpp"

#include <algorithm>
#include <cstdlib>

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

bool AbstractPipelineNode::isDisabled() const
{
  return m_IsDisabled;
}

bool AbstractPipelineNode::isEnabled() const
{
  return !m_IsDisabled;
}

void AbstractPipelineNode::setDisabled(bool disabled)
{
  m_IsDisabled = disabled;
  notify(std::make_shared<NodeStatusMessage>(this, m_FaultState, m_RunState));
}

void AbstractPipelineNode::setEnabled(bool enabled)
{
  return setDisabled(!enabled);
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
}

void AbstractPipelineNode::clearPreflightStructure()
{
  m_DataStructure = DataStructure();
  m_PreflightStructure = DataStructure();
  m_IsPreflighted = false;
}

bool AbstractPipelineNode::isPreflighted() const
{
  return m_IsPreflighted;
}

void AbstractPipelineNode::endExecution(DataStructure& dataStructure)
{
  setDataStructure(dataStructure);
}

void AbstractPipelineNode::notify(const std::shared_ptr<AbstractPipelineMessage>& msg)
{
  m_Signal(this, msg);
}

AbstractPipelineNode::SignalType& AbstractPipelineNode::getSignal()
{
  return m_Signal;
}
const AbstractPipelineNode::PipelineRunStateSignalType& AbstractPipelineNode::getPipelineRunStateSignal() const
{
  return m_PipelineRunStateSignal;
}
AbstractPipelineNode::PipelineRunStateSignalType& AbstractPipelineNode::getPipelineRunStateSignal()
{
  return m_PipelineRunStateSignal;
}

void AbstractPipelineNode::sendPipelineRunStateMessage(AbstractPipelineNode::RunState value)
{
  m_PipelineRunStateSignal(this, value);
}

const AbstractPipelineNode::FilterRunStateSignalType& AbstractPipelineNode::getFilterRunStateSignal() const
{
  return m_FilterRunStateSignal;
}
AbstractPipelineNode::FilterRunStateSignalType& AbstractPipelineNode::getFilterRunStateSignal()
{
  return m_FilterRunStateSignal;
}
void AbstractPipelineNode::sendFilterRunStateMessage(int32_t filterIndex, AbstractPipelineNode::RunState value)
{
  m_FilterRunStateSignal(this, filterIndex, value);
}

const AbstractPipelineNode::FilterProgressSignalType& AbstractPipelineNode::getFilterProgressSignal() const
{
  return m_FilterProgressSignal;
}
AbstractPipelineNode::FilterProgressSignalType& AbstractPipelineNode::getFilterProgressSignal()
{
  return m_FilterProgressSignal;
}
void AbstractPipelineNode::sendFilterProgressMessage(int32_t filterIndex, int32_t progress, const std::string& message)
{
  m_FilterProgressSignal(this, filterIndex, progress, message);
}

const AbstractPipelineNode::FilterUpdateSignalType& AbstractPipelineNode::getFilterUpdateSignal() const
{
  return m_FilterUpdateSignal;
}
AbstractPipelineNode::FilterUpdateSignalType& AbstractPipelineNode::getFilterUpdateSignal()
{
  return m_FilterUpdateSignal;
}
void AbstractPipelineNode::sendFilterUpdateMessage(int32_t filterIndex, const std::string& message)
{
  m_FilterUpdateSignal(this, filterIndex, message);
}

const AbstractPipelineNode::PipelineFaultSignalType& AbstractPipelineNode::getPipelineFaultSignal() const
{
  return m_PipelineFaultSignal;
}
AbstractPipelineNode::PipelineFaultSignalType& AbstractPipelineNode::getPipelineFaultSignal()
{
  return m_PipelineFaultSignal;
}
void AbstractPipelineNode::sendPipelineFaultMessage(AbstractPipelineNode::FaultState state)
{
  m_PipelineFaultSignal(this, state);
}

const AbstractPipelineNode::FilterFaultSignalType& AbstractPipelineNode::getFilterFaultSignal() const
{
  return m_FilterFaultSignal;
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

complex::AbstractPipelineNode::RunState AbstractPipelineNode::getRunState() const
{
  return m_RunState;
}

complex::AbstractPipelineNode::FaultState AbstractPipelineNode::getFaultState() const
{
  return m_FaultState;
}

bool AbstractPipelineNode::hasErrors() const
{
  return m_FaultState == complex::AbstractPipelineNode::FaultState::Errors;
}

bool AbstractPipelineNode::hasWarnings() const
{
  return m_FaultState == complex::AbstractPipelineNode::FaultState::Warnings;
}

void AbstractPipelineNode::setHasWarnings(bool value)
{
  if(value)
  {
    m_FaultState = complex::AbstractPipelineNode::FaultState::Warnings;
    notify(std::make_shared<NodeStatusMessage>(this, m_FaultState, m_RunState));
  }
  else if(!value && m_FaultState == complex::AbstractPipelineNode::FaultState::Warnings)
  {
    m_FaultState = complex::AbstractPipelineNode::FaultState::None;
    notify(std::make_shared<NodeStatusMessage>(this, m_FaultState, m_RunState));
  }
}

void AbstractPipelineNode::setHasErrors(bool value)
{
  if(value)
  {
    m_FaultState = complex::AbstractPipelineNode::FaultState::Errors;
    notify(std::make_shared<NodeStatusMessage>(this, m_FaultState, m_RunState));
  }
  else if(!value && m_FaultState == complex::AbstractPipelineNode::FaultState::Errors)
  {
    m_FaultState = complex::AbstractPipelineNode::FaultState::None;
    notify(std::make_shared<NodeStatusMessage>(this, m_FaultState, m_RunState));
  }
}

void AbstractPipelineNode::clearFaultState()
{
  m_FaultState = complex::AbstractPipelineNode::FaultState::None;
  notify(std::make_shared<NodeStatusMessage>(this, m_FaultState, m_RunState));
}

void AbstractPipelineNode::setRunState(RunState runState)
{
  m_RunState = runState;
  notify(std::make_shared<NodeStatusMessage>(this, m_FaultState, m_RunState));
}
