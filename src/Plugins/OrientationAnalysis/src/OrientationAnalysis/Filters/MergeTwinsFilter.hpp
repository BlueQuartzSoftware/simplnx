#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class MergeTwinsFilter
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT MergeTwinsFilter : public IFilter
{
public:
  MergeTwinsFilter() = default;
  ~MergeTwinsFilter() noexcept override = default;

  MergeTwinsFilter(const MergeTwinsFilter&) = delete;
  MergeTwinsFilter(MergeTwinsFilter&&) noexcept = delete;

  MergeTwinsFilter& operator=(const MergeTwinsFilter&) = delete;
  MergeTwinsFilter& operator=(MergeTwinsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_UseNonContiguousNeighbors_Key = "use_non_contiguous_neighbors";
  static inline constexpr StringLiteral k_NonContiguousNeighborListArrayPath_Key = "non_contiguous_neighbor_list_array_path";
  static inline constexpr StringLiteral k_ContiguousNeighborListArrayPath_Key = "contiguous_neighbor_list_array_path";
  static inline constexpr StringLiteral k_AxisTolerance_Key = "axis_tolerance";
  static inline constexpr StringLiteral k_AngleTolerance_Key = "angle_tolerance";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "avg_quats_array_path";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_CellParentIdsArrayName_Key = "cell_parent_ids_array_name";
  static inline constexpr StringLiteral k_NewCellFeatureAttributeMatrixName_Key = "new_cell_feature_attribute_matrix_name";
  static inline constexpr StringLiteral k_FeatureParentIdsArrayName_Key = "feature_parent_ids_array_name";
  static inline constexpr StringLiteral k_ActiveArrayName_Key = "active_array_name";
  static inline constexpr StringLiteral k_UseSeed_Key = "use_seed";
  static inline constexpr StringLiteral k_SeedValue_Key = "seed_value";
  static inline constexpr StringLiteral k_SeedArrayName_Key = "seed_array_name";

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
   * @brief Returns the human readable name of the filter.
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
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, MergeTwinsFilter, "f173786a-50cd-4c3c-9518-48ef6fc2bac9");
