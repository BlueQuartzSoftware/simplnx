#pragma once

#include "complex/Filter/IFilter.hpp"
#include "complex/Pipeline/Messaging/AbstractPipelineMessage.hpp"

namespace complex
{
class PipelineFilter;

/**
 * @class PipelineFilterMessage
 * @brief The PipelineFilterMessage class exists for alerting PipelineFilter
 * listeners to IFilter::Messages.
 */
class COMPLEX_EXPORT PipelineFilterMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a PipelineFilterMessage for the given node with the IFilter::Message.
   * @param filter
   * @param filterMessage
   */
  PipelineFilterMessage(PipelineFilter* filter, const IFilter::Message& filterMessage);

  virtual ~PipelineFilterMessage();

  /**
   * @brief Returns the IFilter::Message
   * @return IFilter::Message
   */
  IFilter::Message getFilterMessage() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  IFilter::Message m_FilterMessage;
};
} // namespace complex
