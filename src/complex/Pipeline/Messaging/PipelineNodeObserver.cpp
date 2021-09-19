#include "PipelineNodeObserver.hpp"

#include <algorithm>

#include "complex/Pipeline/IPipelineNode.hpp"

using namespace complex;

PipelineNodeObserver::PipelineNodeObserver()
{
}

PipelineNodeObserver::~PipelineNodeObserver()
{
  if(isObservingNode())
  {
    stopObservingNode(m_ObservedNode);
  }
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
    stopObservingNode(m_ObservedNode);
    return;
  }

  m_Connection = node->getSignal().connect([this](IPipelineNode* node, const std::shared_ptr<IPipelineMessage>& msg) { this->onNotify(node, msg); });
  m_ObservedNode = node;
  node->addObserver(this);
}

void PipelineNodeObserver::stopObservingNode(IPipelineNode* node)
{
  if(!isObservingNode() || m_ObservedNode != node)
  {
    return;
  }

  m_Connection.disconnect();
  m_ObservedNode = nullptr;
  node->removeObserver(this);
}
