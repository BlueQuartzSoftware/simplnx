#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindGBPDMetricBased
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT FindGBPDMetricBased : public IFilter
{
public:
  FindGBPDMetricBased() = default;
  ~FindGBPDMetricBased() noexcept override = default;

  FindGBPDMetricBased(const FindGBPDMetricBased&) = delete;
  FindGBPDMetricBased(FindGBPDMetricBased&&) noexcept = delete;

  FindGBPDMetricBased& operator=(const FindGBPDMetricBased&) = delete;
  FindGBPDMetricBased& operator=(FindGBPDMetricBased&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PhaseOfInterest_Key = "PhaseOfInterest";
  static inline constexpr StringLiteral k_LimitDist_Key = "LimitDist";
  static inline constexpr StringLiteral k_NumSamplPts_Key = "NumSamplPts";
  static inline constexpr StringLiteral k_ExcludeTripleLines_Key = "ExcludeTripleLines";
  static inline constexpr StringLiteral k_DistOutputFile_Key = "DistOutputFile";
  static inline constexpr StringLiteral k_ErrOutputFile_Key = "ErrOutputFile";
  static inline constexpr StringLiteral k_SaveRelativeErr_Key = "SaveRelativeErr";
  static inline constexpr StringLiteral k_NodeTypesArrayPath_Key = "NodeTypesArrayPath";
  static inline constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPath_Key = "SurfaceMeshFaceLabelsArrayPath";
  static inline constexpr StringLiteral k_SurfaceMeshFaceNormalsArrayPath_Key = "SurfaceMeshFaceNormalsArrayPath";
  static inline constexpr StringLiteral k_SurfaceMeshFaceAreasArrayPath_Key = "SurfaceMeshFaceAreasArrayPath";
  static inline constexpr StringLiteral k_SurfaceMeshFeatureFaceLabelsArrayPath_Key = "SurfaceMeshFeatureFaceLabelsArrayPath";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayPath_Key = "FeatureEulerAnglesArrayPath";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";

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
  Result<OutputActions> preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindGBPDMetricBased, "aab9db9c-ed7e-5d19-b6e0-4564f7bc7295");
