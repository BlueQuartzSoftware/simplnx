#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class EMsoftSO3Sampler
 * @brief This filter will ....
 */
class COMPLEX_EXPORT EMsoftSO3Sampler : public IFilter
{
public:
  EMsoftSO3Sampler() = default;
  ~EMsoftSO3Sampler() noexcept override = default;

  EMsoftSO3Sampler(const EMsoftSO3Sampler&) = delete;
  EMsoftSO3Sampler(EMsoftSO3Sampler&&) noexcept = delete;

  EMsoftSO3Sampler& operator=(const EMsoftSO3Sampler&) = delete;
  EMsoftSO3Sampler& operator=(EMsoftSO3Sampler&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_sampleModeSelector_Key = "sampleModeSelector";
  static inline constexpr StringLiteral k_PointGroup_Key = "PointGroup";
  static inline constexpr StringLiteral k_OffsetGrid_Key = "OffsetGrid";
  static inline constexpr StringLiteral k_MisOr_Key = "MisOr";
  static inline constexpr StringLiteral k_RefOr_Key = "RefOr";
  static inline constexpr StringLiteral k_MisOrFull_Key = "MisOrFull";
  static inline constexpr StringLiteral k_RefOrFull_Key = "RefOrFull";
  static inline constexpr StringLiteral k_Numsp_Key = "Numsp";
  static inline constexpr StringLiteral k_EulerAnglesArrayName_Key = "EulerAnglesArrayName";

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

COMPLEX_DEF_FILTER_TRAITS(complex, EMsoftSO3Sampler, "98d7e2a0-ab15-5a3a-ae5d-9926c7fe74b7");
