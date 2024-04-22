#include "PipelineNodeObserver.hpp"

#include "simplnx/Pipeline/AbstractPipelineNode.hpp"

using namespace nx::core;

PipelineNodeObserver::PipelineNodeObserver()
{
}

PipelineNodeObserver::~PipelineNodeObserver() noexcept
{
  stopObservingNode();
}

AbstractPipelineNode* PipelineNodeObserver::getObservedNode() const
{
  return m_ObservedNode;
}

bool PipelineNodeObserver::isObservingNode() const
{
  return m_ObservedNode != nullptr;
}

void PipelineNodeObserver::startObservingNode(AbstractPipelineNode* node)
{
  if(node == nullptr)
  {
    return;
  }
  if(isObservingNode())
  {
    stopObservingNode();
  }

  m_Connection = node->getSignal().connect([this](AbstractPipelineNode* pipelineNode, const std::shared_ptr<AbstractPipelineMessage>& msg) { this->onNotify(pipelineNode, msg); });
  m_ObservedNode = node;
}

void PipelineNodeObserver::stopObservingNode()
{
  m_Connection.disconnect();
  m_ObservedNode = nullptr;
}
