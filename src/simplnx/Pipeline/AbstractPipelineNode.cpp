#include "AbstractPipelineNode.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/Pipeline/Messaging/NodeStatusMessage.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_IsDisabledKey = "isDisabled";
}

AbstractPipelineNode::AbstractPipelineNode(Pipeline* parent)
: m_Parent(parent)
{
}

AbstractPipelineNode::~AbstractPipelineNode() noexcept = default;

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
}

void AbstractPipelineNode::setEnabled(bool enabled)
{
  return setDisabled(!enabled);
}

const DataStructure& AbstractPipelineNode::getDataStructure() const
{
  return m_DataStructure;
}

void AbstractPipelineNode::setDataStructure(const DataStructure& dataStructure)
{
  m_DataStructure = dataStructure;
}

void AbstractPipelineNode::checkDataStructureSize(DataStructure& dataStructure)
{
  const uint64 largeDataStructureSize = Application::Instance()->getPreferences()->largeDataStructureSize();
  if(dataStructure.memoryUsage() > largeDataStructureSize)
  {
    auto result = dataStructure.transferDataArraysOoc();
    if(result.warnings().size() > 0)
    {
      std::cout << "Some DataArrays not converted to out-of-core:" << std::endl;
      for(const auto& warning : result.warnings())
      {
        std::cout << "\tWarning: " << warning.message << std::endl;
      }
    }
  }
}

const DataStructure& AbstractPipelineNode::getPreflightStructure() const
{
  return m_PreflightStructure;
}

void AbstractPipelineNode::setPreflightStructure(const DataStructure& dataStructure, bool success)
{
  m_PreflightStructure = dataStructure;
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
  dataStructure.flush();
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

void AbstractPipelineNode::sendPipelineRunStateMessage(nx::core::RunState value)
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
void AbstractPipelineNode::sendFilterRunStateMessage(int32_t filterIndex, nx::core::RunState value)
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
void AbstractPipelineNode::sendPipelineFaultMessage(nx::core::FaultState state)
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
void AbstractPipelineNode::sendFilterFaultMessage(int32_t filterIndex, nx::core::FaultState state)
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

nx::core::FaultState AbstractPipelineNode::getFaultState() const
{
  return m_FaultState;
}

bool AbstractPipelineNode::hasErrors() const
{
  return m_FaultState == nx::core::FaultState::Errors;
}

bool AbstractPipelineNode::hasWarnings() const
{
  return m_FaultState == nx::core::FaultState::Warnings;
}

void AbstractPipelineNode::setHasWarnings(bool value)
{
  if(value)
  {
    m_FaultState = nx::core::FaultState::Warnings;
  }
  else if(!value && m_FaultState == nx::core::FaultState::Warnings)
  {
    m_FaultState = nx::core::FaultState::None;
  }
}

void AbstractPipelineNode::setHasErrors(bool value)
{
  if(value)
  {
    m_FaultState = nx::core::FaultState::Errors;
  }
  else if(!value && m_FaultState == nx::core::FaultState::Errors)
  {
    m_FaultState = nx::core::FaultState::None;
  }
}

void AbstractPipelineNode::clearFaultState()
{
  m_FaultState = nx::core::FaultState::None;
}

const AbstractPipelineNode::CancelledSignalType& AbstractPipelineNode::getCancelledSignal() const
{
  return m_CancelledSignal;
}

AbstractPipelineNode::CancelledSignalType& AbstractPipelineNode::getCancelledSignal()
{
  return m_CancelledSignal;
}

void AbstractPipelineNode::sendCancelledMessage()
{
  m_CancelledSignal();
}

nlohmann::json AbstractPipelineNode::toJson() const
{
  auto json = toJsonImpl();
  json[k_IsDisabledKey] = m_IsDisabled;
  return json;
}

bool AbstractPipelineNode::ReadDisabledState(const nlohmann::json& json)
{
  if(!json.contains(k_IsDisabledKey.str()))
  {
    return false;
  }
  return json[k_IsDisabledKey].get<bool>();
}
