#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class EstablishFoamMorphology
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT EstablishFoamMorphology : public IFilter
{
public:
  EstablishFoamMorphology() = default;
  ~EstablishFoamMorphology() noexcept override = default;

  EstablishFoamMorphology(const EstablishFoamMorphology&) = delete;
  EstablishFoamMorphology(EstablishFoamMorphology&&) noexcept = delete;

  EstablishFoamMorphology& operator=(const EstablishFoamMorphology&) = delete;
  EstablishFoamMorphology& operator=(EstablishFoamMorphology&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PeriodicBoundaries_Key = "PeriodicBoundaries";
  static inline constexpr StringLiteral k_DataContainerName_Key = "DataContainerName";
  static inline constexpr StringLiteral k_InputStatsArrayPath_Key = "InputStatsArrayPath";
  static inline constexpr StringLiteral k_InputPhaseTypesArrayPath_Key = "InputPhaseTypesArrayPath";
  static inline constexpr StringLiteral k_InputPhaseNamesArrayPath_Key = "InputPhaseNamesArrayPath";
  static inline constexpr StringLiteral k_InputShapeTypesArrayPath_Key = "InputShapeTypesArrayPath";
  static inline constexpr StringLiteral k_InputCellFeatureIdsArrayPath_Key = "InputCellFeatureIdsArrayPath";
  static inline constexpr StringLiteral k_OutputCellEnsembleAttributeMatrixName_Key = "OutputCellEnsembleAttributeMatrixName";
  static inline constexpr StringLiteral k_NumFeaturesArrayName_Key = "NumFeaturesArrayName";
  static inline constexpr StringLiteral k_OutputCellFeatureAttributeMatrixName_Key = "OutputCellFeatureAttributeMatrixName";
  static inline constexpr StringLiteral k_FeatureIdsArrayName_Key = "FeatureIdsArrayName";
  static inline constexpr StringLiteral k_MaskArrayName_Key = "MaskArrayName";
  static inline constexpr StringLiteral k_CellPhasesArrayName_Key = "CellPhasesArrayName";
  static inline constexpr StringLiteral k_QPEuclideanDistancesArrayName_Key = "QPEuclideanDistancesArrayName";
  static inline constexpr StringLiteral k_TJEuclideanDistancesArrayName_Key = "TJEuclideanDistancesArrayName";
  static inline constexpr StringLiteral k_GBEuclideanDistancesArrayName_Key = "GBEuclideanDistancesArrayName";
  static inline constexpr StringLiteral k_WriteGoalAttributes_Key = "WriteGoalAttributes";
  static inline constexpr StringLiteral k_CsvOutputFile_Key = "CsvOutputFile";
  static inline constexpr StringLiteral k_HaveFeatures_Key = "HaveFeatures";
  static inline constexpr StringLiteral k_MinStrutThickness_Key = "MinStrutThickness";
  static inline constexpr StringLiteral k_StrutThicknessVariability_Key = "StrutThicknessVariability";
  static inline constexpr StringLiteral k_StrutShapeVariability_Key = "StrutShapeVariability";

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

COMPLEX_DEF_FILTER_TRAITS(complex, EstablishFoamMorphology, "bd76bba1-00d4-4acd-88e0-05578525a8cb");
