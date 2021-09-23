#pragma once

#include <vector>

#include "complex/Pipeline/Messaging/IPipelineMessage.hpp"

namespace complex
{
struct Error;
struct Warning;
class FilterNode;

/**
 * @class FilterPreflightMessage
 * @brief The FilterPreflightMessage class is emitted when a FilterNode
 * finishes preflighting. It contains vectors of both the resulting warnings
 * and errors.
 */
class COMPLEX_EXPORT FilterPreflightMessage : public IPipelineMessage
{
public:
  /**
   * @brief Constructs a new message using the FilterNode and the resulting vector of errors and warnings.
   * @param filterNode
   * @param warnings
   * @param errors
   */
  FilterPreflightMessage(FilterNode* filterNode, const std::vector<complex::Warning>& warnings, const std::vector<complex::Error>& errors);

  virtual ~FilterPreflightMessage();

  /**
   * @brief Returns the FilterNode used to construct the message.
   * @return FilterNode*
   */
  FilterNode* getFilterNode() const;

  /**
   * @brief Returns a vector of warnings resulting from preflighting the FilterNode.
   * @return std::vector<complex::Warning>
   */
  std::vector<complex::Warning> getWarnings() const;

  /**
   * @brief Returns a vector of errors resulting from preflighting the FilterNode.
   * @return std::vector<complex::Error>
   */
  std::vector<complex::Error> getErrors() const;

private:
  FilterNode* m_FilteNode;
  std::vector<complex::Warning> m_Warnings;
  std::vector<complex::Error> m_Errors;
};
} // namespace complex
