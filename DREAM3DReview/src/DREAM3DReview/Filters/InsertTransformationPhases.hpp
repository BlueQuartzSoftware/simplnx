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
  static inline constexpr StringLiteral k_ParentPhase_Key = "ParentPhase";
  static inline constexpr StringLiteral k_TransCrystalStruct_Key = "TransCrystalStruct";
  static inline constexpr StringLiteral k_TransformationPhaseMisorientation_Key = "TransformationPhaseMisorientation";
  static inline constexpr StringLiteral k_DefineHabitPlane_Key = "DefineHabitPlane";
  static inline constexpr StringLiteral k_TransformationPhaseHabitPlane_Key = "TransformationPhaseHabitPlane";
  static inline constexpr StringLiteral k_UseAllVariants_Key = "UseAllVariants";
  static inline constexpr StringLiteral k_CoherentFrac_Key = "CoherentFrac";
  static inline constexpr StringLiteral k_TransformationPhaseThickness_Key = "TransformationPhaseThickness";
  static inline constexpr StringLiteral k_NumTransformationPhasesPerFeature_Key = "NumTransformationPhasesPerFeature";
  static inline constexpr StringLiteral k_PeninsulaFrac_Key = "PeninsulaFrac";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "CellEulerAnglesArrayPath";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "CellPhasesArrayPath";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixName_Key = "CellFeatureAttributeMatrixName";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayPath_Key = "FeatureEulerAnglesArrayPath";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "AvgQuatsArrayPath";
  static inline constexpr StringLiteral k_CentroidsArrayPath_Key = "CentroidsArrayPath";
  static inline constexpr StringLiteral k_EquivalentDiametersArrayPath_Key = "EquivalentDiametersArrayPath";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";
  static inline constexpr StringLiteral k_StatsGenCellEnsembleAttributeMatrixPath_Key = "StatsGenCellEnsembleAttributeMatrixPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";
  static inline constexpr StringLiteral k_PhaseTypesArrayPath_Key = "PhaseTypesArrayPath";
  static inline constexpr StringLiteral k_ShapeTypesArrayPath_Key = "ShapeTypesArrayPath";
  static inline constexpr StringLiteral k_NumFeaturesArrayPath_Key = "NumFeaturesArrayPath";
  static inline constexpr StringLiteral k_FeatureParentIdsArrayName_Key = "FeatureParentIdsArrayName";
  static inline constexpr StringLiteral k_NumFeaturesPerParentArrayPath_Key = "NumFeaturesPerParentArrayPath";

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
