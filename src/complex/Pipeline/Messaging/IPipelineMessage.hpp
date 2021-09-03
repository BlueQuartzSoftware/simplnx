#pragma once

#include "complex/complex_export.hpp"

namespace complex
{
class IPipelineNode;

/**
 * @class IPipelineMessage
 * @brief The IPipelineMessage class is the base class for all pipeline-related messages.
 */
class COMPLEX_EXPORT IPipelineMessage
{
public:
  /**
   * @brief Constructs a new pipeline message for the specified node.
   * @param node
   */
  IPipelineMessage(IPipelineNode* node);

  virtual ~IPipelineMessage();

  /**
   * @brief Returns the pipeline node that emitted the message.
   * @return IPipelineNode*
   */
  IPipelineNode* getNode() const;

private:
  IPipelineNode* m_Node = nullptr;
};
} // namespace complex
