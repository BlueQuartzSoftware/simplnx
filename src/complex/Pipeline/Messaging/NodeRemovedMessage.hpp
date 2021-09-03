#pragma once

#include <cstddef>

#include "IPipelineMessage.hpp"

namespace complex
{
class Pipeline;

/**
 * @class NodeRemovedMessage
 * @brief
 */
class COMPLEX_EXPORT NodeRemovedMessage : public IPipelineMessage
{
public:
  /**
   * @brief
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
