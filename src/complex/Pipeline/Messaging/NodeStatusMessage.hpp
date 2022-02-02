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
  NodeStatusMessage(AbstractPipelineNode* node, AbstractPipelineNode::FaultState faultState, AbstractPipelineNode::RunState runState);

  ~NodeStatusMessage() override;

  /**
   * @brief Returns the updated AbstractPipelineNode::FaultState.
   * @return AbstractPipelineNode::FaultState
   */
  AbstractPipelineNode::FaultState getFaultState() const;

  /**
   * @brief Returns the updated AbstractPipelineNode::RunState.
   * @return AbstractPipelineNode::RunState
   */
  AbstractPipelineNode::RunState getRunState() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  AbstractPipelineNode::FaultState m_FaultState;
  AbstractPipelineNode::RunState m_RunState;
};
} // namespace complex
