#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindSlipTransmissionMetrics
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT FindSlipTransmissionMetrics : public IFilter
{
public:
  FindSlipTransmissionMetrics() = default;
  ~FindSlipTransmissionMetrics() noexcept override = default;

  FindSlipTransmissionMetrics(const FindSlipTransmissionMetrics&) = delete;
  FindSlipTransmissionMetrics(FindSlipTransmissionMetrics&&) noexcept = delete;

  FindSlipTransmissionMetrics& operator=(const FindSlipTransmissionMetrics&) = delete;
  FindSlipTransmissionMetrics& operator=(FindSlipTransmissionMetrics&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_NeighborListArrayPath_Key = "NeighborListArrayPath";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "AvgQuatsArrayPath";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";
  static inline constexpr StringLiteral k_F1ListArrayName_Key = "F1ListArrayName";
  static inline constexpr StringLiteral k_F1sptListArrayName_Key = "F1sptListArrayName";
  static inline constexpr StringLiteral k_F7ListArrayName_Key = "F7ListArrayName";
  static inline constexpr StringLiteral k_mPrimeListArrayName_Key = "mPrimeListArrayName";

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindSlipTransmissionMetrics, "97523038-5fb2-5e82-9177-ed3e8b24b4bd");
