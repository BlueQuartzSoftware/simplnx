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
   * @param faultState
   * @param runState
   */
  NodeStatusMessage(AbstractPipelineNode* node, complex::FaultState faultState, complex::RunState runState);

  ~NodeStatusMessage() override;

  /**
   * @brief Returns the updated complex::FaultState.
   * @return complex::FaultState
   */
  complex::FaultState getFaultState() const;

  /**
   * @brief Returns the updated complex::RunState.
   * @return complex::RunState
   */
  complex::RunState getRunState() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  complex::FaultState m_FaultState;
  complex::RunState m_RunState;
};
} // namespace complex
