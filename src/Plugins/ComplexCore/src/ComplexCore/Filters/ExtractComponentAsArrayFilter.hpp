#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ExtractComponentAsArrayFilter
 * @brief This filter will extract components of an array either to a new array or
 * delete it.
 */
class COMPLEXCORE_EXPORT ExtractComponentAsArrayFilter : public IFilter
{
public:
  ExtractComponentAsArrayFilter() = default;
  ~ExtractComponentAsArrayFilter() noexcept override = default;

  ExtractComponentAsArrayFilter(const ExtractComponentAsArrayFilter&) = delete;
  ExtractComponentAsArrayFilter(ExtractComponentAsArrayFilter&&) noexcept = delete;

  ExtractComponentAsArrayFilter& operator=(const ExtractComponentAsArrayFilter&) = delete;
  ExtractComponentAsArrayFilter& operator=(ExtractComponentAsArrayFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_RemoveComponentsFromArray_Key = "remove_components_from_array";
  static inline constexpr StringLiteral k_MoveComponentsToNewArray_Key = "move_components_to_new_array";
  static inline constexpr StringLiteral k_CompNumber_Key = "comp_number";
  static inline constexpr StringLiteral k_SelectedArrayPath_Key = "selected_array_path";
  static inline constexpr StringLiteral k_NewArrayPath_Key = "new_array_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ExtractComponentAsArrayFilter, "fcc1c1cc-c37a-40fc-97fa-ce40201d34e3");
/* LEGACY UUID FOR THIS FILTER 79d59b85-01e8-5c4a-a6e1-3fd3e2ceffb4 */
