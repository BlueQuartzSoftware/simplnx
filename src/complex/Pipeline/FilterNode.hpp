#pragma once

#include "IPipelineNode.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Filter/Messaging/FilterObserver.hpp"

namespace complex
{
class FilterHandle;

/**
 * @class FilterNode
 * @brief The FilterNode class is a pipeline node specialized for wrapping an
 * IFilter object. The node keeps track of the resulting DataStructure as well
 * as error and warning messages.
 */
class COMPLEX_EXPORT FilterNode : public IPipelineNode, public FilterObserver
{
public:
  using Messages = std::vector<std::shared_ptr<FilterMessage>>;

  /**
   * @brief Attempts to construct a FilterNode based on the specified
   * FilterHandle. Returns nullptr if the corresponding filter could not be
   * created.
   * @param handle
   * @return FilterNode*
   */
  static FilterNode* Create(const FilterHandle& handle);

  /**
   * @brief Constructs a FilterNode with the specified filter.
   *
   * The unique_ptr is moved to the FilterNode.
   * @param filter
   */
  FilterNode(IFilter::UniquePointer&& filter);

  virtual ~FilterNode();

  /**
   * @brief Returns the node type for quick type checking.
   * @return NodeType
   */
  NodeType getType() const override;

  /**
   * @brief Returns the filter's human label.
   * @return std::string
   */
  std::string getName() override;

  /**
   * @brief Returns a pointer to the wrapped filter.
   * @return IFilter*
   */
  IFilter* getFilter() const;

  /**
   * @brief Returns a collection of the filter's parameters.
   * @return Parameters
   */
  Parameters getParameters() const;

  /**
   * @brief Returns a collection of the node's filter arguments.
   * @return Arguments
   */
  Arguments getArguments() const;

  /**
   * @brief Sets the node's filter arguments and marks the node as dirty.
   * @param args
   */
  void setArguments(const Arguments& args);

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  bool preflight(DataStructure& data) override;

  /**
   * @brief Attempts to execute the node using the provided DataStructure.
   * Returns true if execution succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  bool execute(DataStructure& data) override;

  /**
   * @brief Returns a collection of warning messages emitted by the target filter.
   * This collection is cleared when the node is preflighted or executed.
   * @return Messages
   */
  Messages getWarnings() const;

  /**
   * @brief Returns a collection of error messages emitted by the target filter.
   * This collection is cleared when the node is preflighted or executed.
   * @return Messages
   */
  Messages getErrors() const;

protected:
  /**
   * @brief Clears all error and warning messages.
   */
  void clearMsgs();

  /**
   * @brief Called when an observed filter notifies observers of a message.
   * @param filter
   * @param msg
   */
  void onNotify(IFilter* filter, const std::shared_ptr<FilterMessage>& msg) override;

private:
  IFilter::UniquePointer m_Filter;
  Arguments m_Arguments;
  Messages m_WarningMsgs;
  Messages m_ErrorMsgs;
};
} // namespace complex
