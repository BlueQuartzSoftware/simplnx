#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp"

namespace nx::core
{
class AbstractPipelineNode;
class Pipeline;

/**
 * @class NodeAddedMessage
 * @brief The NodeAddedMessage class exists to notify Pipeline observers that a
 * target AbstractPipelineNode was added at the specified index.
 */
class SIMPLNX_EXPORT NodeAddedMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a NodeAddedMessage specifying which AbstractPipelineNode
   * was added to the target Pipeline and the index it was added at.
   * @param pipeline
   * @param newNode
   * @param index
   */
  NodeAddedMessage(Pipeline* pipeline, AbstractPipelineNode* newNode, usize index);

  virtual ~NodeAddedMessage();

  /**
   * @brief Returns a pointer to the new pipeline node.
   * @return AbstractPipelineNode*
   */
  AbstractPipelineNode* getNewNode() const;

  /**
   * @brief Returns the index at which the new node was added to the Pipeline.
   * @return usize
   */
  usize getIndex() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  AbstractPipelineNode* m_Node;
  usize m_Index;
};
} // namespace nx::core
