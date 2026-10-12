#include "CliObserver.hpp"

#include "simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp"
#include "simplnx/Utilities/TimeUtilities.hpp"

#include <chrono>
#include <iostream>
#include <string>

using namespace nx::core;
using namespace nx::core::CLI;

PipelineObserver::PipelineObserver(Pipeline* pipeline)
{
  if(pipeline != nullptr)
  {
    startObservingNode(pipeline);
    pipeline->getCancelledSignal().connect([this](void) { onCancelled(); });
    pipeline->getPipelineFaultSignal().connect([this](AbstractPipelineNode* node, FaultState state) { onFaultStateChanged(node, state); });
  }
  if(pipeline == nullptr)
  {
    return;
  }
  int32_t currentFilterIndex = 0;
  for(const auto& cxFilter : *pipeline)
  {
    m_SignalConnections.push_back(cxFilter->getFilterUpdateSignal().connect([currentFilterIndex](nx::core::AbstractPipelineNode* node, int32_t, const std::string& message) {
      std::cout << timestamp() << "  [" << currentFilterIndex << "] " << node->getName() << ": " << message << std::endl;
    }));

    m_SignalConnections.push_back(cxFilter->getFilterProgressSignal().connect([currentFilterIndex](nx::core::AbstractPipelineNode* node, int32_t, int32_t progress, const std::string& message) {
      std::cout << timestamp() << "  [" << currentFilterIndex << "] " << node->getName() << ": " << progress << "% " << message << std::endl;
    }));

    m_SignalConnections.push_back(cxFilter->getFilterFaultSignal().connect([currentFilterIndex](nx::core::AbstractPipelineNode*, int32_t filterIndex, nx::core::FaultState state) {
      if(state == nx::core::FaultState::Errors)
      {
        std::cout << timestamp() << "  [" << currentFilterIndex << "] Error(s) Encountered during filter execution. Fault state= " << static_cast<int32_t>(state) << std::endl;
      }
      if(state == nx::core::FaultState::Warnings)
      {
        std::cout << timestamp() << "  [" << currentFilterIndex << "] Warning(s) Encountered during filter execution. Fault state= " << static_cast<int32_t>(state) << std::endl;
      }
    }));

    m_SignalConnections.push_back(cxFilter->getFilterFaultDetailSignal().connect(
        [currentFilterIndex](nx::core::AbstractPipelineNode*, int32_t filterIndex, const nx::core::WarningCollection& warnings, const nx::core::ErrorCollection& errors) {
          if(!warnings.empty())
          {
            std::cout << "[" << currentFilterIndex << "] Warnings During Execution" << std::endl;
          }
          for(const auto& warn : warnings)
          {
            std::cout << "    Code: " << warn.code << "    Message: " << warn.message << std::endl;
          }
          if(!errors.empty())
          {
            std::cout << "[" << currentFilterIndex << "] Errors During Execution" << std::endl;
          }
          for(const auto& error : errors)
          {
            std::cout << "    Code: " << error.code << "    Message: " << error.message << std::endl;
          }
        }));

    currentFilterIndex++;
  }
}

PipelineObserver::~PipelineObserver() = default;

void PipelineObserver::onNotify(AbstractPipelineNode* node, const std::shared_ptr<AbstractPipelineMessage>& msg)
{
  std::cout << msg->toString() << std::endl;
}

void PipelineObserver::onCancelled() const
{
  std::cout << timestamp() << "  Pipeline has been cancelled" << std::endl;
}

void PipelineObserver::onFaultStateChanged(AbstractPipelineNode* node, FaultState state) const
{
  switch(state)
  {
  case FaultState::Errors:
    std::cout << timestamp() << fmt::format(" '{}' has completed with errors", node->getName()) << std::endl;
    break;
  case FaultState::Warnings:
    std::cout << timestamp() << fmt::format(" '{}' has completed with warnings", node->getName()) << std::endl;
    break;
  case FaultState::None:
    break;
  }
}
