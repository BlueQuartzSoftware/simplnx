#pragma once

#include "simplnx/Pipeline/AbstractPipelineNode.hpp"

#include <optional>

namespace nx::core
{
/**
 * @class AbstractPipelineFilter
 * @brief Base class for PipelineFilter and PlaceholderFilter
 */
class SIMPLNX_EXPORT AbstractPipelineFilter : public AbstractPipelineNode
{
public:
  enum class FilterType
  {
    Filter,
    Placeholder
  };

  /**
   * @brief
   */
  ~AbstractPipelineFilter() noexcept override;

  /**
   * @brief Returns the node type for quick type checking.
   * @return NodeType
   */
  NodeType getType() const override;

  /**
   * @brief Sets the index of this filter in an executing pipeline
   * @param index
   */
  void setIndex(int32 index);

  /**
   * @brief Returns the caller-assigned notification index when one exists.
   * @return The assigned index, including zero, or no value for an unassigned filter.
   */
  std::optional<int32> getAssignedIndex() const;

  /**
   * @brief Returns the type of filter of this node (filter or placeholder)
   * @return AbstractPipelineFilter::FilterType
   */
  virtual FilterType getFilterType() const = 0;

protected:
  /**
   * @brief
   */
  AbstractPipelineFilter();

  int32 m_Index = 0;

private:
  bool m_HasAssignedIndex = false;
};
} // namespace nx::core
