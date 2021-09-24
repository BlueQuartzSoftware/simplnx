#pragma once

#include <cstddef>

#include "AbstractPipelineMessage.hpp"

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
  NodeRemovedMessage(Pipeline* pipeline, size_t index);

  virtual ~NodeRemovedMessage();

  /**
   * @brief Returns the index at which the pipeline node was removed.
   * @return size_t
   */
  size_t getIndex() const;

private:
  size_t m_Index;
};
} // namespace complex
