#pragma once

#include "simplnx/Pipeline/AbstractPipelineFilter.hpp"

#include <nlohmann/json.hpp>

namespace nx::core
{
/**
 * @class PlaceholderFilter
 * @brief PlaceholderFilter acts as a placeholder for filters that weren't able
 * to be created successfully from json. The original json is stored in case the
 * filter is able to be restored later.
 */
class SIMPLNX_EXPORT PlaceholderFilter : public AbstractPipelineFilter
{
public:
  /**
   * @brief Creates a placeholder with the saved JSON and its disabled state.
   * @param json Supplies the unavailable filter settings. A missing or non-Boolean disabled flag means enabled.
   * @return New placeholder without a parent pipeline.
   */
  static std::unique_ptr<PlaceholderFilter> Create(nlohmann::json json);

  /**
   * @brief
   */
  PlaceholderFilter();

  /**
   * @brief Constructs a placeholder with the saved JSON and its disabled state.
   * @param json Supplies the unavailable filter settings. A missing or non-Boolean disabled flag means enabled.
   */
  PlaceholderFilter(nlohmann::json json);

  /**
   * @brief
   */
  ~PlaceholderFilter() noexcept override;

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param data
   * @param shouldCancel
   * @return bool
   */
  bool preflight(DataStructure& dataStructure, const std::atomic_bool& shouldCancel) override;

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param data
   * @param renamedPaths Collection of renamed output paths.
   * @param shouldCancel
   * @param allowRenaming
   * @return bool
   */
  bool preflight(DataStructure& dataStructure, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel, bool allowRenaming) override;

  /**
   * @brief Attempts to execute the node using the provided DataStructure.
   * Returns true if execution succeeded. Otherwise, this returns false.
   * @param data
   * @param shouldCancel
   * @return bool
   */
  bool execute(DataStructure& dataStructure, const std::atomic_bool& shouldCancel) override;

  /**
   * @brief Copies the saved JSON and current disabled state into an independent placeholder.
   * The current disabled state can differ from the original JSON. The copy starts with fresh execution state and signals.
   * @return Independent placeholder without a parent pipeline.
   */
  std::unique_ptr<AbstractPipelineNode> deepCopy() const override;

  /**
   * @brief Returns the filter's human label.
   * @return std::string
   */
  std::string getName() const override;

  /**
   * @brief Returns the original json for this filter.
   * @return const nlohmann::json&
   */
  const nlohmann::json& getJson() const;

  /**
   * @brief Returns the type of filter of this node (filter or placeholder)
   * @return AbstractPipelineFilter::FilterType
   */
  FilterType getFilterType() const override;

protected:
  /**
   * @brief Returns implementation-specific json value for the node.
   * This method should only be called from toJson().
   * @return
   */
  nlohmann::json toJsonImpl() const override;

private:
  nlohmann::json m_Json;
};
} // namespace nx::core
