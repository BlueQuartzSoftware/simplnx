#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class CombineAttributeMatrices
 * @brief This filter will ....
 */
class COMPLEX_EXPORT CombineAttributeMatrices : public IFilter
{
public:
  CombineAttributeMatrices() = default;
  ~CombineAttributeMatrices() noexcept override = default;

  CombineAttributeMatrices(const CombineAttributeMatrices&) = delete;
  CombineAttributeMatrices(CombineAttributeMatrices&&) noexcept = delete;

  CombineAttributeMatrices& operator=(const CombineAttributeMatrices&) = delete;
  CombineAttributeMatrices& operator=(CombineAttributeMatrices&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FirstAttributeMatrixPath_Key = "FirstAttributeMatrixPath";
  static inline constexpr StringLiteral k_SecondAttributeMatrixPath_Key = "SecondAttributeMatrixPath";
  static inline constexpr StringLiteral k_FirstIndexArrayPath_Key = "FirstIndexArrayPath";
  static inline constexpr StringLiteral k_SecondIndexArrayPath_Key = "SecondIndexArrayPath";
  static inline constexpr StringLiteral k_NewIndexArrayName_Key = "NewIndexArrayName";
  static inline constexpr StringLiteral k_CombinedAttributeMatrixName_Key = "CombinedAttributeMatrixName";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

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

COMPLEX_DEF_FILTER_TRAITS(complex::CombineAttributeMatrices, "58e74e74-24c3-578f-89cc-f1f865f68465");
