#include "PipelineNodeObserver.hpp"

#include <algorithm>

#include "complex/Pipeline/IPipelineNode.hpp"

using namespace complex;

PipelineNodeObserver::PipelineNodeObserver()
{
}

PipelineNodeObserver::~PipelineNodeObserver() = default;

std::vector<IPipelineNode*> PipelineNodeObserver::getObservedNodes() const
{
  return m_ObservedNodes;
}

bool PipelineNodeObserver::isObservingNode(IPipelineNode* node) const
{
  return std::find(m_ObservedNodes.begin(), m_ObservedNodes.end(), node) != m_ObservedNodes.end();
}

void PipelineNodeObserver::startObservingNode(IPipelineNode* node)
{
  if(isObservingNode(node))
  {
    return;
  }

  m_ObservedNodes.push_back(node);
  node->addObserver(this);
}

void PipelineNodeObserver::stopObservingNode(IPipelineNode* node)
{
  if(!isObservingNode(node))
  {
    return;
  }

  m_ObservedNodes.erase(std::remove(m_ObservedNodes.begin(), m_ObservedNodes.end(), node));
  node->removeObserver(this);
}
