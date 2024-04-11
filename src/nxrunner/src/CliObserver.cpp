#include "CliObserver.hpp"

#include "simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp"

#include <iostream>
#include <string>

using namespace nx::core;
using namespace nx::core::CLI;

PipelineObserver::PipelineObserver(Pipeline* pipeline)
{
  if(pipeline != nullptr)
  {
    startObservingNode(pipeline);
    pipeline->getCancelledSignal().connect([=](void) { onCancelled(); });
    pipeline->getFilterProgressSignal().connect([=](AbstractPipelineNode* node, int32_t progress, int32_t max, const std::string& msg) { onFilterProgress(node, progress, max, msg); });
    pipeline->getFilterRunStateSignal().connect([=](AbstractPipelineNode* node, int32_t index, RunState state) { onRunStateChanged(node, state); });
    pipeline->getFilterUpdateSignal().connect([=](AbstractPipelineNode* node, int32 index, const std::string& msg) { onFilterUpdate(node, msg); });
    pipeline->getPipelineFaultSignal().connect([=](AbstractPipelineNode* node, FaultState state) { onFaultStateChanged(node, state); });
  }
  if(pipeline == nullptr)
  {
    return;
  }
  int32_t currentFilterIndex = 0;
  for(const auto cxFilter : *pipeline)
  {
    m_SignalConnections.push_back(cxFilter->getFilterUpdateSignal().connect([currentFilterIndex](nx::core::AbstractPipelineNode* node, int32_t, const std::string& message) {
      std::cout << "[" << currentFilterIndex << "] " << node->getName() << ": " << message << std::endl;
    }));

    m_SignalConnections.push_back(cxFilter->getFilterProgressSignal().connect([currentFilterIndex](nx::core::AbstractPipelineNode* node, int32_t, int32_t progress, const std::string& message) {
      std::cout << "[" << currentFilterIndex << "] " << node->getName() << ": " << progress << "% " << message << std::endl;
    }));

    m_SignalConnections.push_back(cxFilter->getFilterFaultSignal().connect([currentFilterIndex](nx::core::AbstractPipelineNode*, int32_t filterIndex, nx::core::FaultState state) {
      if(state == nx::core::FaultState::Errors)
      {
        std::cout << "[" << currentFilterIndex << "] Error(s) Encountered during filter execution. Fault state= " << static_cast<int32_t>(state) << std::endl;
      }
      if(state == nx::core::FaultState::Warnings)
      {
        std::cout << "[" << currentFilterIndex << "] Warning(s) Encountered during filter execution. Fault state= " << static_cast<int32_t>(state) << std::endl;
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
  std::cout << "Pipeline has been cancelled" << std::endl;
}

void PipelineObserver::onFilterProgress(AbstractPipelineNode* node, int32 progress, int32 maxProgress, const std::string& msg) const
{
  std::cout << fmt::format("{} ({} / {}): {}", node->getName(), progress, maxProgress, msg) << std::endl;
}

void PipelineObserver::onRunStateChanged(AbstractPipelineNode* node, RunState state) const
{
  switch(state)
  {
  case RunState::Executing:
    std::cout << fmt::format("{} has begun executing", node->getName()) << std::endl;
    break;
  case RunState::Preflighting:
    std::cout << fmt::format("{} has begun preflighting", node->getName()) << std::endl;
    break;
  case RunState::Idle:
    std::cout << fmt::format("{} has completed", node->getName()) << std::endl;
    break;
  case RunState::Queued:
    break;
  }
}

void PipelineObserver::onFilterUpdate(AbstractPipelineNode* node, const std::string& msg) const
{
  std::cout << fmt::format("{}: {}", node->getName(), msg) << std::endl;
}

void PipelineObserver::onFaultStateChanged(AbstractPipelineNode* node, FaultState state) const
{
  switch(state)
  {
  case FaultState::Errors:
    std::cout << fmt::format("{} has completed with errors", node->getName()) << std::endl;
    break;
  case FaultState::Warnings:
    std::cout << fmt::format("{} has completed with warnings", node->getName()) << std::endl;
    break;
  case FaultState::None:
    break;
  }
}
