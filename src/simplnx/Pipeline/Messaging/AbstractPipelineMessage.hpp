#pragma once

#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
class AbstractPipelineNode;

/**
 * @class AbstractPipelineMessage
 * @brief The AbstractPipelineMessage class is the base class for all pipeline-related messages.
 */
class SIMPLNX_EXPORT AbstractPipelineMessage
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

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  virtual std::string toString() const = 0;

private:
  AbstractPipelineNode* m_Node = nullptr;
};
} // namespace nx::core
