#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class InsertTransformationPhases
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT InsertTransformationPhases : public IFilter
{
public:
  InsertTransformationPhases() = default;
  ~InsertTransformationPhases() noexcept override = default;

  InsertTransformationPhases(const InsertTransformationPhases&) = delete;
  InsertTransformationPhases(InsertTransformationPhases&&) noexcept = delete;

  InsertTransformationPhases& operator=(const InsertTransformationPhases&) = delete;
  InsertTransformationPhases& operator=(InsertTransformationPhases&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ParentPhase_Key = "parent_phase";
  static inline constexpr StringLiteral k_TransCrystalStruct_Key = "trans_crystal_struct";
  static inline constexpr StringLiteral k_TransformationPhaseMisorientation_Key = "transformation_phase_misorientation";
  static inline constexpr StringLiteral k_DefineHabitPlane_Key = "define_habit_plane";
  static inline constexpr StringLiteral k_TransformationPhaseHabitPlane_Key = "transformation_phase_habit_plane";
  static inline constexpr StringLiteral k_UseAllVariants_Key = "use_all_variants";
  static inline constexpr StringLiteral k_CoherentFrac_Key = "coherent_frac";
  static inline constexpr StringLiteral k_TransformationPhaseThickness_Key = "transformation_phase_thickness";
  static inline constexpr StringLiteral k_NumTransformationPhasesPerFeature_Key = "num_transformation_phases_per_feature";
  static inline constexpr StringLiteral k_PeninsulaFrac_Key = "peninsula_frac";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixName_Key = "cell_feature_attribute_matrix_name";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayPath_Key = "feature_euler_angles_array_path";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "avg_quats_array_path";
  static inline constexpr StringLiteral k_CentroidsArrayPath_Key = "centroids_array_path";
  static inline constexpr StringLiteral k_EquivalentDiametersArrayPath_Key = "equivalent_diameters_array_path";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_StatsGenCellEnsembleAttributeMatrixPath_Key = "stats_gen_cell_ensemble_attribute_matrix_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_PhaseTypesArrayPath_Key = "phase_types_array_path";
  static inline constexpr StringLiteral k_ShapeTypesArrayPath_Key = "shape_types_array_path";
  static inline constexpr StringLiteral k_NumFeaturesArrayPath_Key = "num_features_array_path";
  static inline constexpr StringLiteral k_FeatureParentIdsArrayName_Key = "feature_parent_ids_array_name";
  static inline constexpr StringLiteral k_NumFeaturesPerParentArrayPath_Key = "num_features_per_parent_array_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, InsertTransformationPhases, "23682d16-3f6c-4a03-8a94-f2a29cfc4b8d");
