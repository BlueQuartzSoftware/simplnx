#pragma once

#include "complex/Pipeline/AbstractPipelineNode.hpp"
#include "complex/Pipeline/Messaging/AbstractPipelineMessage.hpp"

namespace complex
{
class PipelineFilter;
class IFilter;

/**
 * @class NodeStatusMessage
 * @brief The NodeStatusMessage class is used to notify observers that a
 * AbstractPipelineNode's Status has been changed.
 */
class COMPLEX_EXPORT NodeStatusMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a new NodeStatusMessage specifying the node and its Status.
   * @param node
   * @param status
   */
  NodeStatusMessage(AbstractPipelineNode* node, AbstractPipelineNode::Status status);

  ~NodeStatusMessage() override;

  /**
   * @brief Returns the updated AbstractPipelineNode::Status.
   * @return AbstractPipelineNode::Status
   */
  AbstractPipelineNode::Status getStatus() const;

private:
  AbstractPipelineNode::Status m_Status;
};
} // namespace complex
