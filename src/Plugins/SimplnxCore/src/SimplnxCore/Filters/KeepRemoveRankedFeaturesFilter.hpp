#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class KeepRemoveRankedFeaturesFilter
 * @brief This filter keeps or removes a count or percentage of Features ranked by a scalar Feature level array.
 *
 * Unlike the value based thresholds such as Remove Minimum Size Features, the cut here depends on a
 * Feature's position within the sorted population rather than on its value alone. That makes
 * requests such as "keep the 10 largest" or "remove the smallest 10%" expressible.
 */
class SIMPLNXCORE_EXPORT KeepRemoveRankedFeaturesFilter : public IFilter
{
public:
  KeepRemoveRankedFeaturesFilter() = default;
  ~KeepRemoveRankedFeaturesFilter() noexcept override = default;

  KeepRemoveRankedFeaturesFilter(const KeepRemoveRankedFeaturesFilter&) = delete;
  KeepRemoveRankedFeaturesFilter(KeepRemoveRankedFeaturesFilter&&) noexcept = delete;

  KeepRemoveRankedFeaturesFilter& operator=(const KeepRemoveRankedFeaturesFilter&) = delete;
  KeepRemoveRankedFeaturesFilter& operator=(KeepRemoveRankedFeaturesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_Operation_Key = "operation_index";
  static inline constexpr StringLiteral k_RankFrom_Key = "rank_from_index";
  static inline constexpr StringLiteral k_Criterion_Key = "selection_criterion_index";
  static inline constexpr StringLiteral k_NumFeatures_Key = "num_features";
  static inline constexpr StringLiteral k_Percent_Key = "percent";
  static inline constexpr StringLiteral k_FillRemovedFeatures_Key = "fill_removed_features";
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_RankingArrayPath_Key = "ranking_array_path";
  static inline constexpr StringLiteral k_IgnoredDataArrayPaths_Key = "ignored_data_array_paths";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human-readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns parameters version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, KeepRemoveRankedFeaturesFilter, "07eb5919-bfd3-4ab7-abdb-5af756a66bce");
