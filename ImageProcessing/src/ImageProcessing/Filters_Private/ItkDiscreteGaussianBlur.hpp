#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ItkDiscreteGaussianBlur
 * @brief This filter will ....
 */
class IMAGEPROCESSING_EXPORT ItkDiscreteGaussianBlur : public IFilter
{
public:
  ItkDiscreteGaussianBlur() = default;
  ~ItkDiscreteGaussianBlur() noexcept override = default;

  ItkDiscreteGaussianBlur(const ItkDiscreteGaussianBlur&) = delete;
  ItkDiscreteGaussianBlur(ItkDiscreteGaussianBlur&&) noexcept = delete;

  ItkDiscreteGaussianBlur& operator=(const ItkDiscreteGaussianBlur&) = delete;
  ItkDiscreteGaussianBlur& operator=(ItkDiscreteGaussianBlur&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SaveAsNewArray_Key = "save_as_new_array";
  static inline constexpr StringLiteral k_SelectedCellArrayPath_Key = "selected_cell_array_path";
  static inline constexpr StringLiteral k_NewCellArrayName_Key = "new_cell_array_name";
  static inline constexpr StringLiteral k_Stdev_Key = "stdev";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ItkDiscreteGaussianBlur, "59aa4ee5-be66-4f48-89ec-da2b0d1a04fc");
