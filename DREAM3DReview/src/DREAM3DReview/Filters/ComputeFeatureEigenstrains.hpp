#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class ComputeFeatureEigenstrains
 * @brief This filter will ....
 */
class COMPLEX_EXPORT ComputeFeatureEigenstrains : public IFilter
{
public:
  ComputeFeatureEigenstrains() = default;
  ~ComputeFeatureEigenstrains() noexcept override = default;

  ComputeFeatureEigenstrains(const ComputeFeatureEigenstrains&) = delete;
  ComputeFeatureEigenstrains(ComputeFeatureEigenstrains&&) noexcept = delete;

  ComputeFeatureEigenstrains& operator=(const ComputeFeatureEigenstrains&) = delete;
  ComputeFeatureEigenstrains& operator=(ComputeFeatureEigenstrains&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PoissonRatio_Key = "PoissonRatio";
  static inline constexpr StringLiteral k_UseEllipsoidalGrains_Key = "UseEllipsoidalGrains";
  static inline constexpr StringLiteral k_UseCorrectionalMatrix_Key = "UseCorrectionalMatrix";
  static inline constexpr StringLiteral k_Beta11_Key = "Beta11";
  static inline constexpr StringLiteral k_Beta22_Key = "Beta22";
  static inline constexpr StringLiteral k_Beta33_Key = "Beta33";
  static inline constexpr StringLiteral k_Beta23_Key = "Beta23";
  static inline constexpr StringLiteral k_Beta13_Key = "Beta13";
  static inline constexpr StringLiteral k_Beta12_Key = "Beta12";
  static inline constexpr StringLiteral k_AxisLengthsArrayPath_Key = "AxisLengthsArrayPath";
  static inline constexpr StringLiteral k_AxisEulerAnglesArrayPath_Key = "AxisEulerAnglesArrayPath";
  static inline constexpr StringLiteral k_ElasticStrainsArrayPath_Key = "ElasticStrainsArrayPath";
  static inline constexpr StringLiteral k_EigenstrainsArrayName_Key = "EigenstrainsArrayName";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ComputeFeatureEigenstrains, "879e1eb8-40dc-5a5b-abe5-7e0baa77ed73");
