#pragma once

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractPipelineNode;

/**
 * @class AbstractPipelineMessage
 * @brief The AbstractPipelineMessage class is the base class for all pipeline-related messages.
 */
class COMPLEX_EXPORT AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a new pipeline message for the specified node.
   * @param node
   */
  AbstractPipelineMessage(AbstractPipelineNode* node);

  virtual ~AbstractPipelineMessage();

  /**
   * @brief Returns a pointer to the pipeline node emitting the message.
   * @return AbstractPipelineNode*
   */
  AbstractPipelineNode* getNode() const;

private:
  AbstractPipelineNode* m_Node = nullptr;
};
} // namespace complex
