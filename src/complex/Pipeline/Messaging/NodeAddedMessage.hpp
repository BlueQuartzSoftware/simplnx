#pragma once

#include <cstddef>

#include "AbstractPipelineMessage.hpp"

namespace complex
{
class AbstractPipelineNode;
class Pipeline;

/**
 * @class NodeAddedMessage
 * @brief The NodeAddedMessage class exists to notify Pipeline observers that a
 * target AbstractPipelineNode was added at the specified index.
 */
class COMPLEX_EXPORT NodeAddedMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a NodeAddedMessage specifying which AbstractPipelineNode
   * was added to the target Pipeline and the index it was added at.
   * @param pipeline
   * @param newNode
   * @param index
   */
  NodeAddedMessage(Pipeline* pipeline, AbstractPipelineNode* newNode, size_t index);

  virtual ~NodeAddedMessage();

  /**
   * @brief Returns a pointer to the new pipeline node.
   * @return AbstractPipelineNode*
   */
  AbstractPipelineNode* getNewNode() const;

  /**
   * @brief Returns the index at which the new node was added to the Pipeline.
   * @return size_t
   */
  size_t getIndex() const;

private:
  AbstractPipelineNode* m_Node;
  size_t m_Index;
};
} // namespace complex
