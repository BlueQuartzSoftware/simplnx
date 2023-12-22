#pragma once

#include "simplnx/Pipeline/AbstractPipelineNode.hpp"
#include "simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp"

namespace nx::core
{
class PipelineFilter;
class IFilter;

/**
 * @class NodeStatusMessage
 * @brief The NodeStatusMessage class is used to notify observers that a
 * AbstractPipelineNode's Status has been changed.
 */
class SIMPLNX_EXPORT NodeStatusMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a new NodeStatusMessage specifying the node and its Status.
   * @param node
   * @param faultState
   * @param runState
   */
  NodeStatusMessage(AbstractPipelineNode* node, nx::core::FaultState faultState, nx::core::RunState runState);

  ~NodeStatusMessage() override;

  /**
   * @brief Returns the updated nx::core::FaultState.
   * @return nx::core::FaultState
   */
  nx::core::FaultState getFaultState() const;

  /**
   * @brief Returns the updated nx::core::RunState.
   * @return nx::core::RunState
   */
  nx::core::RunState getRunState() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  nx::core::FaultState m_FaultState;
  nx::core::RunState m_RunState;
};
} // namespace nx::core
