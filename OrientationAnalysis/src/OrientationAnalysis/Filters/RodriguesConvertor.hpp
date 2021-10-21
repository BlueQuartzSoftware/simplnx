#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class RodriguesConvertor
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT RodriguesConvertor : public IFilter
{
public:
  RodriguesConvertor() = default;
  ~RodriguesConvertor() noexcept override = default;

  RodriguesConvertor(const RodriguesConvertor&) = delete;
  RodriguesConvertor(RodriguesConvertor&&) noexcept = delete;

  RodriguesConvertor& operator=(const RodriguesConvertor&) = delete;
  RodriguesConvertor& operator=(RodriguesConvertor&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_RodriguesDataArrayPath_Key = "RodriguesDataArrayPath";
  static inline constexpr StringLiteral k_OutputDataArrayPath_Key = "OutputDataArrayPath";
  static inline constexpr StringLiteral k_DeleteOriginalData_Key = "DeleteOriginalData";

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

COMPLEX_DEF_FILTER_TRAITS(complex, RodriguesConvertor, "a2b62395-1a7d-5058-a840-752d8f8e2430");
