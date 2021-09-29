#pragma once

#include "complex/Common/Types.hpp"
#include "complex/Pipeline/Messaging/AbstractPipelineMessage.hpp"

namespace complex
{
class Pipeline;

/**
 * @class NodeRemovedMessage
 * @brief The NodeRemovedMessage class exists to notify Pipeline observers
 * that a target AbstractPipelineNode was removed from a pipeline at the
 * specified index.
 */
class COMPLEX_EXPORT NodeRemovedMessage : public AbstractPipelineMessage
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

private:
  usize m_Index;
};
} // namespace complex
