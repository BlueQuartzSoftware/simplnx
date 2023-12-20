#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp"

#include <vector>

namespace nx::core
{
class PipelineFilter;

/**
 * @class FilterPreflightMessage
 * @brief The FilterPreflightMessage class is emitted when a PipelineFilter
 * finishes preflighting. Its sole purpose is to pass along vectors of the
 * resulting errors and warnings.
 */
class SIMPLNX_EXPORT FilterPreflightMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a new message using the PipelineFilter and the resulting vector of errors and warnings.
   * @param filterNode
   * @param warnings
   * @param errors
   */
  FilterPreflightMessage(PipelineFilter* filterNode, const std::vector<nx::core::Warning>& warnings, const std::vector<nx::core::Error>& errors);

  ~FilterPreflightMessage() override;

  /**
   * @brief Returns a pointer to the target PipelineFilter.
   * @return PipelineFilter*
   */
  PipelineFilter* getFilterNode() const;

  /**
   * @brief Returns a vector of warnings resulting from preflighting the PipelineFilter.
   * @return std::vector<nx::core::Warning>
   */
  std::vector<nx::core::Warning> getWarnings() const;

  /**
   * @brief Returns a vector of errors resulting from preflighting the PipelineFilter.
   * @return std::vector<nx::core::Error>
   */
  std::vector<nx::core::Error> getErrors() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  std::vector<nx::core::Warning> m_Warnings;
  std::vector<nx::core::Error> m_Errors;
};
} // namespace nx::core
