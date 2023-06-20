#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class MergeColoniesFilter
 * @brief This filter will ....
 */
class COMPLEXCORE_EXPORT MergeColoniesFilter : public IFilter
{
public:
  MergeColoniesFilter() = default;
  ~MergeColoniesFilter() noexcept override = default;

  MergeColoniesFilter(const MergeColoniesFilter&) = delete;
  MergeColoniesFilter(MergeColoniesFilter&&) noexcept = delete;

  MergeColoniesFilter& operator=(const MergeColoniesFilter&) = delete;
  MergeColoniesFilter& operator=(MergeColoniesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_UseNonContiguousNeighbors_Key = "use_non_contiguous_neighbors";
  static inline constexpr StringLiteral k_NonContiguousNeighborListArrayPath_Key = "non_contiguous_neighbor_list_array_path";
  static inline constexpr StringLiteral k_ContiguousNeighborListArrayPath_Key = "contiguous_neighbor_list_array_path";
  static inline constexpr StringLiteral k_AxisTolerance_Key = "axis_tolerance";
  static inline constexpr StringLiteral k_AngleTolerance_Key = "angle_tolerance";
  static inline constexpr StringLiteral k_IdentifyGlobAlpha_Key = "identify_glob_alpha";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "avg_quats_array_path";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_CellParentIdsArrayName_Key = "cell_parent_ids_array_name";
  static inline constexpr StringLiteral k_GlobAlphaArrayName_Key = "glob_alpha_array_name";
  static inline constexpr StringLiteral k_NewCellFeatureAttributeMatrixName_Key = "new_cell_feature_attribute_matrix_name";
  static inline constexpr StringLiteral k_FeatureParentIdsArrayName_Key = "feature_parent_ids_array_name";
  static inline constexpr StringLiteral k_ActiveArrayName_Key = "active_array_name";

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
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, MergeColoniesFilter, "7e3dbc15-51a3-482c-97c2-f82f7af685bf");
/* LEGACY UUID FOR THIS FILTER 2c4a6d83-6a1b-56d8-9f65-9453b28845b9 */
