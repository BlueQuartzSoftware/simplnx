#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class EbsdToH5EbsdFilter
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT EbsdToH5EbsdFilter : public IFilter
{
public:
  EbsdToH5EbsdFilter() = default;
  ~EbsdToH5EbsdFilter() noexcept override = default;

  EbsdToH5EbsdFilter(const EbsdToH5EbsdFilter&) = delete;
  EbsdToH5EbsdFilter(EbsdToH5EbsdFilter&&) noexcept = delete;

  EbsdToH5EbsdFilter& operator=(const EbsdToH5EbsdFilter&) = delete;
  EbsdToH5EbsdFilter& operator=(EbsdToH5EbsdFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ZSpacing_Key = "z_spacing";
  static inline constexpr StringLiteral k_StackingOrder_Key = "stacking_order_index";
  static inline constexpr StringLiteral k_ReferenceFrame_Key = "reference_frame_index";
  static inline constexpr StringLiteral k_OutputPath_Key = "output_file_path";
  static inline constexpr StringLiteral k_InputFileListInfo_Key = "input_file_list_object";

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
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, EbsdToH5EbsdFilter, "2d05ca72-0a1b-4aec-b9b0-bc470845c448");
