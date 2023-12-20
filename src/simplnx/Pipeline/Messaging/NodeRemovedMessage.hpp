#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp"

namespace nx::core
{
class Pipeline;

/**
 * @class NodeRemovedMessage
 * @brief The NodeRemovedMessage class exists to notify Pipeline observers
 * that a target AbstractPipelineNode was removed from a pipeline at the
 * specified index.
 */
class SIMPLNX_EXPORT NodeRemovedMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a NodeRemovedMessage specifying the index of the
   * removed node from a target Pipeline.
   * @param pipeline
   * @param index
   */
  NodeRemovedMessage(Pipeline* pipeline, usize index);

  ~NodeRemovedMessage() override;

  /**
   * @brief Returns the index at which the pipeline node was removed.
   * @return usize
   */
  usize getIndex() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  usize m_Index;
};
} // namespace nx::core
