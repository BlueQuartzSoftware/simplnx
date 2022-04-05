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
  }
}

PipelineObserver::~PipelineObserver() = default;

void PipelineObserver::onNotify(AbstractPipelineNode* node, const std::shared_ptr<AbstractPipelineMessage>& msg)
{
  std::cout << msg->toString() << std::endl;
}
