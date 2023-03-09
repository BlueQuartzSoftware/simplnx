#pragma once

#include "complex/Common/Result.hpp"
#include "complex/Pipeline/Messaging/AbstractPipelineMessage.hpp"

#include <vector>

namespace complex
{
class PipelineFilter;

/**
 * @class FilterPreflightMessage
 * @brief The FilterPreflightMessage class is emitted when a PipelineFilter
 * finishes preflighting. Its sole purpose is to pass along vectors of the
 * resulting errors and warnings.
 */
class COMPLEX_EXPORT FilterPreflightMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a new message using the PipelineFilter and the resulting vector of errors and warnings.
   * @param filterNode
   * @param warnings
   * @param errors
   */
  FilterPreflightMessage(PipelineFilter* filterNode, const std::vector<complex::Warning>& warnings, const std::vector<complex::Error>& errors);

  ~FilterPreflightMessage() override;

  /**
   * @brief Returns a pointer to the target PipelineFilter.
   * @return PipelineFilter*
   */
  PipelineFilter* getFilterNode() const;

  /**
   * @brief Returns a vector of warnings resulting from preflighting the PipelineFilter.
   * @return std::vector<complex::Warning>
   */
  std::vector<complex::Warning> getWarnings() const;

  /**
   * @brief Returns a vector of errors resulting from preflighting the PipelineFilter.
   * @return std::vector<complex::Error>
   */
  std::vector<complex::Error> getErrors() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  std::vector<complex::Warning> m_Warnings;
  std::vector<complex::Error> m_Errors;
};
} // namespace complex
