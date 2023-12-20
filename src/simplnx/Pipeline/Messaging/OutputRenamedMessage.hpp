#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"

namespace nx::core
{
class AbstractPipelineNode;
class Pipeline;

/**
 * @class OutputRenamedMessage
 * @brief The OutputRenamedMessage class exists to notify PipelineFilter observers that a
 * specified node's created DataPath was changed to the a new value.
 */
class SIMPLNX_EXPORT OutputRenamedMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a OutputRenamedMessage specifying which AbstractPipelineNode
   * was added to the target Pipeline and the index it was added at.
   * @param pipeline
   * @param newNode
   * @param index
   */
  OutputRenamedMessage(AbstractPipelineNode* pipeline, const PipelineFilter::RenamedPath& renamedOutput);

  ~OutputRenamedMessage() noexcept override;

  /**
   * @brief Returns the PipelineFilter::RenamedPath pair.
   * @return PipelineFilter::RenamedPath
   */
  PipelineFilter::RenamedPath getRenamedPath() const;

  /**
   * @brief Returns the previous output path
   * @return DataPath
   */
  DataPath getPreviousPath() const;

  /**
   * @brief Returns the new output path
   * @return DataPath
   */
  DataPath getNewPath() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  PipelineFilter::RenamedPath m_RenamedPath;
};
} // namespace nx::core
