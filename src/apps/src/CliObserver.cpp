#include "CliObserver.hpp"
#include "simplnx/Common/Result.hpp"
#include <cstdint>

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
    pipeline->getCancelledSignal().connect([this]() { onCancelled(); });
    pipeline->getPipelineFaultSignal().connect([this](AbstractPipelineNode* node, FaultState state) { onFaultStateChanged(node, state); });
  }
  if(pipeline == nullptr)
  {
    return;
  }
  m_SignalConnections.emplace_back(pipeline->getFilterFaultDetailSignal().connect([](AbstractPipelineNode*, int32_t, const WarningCollection& warnings, const ErrorCollection& errors) {
    if(!warnings.empty())
    {
      std::cout << "Pipeline completion warnings" << '\n';
    }
    for(const auto& warning : warnings)
    {
      std::cout << "    Code: " << warning.code << "    Message: " << warning.message << '\n';
    }
    if(!errors.empty())
    {
      std::cout << "Pipeline completion errors" << '\n';
    }
    for(const auto& error : errors)
    {
      std::cout << "    Code: " << error.code << "    Message: " << error.message << '\n';
    }
  }));
  int32_t currentFilterIndex = 0;
  for(const auto& cxFilter : *pipeline)
  {
    m_SignalConnections.emplace_back(cxFilter->getFilterUpdateSignal().connect([currentFilterIndex](nx::core::AbstractPipelineNode* node, int32_t, const std::string& message) {
      std::cout << timestamp() << "  [" << currentFilterIndex << "] " << node->getName() << ": " << message << '\n';
    }));

    m_SignalConnections.emplace_back(cxFilter->getFilterProgressSignal().connect([currentFilterIndex](nx::core::AbstractPipelineNode* node, int32_t, int32_t progress, const std::string& message) {
      std::cout << timestamp() << "  [" << currentFilterIndex << "] " << node->getName() << ": " << progress << "% " << message << '\n';
    }));

    m_SignalConnections.emplace_back(cxFilter->getFilterFaultSignal().connect([currentFilterIndex](nx::core::AbstractPipelineNode*, int32_t /*filterIndex*/, nx::core::FaultState state) {
      if(state == nx::core::FaultState::Errors)
      {
        std::cout << timestamp() << "  [" << currentFilterIndex << "] Error(s) Encountered during filter execution. Fault state= " << static_cast<int32_t>(state) << '\n';
      }
      if(state == nx::core::FaultState::Warnings)
      {
        std::cout << timestamp() << "  [" << currentFilterIndex << "] Warning(s) Encountered during filter execution. Fault state= " << static_cast<int32_t>(state) << '\n';
      }
    }));

    m_SignalConnections.emplace_back(cxFilter->getFilterFaultDetailSignal().connect(
        [currentFilterIndex](nx::core::AbstractPipelineNode*, int32_t /*filterIndex*/, const nx::core::WarningCollection& warnings, const nx::core::ErrorCollection& errors) {
          if(!warnings.empty())
          {
            std::cout << "[" << currentFilterIndex << "] Warnings During Execution" << '\n';
          }
          for(const auto& warn : warnings)
          {
            std::cout << "    Code: " << warn.code << "    Message: " << warn.message << '\n';
          }
          if(!errors.empty())
          {
            std::cout << "[" << currentFilterIndex << "] Errors During Execution" << '\n';
          }
          for(const auto& error : errors)
          {
            std::cout << "    Code: " << error.code << "    Message: " << error.message << '\n';
          }
        }));

    currentFilterIndex++;
  }
}

PipelineObserver::~PipelineObserver() = default;

void PipelineObserver::onNotify(AbstractPipelineNode* /*node*/, const std::shared_ptr<AbstractPipelineMessage>& msg)
{
  std::cout << msg->toString() << '\n';
}

void PipelineObserver::onCancelled()
{
  std::cout << timestamp() << "  Pipeline has been cancelled" << '\n';
}

void PipelineObserver::onFaultStateChanged(AbstractPipelineNode* node, FaultState state)
{
  switch(state)
  {
  case FaultState::Errors:
    std::cout << timestamp() << fmt::format(" '{}' has completed with errors", node->getName()) << '\n';
    break;
  case FaultState::Warnings:
    std::cout << timestamp() << fmt::format(" '{}' has completed with warnings", node->getName()) << '\n';
    break;
  case FaultState::None:
    break;
  }
}
