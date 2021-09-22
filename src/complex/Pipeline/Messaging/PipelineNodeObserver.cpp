#include "PipelineNodeObserver.hpp"

#include <algorithm>

#include "complex/Pipeline/IPipelineNode.hpp"

using namespace complex;

PipelineNodeObserver::PipelineNodeObserver()
{
}

PipelineNodeObserver::~PipelineNodeObserver()
{
  stopObservingNode();
}

IPipelineNode* PipelineNodeObserver::getObservedNode() const
{
  return m_ObservedNode;
}

bool PipelineNodeObserver::isObservingNode() const
{
  return m_ObservedNode != nullptr;
}

void PipelineNodeObserver::startObservingNode(IPipelineNode* node)
{
  if(node == nullptr)
  {
    return;
  }
  if(isObservingNode())
  {
    stopObservingNode();
  }

  m_Connection = node->getSignal().connect([this](IPipelineNode* node, const std::shared_ptr<IPipelineMessage>& msg) { this->onNotify(node, msg); });
  m_ObservedNode = node;
}

void PipelineNodeObserver::stopObservingNode()
{
  m_Connection.disconnect();
  m_ObservedNode = nullptr;
}
