#pragma once

#include "complex/Pipeline/IPipelineNode.hpp"
#include "complex/Pipeline/Messaging/IPipelineMessage.hpp"

namespace complex
{
class FilterNode;
class IFilter;

/**
 * @class FilterStatusMessage
 * @brief
 */
class COMPLEX_EXPORT FilterStatusMessage : public IPipelineMessage
{
public:
  /**
   * @brief
   * @param node
   * @param index
   * @param status
   */
  FilterStatusMessage(IPipelineNode* node, IPipelineNode::Status status);

  virtual ~FilterStatusMessage();

  /**
   * @brief
   * @return IPipelineNode::Status
   */
  IPipelineNode::Status getStatus() const;

private:
  IPipelineNode::Status m_Status;
};
} // namespace complex
