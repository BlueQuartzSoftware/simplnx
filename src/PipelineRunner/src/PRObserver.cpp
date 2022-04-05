#include "PRObserver.hpp"

#include <iostream>
#include <string>

#include "complex/Pipeline/Messaging/AbstractPipelineMessage.hpp"

using namespace complex;
using namespace complex::PipelineRunner;

PipelineObserver::PipelineObserver(Pipeline* pipeline)
: PipelineNodeObserver()
{
  if(pipeline != nullptr)
  {
    startObservingNode(pipeline);
    pipeline->getCancelledSignal().connect([=](void) { onCancelled(); });
    pipeline->getFilterProgressSignal().connect([=](AbstractPipelineNode* node, int32_t progress, int32_t max, const std::string& msg) { onFilterProgress(node, progress, max, msg); });
    pipeline->getFilterRunStateSignal().connect([=](AbstractPipelineNode* node, int32_t index, RunState state) { onRunStateChanged(node, state); });
    pipeline->getFilterUpdateSignal().connect([=](AbstractPipelineNode* node, int32 index, const std::string& msg) { onFilterUpdate(node, msg ); });
    pipeline->getPipelineFaultSignal().connect([=](AbstractPipelineNode* node, FaultState state) { onFaultStateChanged(node, state ); });
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
