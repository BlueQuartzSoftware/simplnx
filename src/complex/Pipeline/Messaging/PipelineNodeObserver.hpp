#pragma once

#include <memory>
#include <vector>

#include "complex/complex_export.hpp"

namespace complex
{
class IPipelineMessage;
class IPipelineNode;

/**
 * @class PipelineNodeObserver
 * @brief The PipelineNodeObserver class provides API and base functionality
 * for derived classes to listen for messages emitted by target IPipelineNodes.
 */
class COMPLEX_EXPORT PipelineNodeObserver
{
  friend class IPipelineNode;

public:
  /**
   * @brief Constructs a new PipelineNodeObserver.
   */
  PipelineNodeObserver();

  virtual ~PipelineNodeObserver();

  /**
   * @brief Returns a collection of observed pipeline nodes.
   * @return std::vector<IPipelineNode*>
   */
  std::vector<IPipelineNode*> getObservedNodes() const;

  /**
   * @brief Returns true if the specified node is being observed. Returns
   * false otherwise.
   * @param node
   * @return bool
   */
  bool isObservingNode(IPipelineNode* node) const;

  /**
   * @brief Start observing the specified pipeline node.
   * @param node
   */
  void startObservingNode(IPipelineNode* node);

  /**
   * @brief Stop observing the specified pipeline node.
   * @param node
   */
  void stopObservingNode(IPipelineNode* node);

protected:
  /**
   * @brief Called when the specified pipeline node emits a message.
   * @param node
   * @param msg
   */
  virtual void onNotify(IPipelineNode* node, const std::shared_ptr<IPipelineMessage>& msg) = 0;

private:
  std::vector<IPipelineNode*> m_ObservedNodes;
};
} // namespace complex
