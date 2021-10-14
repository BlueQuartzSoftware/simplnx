#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class Lesson5
 * @brief This filter will ....
 */
class COMPLEX_EXPORT Lesson5 : public IFilter
{
public:
  Lesson5() = default;
  ~Lesson5() noexcept override = default;

  Lesson5(const Lesson5&) = delete;
  Lesson5(Lesson5&&) noexcept = delete;

  Lesson5& operator=(const Lesson5&) = delete;
  Lesson5& operator=(Lesson5&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputDataArrayPath_Key = "InputDataArrayPath";
  static inline constexpr StringLiteral k_OutputDataArrayPath_Key = "OutputDataArrayPath";
  static inline constexpr StringLiteral k_Value_Key = "Value";
  static inline constexpr StringLiteral k_Operator_Key = "Operator";
  static inline constexpr StringLiteral k_Selection_Key = "Selection";
  static inline constexpr StringLiteral k_FloatValue_Key = "FloatValue";

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

COMPLEX_DEF_FILTER_TRAITS(complex::Lesson5, "f41ad0cb-13c7-5e2e-8ae4-1ea73d426307");
