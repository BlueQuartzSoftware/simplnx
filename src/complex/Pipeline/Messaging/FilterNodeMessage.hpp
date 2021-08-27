#pragma once

#include <memory>

#include "IPipelineMessage.hpp"

#include "complex/Filter/IFilter.hpp"
#include "complex/Filter/Messaging/FilterMessage.hpp"

namespace complex
{
class FilterNode;

/**
 * @class FilterNodeMessage
 * @brief
 */
class COMPLEX_EXPORT FilterNodeMessage : public IPipelineMessage
{
public:
  /**
   * @brief
   * @param node
   * @param msg
   */
  FilterNodeMessage(FilterNode* node, const std::shared_ptr<FilterMessage>& msg);

  virtual ~FilterNodeMessage();

  /**
   * @brief Returns a pointer to the filter that emitted the message.
   * @return IFilter*
   */
  IFilter* getFilter() const;

  /**
   * @brief Returns a shared_ptr to the FilterMessage emitted.
   * @return std::shared_ptr<FilterMessage>
   */
  std::shared_ptr<FilterMessage> getMessage() const;

private:
  IFilter* m_Filter;
  std::shared_ptr<FilterMessage> m_Message;
};
} // namespace complex
