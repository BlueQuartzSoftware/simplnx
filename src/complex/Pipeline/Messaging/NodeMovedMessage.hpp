#pragma once

#include "complex/Common/Types.hpp"
#include "complex/Pipeline/Messaging/AbstractPipelineMessage.hpp"

namespace complex
{
class AbstractPipelineNode;
class Pipeline;

/**
 * @class NodeMovedMessage
 * @brief The NodeMovedMessage class exists to notify Pipeline observers that
 * the AbstractPipelineNode has been moved between two indices.
 */
class COMPLEX_EXPORT NodeMovedMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a NodeMovedMessage specifying which AbstractPipelineNode
   * was added to the target Pipeline and the index it was added at.
   * @param pipeline
   * @param fromIndex
   * @param toIndex
   */
  NodeMovedMessage(Pipeline* pipeline, usize fromIndex, usize toIndex);

  virtual ~NodeMovedMessage();

  /**
   * @brief Returns the node's new pipeline index.
   * @return usize
   */
  usize getOldIndex() const;

  /**
   * @brief Returns the node's new pipeline index.
   * @return usize
   */
  usize getNewIndex() const;

private:
  usize m_OldIndex;
  usize m_NewIndex;
};
} // namespace complex
