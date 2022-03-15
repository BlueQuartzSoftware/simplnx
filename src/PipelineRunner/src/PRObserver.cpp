#include "PRObserver.hpp"

#include <iostream>
#include <string>

#include "complex/Pipeline/Messaging/AbstractPipelineMessage.hpp"

using namespace complex;
using namespace complex::PipelineRunner;

PipelineObserver::PipelineObserver(Pipeline* pipeline)
{
  if(pipeline != nullptr)
  {
    startObservingNode(pipeline);
  }
  if(pipeline == nullptr)
  {
    return;
  }
  int32_t currentFilterIndex = 0;
  for(auto iter = pipeline->begin(); iter != pipeline->end(); iter++)
  {
    std::shared_ptr<complex::AbstractPipelineNode> cxFilter = *iter;

    m_SignalConnections.push_back(cxFilter->getFilterRunStateSignal().connect([currentFilterIndex](complex::AbstractPipelineNode* node, int32_t filterIndex, complex::RunState state) {
      // std::cout << "[" << currentFilterIndex << "] Changed State" << std::endl;
    }));

    m_SignalConnections.push_back(cxFilter->getFilterUpdateSignal().connect([currentFilterIndex](complex::AbstractPipelineNode* node, int32_t, const std::string& message) {
      std::cout << "[" << currentFilterIndex << "] " << node->getName() << ": " << message << std::endl;
    }));

    m_SignalConnections.push_back(cxFilter->getFilterProgressSignal().connect([currentFilterIndex](complex::AbstractPipelineNode* node, int32_t, int32_t progress, const std::string& message) {
      std::cout << "[" << currentFilterIndex << "] " << node->getName() << ": " << progress << "% " << message << std::endl;
    }));

    m_SignalConnections.push_back(cxFilter->getFilterFaultSignal().connect([currentFilterIndex](complex::AbstractPipelineNode*, int32_t filterIndex, complex::FaultState state) {
      if(state != complex::FaultState::None)
      {
        std::cout << "[" << currentFilterIndex << "] Errors thrown... Fault state= " << static_cast<int32_t>(state) << std::endl;
      }
    }));

    m_SignalConnections.push_back(cxFilter->getFilterFaultDetailSignal().connect(
        [currentFilterIndex](complex::AbstractPipelineNode*, int32_t filterIndex, const complex::WarningCollection& warnings, const complex::ErrorCollection& errors) {
          if(!warnings.empty() || !errors.empty())
          {
            std::cout << "[" << currentFilterIndex << "] Warnings/Errors During Execution" << std::endl;
          }
          for(const auto& warn : warnings)
          {
            std::cout << "    Code: " << warn.code << "    Message: " << warn.message << std::endl;
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
