#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp"

namespace nx::core
{
class AbstractPipelineNode;
class Pipeline;

/**
 * @class NodeMovedMessage
 * @brief The NodeMovedMessage class exists to notify Pipeline observers that
 * the AbstractPipelineNode has been moved between two indices.
 */
class SIMPLNX_EXPORT NodeMovedMessage : public AbstractPipelineMessage
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

  ~NodeMovedMessage() override;

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

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  usize m_OldIndex;
  usize m_NewIndex;
};
} // namespace nx::core
