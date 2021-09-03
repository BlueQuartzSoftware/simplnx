#pragma once

#include "complex/Pipeline/IPipelineNode.hpp"
#include "complex/Pipeline/Messaging/IPipelineMessage.hpp"

namespace complex
{
class FilterNode;
class IFilter;

/**
 * @class NodeStatusMessage
 * @brief The NodeStatusMessage class is used to notify observers that the
 * IPipelineNode's Status has been changed.
 */
class COMPLEX_EXPORT NodeStatusMessage : public IPipelineMessage
{
public:
  /**
   * @brief Constructs a new NodeStatusMessage.
   * @param node
   * @param status
   */
  NodeStatusMessage(IPipelineNode* node, IPipelineNode::Status status);

  virtual ~NodeStatusMessage();

  /**
   * @brief Returns the updated IPipelineNode::Status.
   * @return IPipelineNode::Status
   */
  IPipelineNode::Status getStatus() const;

private:
  IPipelineNode::Status m_Status;
};
} // namespace complex
