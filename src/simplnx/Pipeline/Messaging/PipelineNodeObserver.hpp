#pragma once

#include "simplnx/simplnx_export.hpp"

#include <nod/nod.hpp>

#include <map>
#include <memory>
#include <vector>

namespace nx::core
{
class AbstractPipelineMessage;
class AbstractPipelineNode;

/**
 * @class PipelineNodeObserver
 * @brief The PipelineNodeObserver class provides API and base functionality
 * for derived classes to listen for messages emitted by target
 * AbstractPipelineNodes.
 */
class SIMPLNX_EXPORT PipelineNodeObserver
{
public:
  /**
   * @brief Constructs a new PipelineNodeObserver.
   */
  PipelineNodeObserver();

  /**
   * @brief Copy constructor not implemented.
   * @param other
   */
  PipelineNodeObserver(const PipelineNodeObserver& other) = delete;

  /**
   * @brief Move constructor not implemented.
   * @param other
   */
  PipelineNodeObserver(PipelineNodeObserver&& other) = delete;

  virtual ~PipelineNodeObserver() noexcept;

  /**
   * @brief Returns a pointer to the observed pipeline node. Returns nullptr
   * if no node is being observed.
   * @return AbstractPipelineNode*
   */
  AbstractPipelineNode* getObservedNode() const;

  /**
   * @brief Returns true an AbstractPipelineNode is being observed. Returns
   * false otherwise.
   * @return bool
   */
  bool isObservingNode() const;

  /**
   * @brief Start observing the specified pipeline node. If a node is currently
   * being observed, this method replaces the previous observed node.
   * @param node
   */
  void startObservingNode(AbstractPipelineNode* node);

  /**
   * @brief Stop observing the current pipeline node and clears the observed
   * pointer.
   */
  void stopObservingNode();

protected:
  /**
   * @brief Called when the specified pipeline node emits a message.
   * @param node
   * @param msg
   */
  virtual void onNotify(AbstractPipelineNode* node, const std::shared_ptr<AbstractPipelineMessage>& msg) = 0;

private:
  AbstractPipelineNode* m_ObservedNode = nullptr;
  nod::connection m_Connection;
};
} // namespace nx::core
