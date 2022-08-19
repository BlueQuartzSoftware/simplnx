#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class GenerateEnsembleStatistics
 * @brief This filter will ....
 */
class STATSTOOLBOX_EXPORT GenerateEnsembleStatistics : public IFilter
{
public:
  GenerateEnsembleStatistics() = default;
  ~GenerateEnsembleStatistics() noexcept override = default;

  GenerateEnsembleStatistics(const GenerateEnsembleStatistics&) = delete;
  GenerateEnsembleStatistics(GenerateEnsembleStatistics&&) noexcept = delete;

  GenerateEnsembleStatistics& operator=(const GenerateEnsembleStatistics&) = delete;
  GenerateEnsembleStatistics& operator=(GenerateEnsembleStatistics&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PhaseTypeArray_Key = "PhaseTypeArray";
  static inline constexpr StringLiteral k_SizeCorrelationResolution_Key = "SizeCorrelationResolution";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";
  static inline constexpr StringLiteral k_NeighborListArrayPath_Key = "NeighborListArrayPath";
  static inline constexpr StringLiteral k_CalculateMorphologicalStats_Key = "CalculateMorphologicalStats";
  static inline constexpr StringLiteral k_SizeDistributionFitType_Key = "SizeDistributionFitType";
  static inline constexpr StringLiteral k_BiasedFeaturesArrayPath_Key = "BiasedFeaturesArrayPath";
  static inline constexpr StringLiteral k_EquivalentDiametersArrayPath_Key = "EquivalentDiametersArrayPath";
  static inline constexpr StringLiteral k_AspectRatioDistributionFitType_Key = "AspectRatioDistributionFitType";
  static inline constexpr StringLiteral k_AspectRatiosArrayPath_Key = "AspectRatiosArrayPath";
  static inline constexpr StringLiteral k_Omega3DistributionFitType_Key = "Omega3DistributionFitType";
  static inline constexpr StringLiteral k_Omega3sArrayPath_Key = "Omega3sArrayPath";
  static inline constexpr StringLiteral k_NeighborhoodDistributionFitType_Key = "NeighborhoodDistributionFitType";
  static inline constexpr StringLiteral k_NeighborhoodsArrayPath_Key = "NeighborhoodsArrayPath";
  static inline constexpr StringLiteral k_AxisEulerAnglesArrayPath_Key = "AxisEulerAnglesArrayPath";
  static inline constexpr StringLiteral k_CalculateCrystallographicStats_Key = "CalculateCrystallographicStats";
  static inline constexpr StringLiteral k_SurfaceFeaturesArrayPath_Key = "SurfaceFeaturesArrayPath";
  static inline constexpr StringLiteral k_VolumesArrayPath_Key = "VolumesArrayPath";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayPath_Key = "FeatureEulerAnglesArrayPath";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "AvgQuatsArrayPath";
  static inline constexpr StringLiteral k_SharedSurfaceAreaListArrayPath_Key = "SharedSurfaceAreaListArrayPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";
  static inline constexpr StringLiteral k_PhaseTypesArrayName_Key = "PhaseTypesArrayName";
  static inline constexpr StringLiteral k_StatisticsArrayName_Key = "StatisticsArrayName";
  static inline constexpr StringLiteral k_IncludeRadialDistFunc_Key = "IncludeRadialDistFunc";
  static inline constexpr StringLiteral k_RDFArrayPath_Key = "RDFArrayPath";
  static inline constexpr StringLiteral k_MaxMinRDFArrayPath_Key = "MaxMinRDFArrayPath";

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

COMPLEX_DEF_FILTER_TRAITS(complex, GenerateEnsembleStatistics, "643e29d3-5af4-4d51-be57-060ae9f0ba37");
