#pragma once

#include <cstddef>

#include "IPipelineMessage.hpp"

namespace complex
{
class IPipelineNode;
class Pipeline;

/**
 * @class NodeAddedMessage
 * @brief
 */
class COMPLEX_EXPORT NodeAddedMessage : public IPipelineMessage
{
public:
  /**
   * @brief 
   * @param pipeline
   * @param newNode
   * @param index
   */
  NodeAddedMessage(Pipeline* pipeline, IPipelineNode* newNode, size_t index);

  virtual ~NodeAddedMessage();

  /**
   * @brief Returns a pointer to the new pipeline node.
   * @return IPipelineNode*
   */
  IPipelineNode* getNewNode() const;

  /**
   * @brief Returns the index at which the new node was added.
   * @return size_t
   */
  size_t getIndex() const;

private:
  IPipelineNode* m_Node;
  size_t m_Index;
};
} // namespace complex
