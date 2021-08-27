#pragma once

#include "complex/Filter/Arguments.hpp"
#include "complex/Pipeline/Messaging/IPipelineMessage.hpp"

namespace complex
{
class FilterNode;
class IFilter;

/**
 * @class FilterArgumentsMessage
 * @brief
 */
class COMPLEX_EXPORT FilterArgumentsMessage : public IPipelineMessage
{
public:
  /**
   * @brief
   * @param node
   * @param args
   */
  FilterArgumentsMessage(FilterNode* node, const Arguments& args);

  virtual ~FilterArgumentsMessage();

  /**
   * @brief
   * @return IFilter*
   */
  IFilter* getFilter() const;

  /**
   * @brief
   * @return Arguments
   */
  Arguments getArguments() const;

private:
  IFilter* m_Filter;
  Arguments m_Args;
};
} // namespace complex
