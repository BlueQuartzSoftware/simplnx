#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WriteImageFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT WriteImageFilter : public IFilter
{
public:
  WriteImageFilter() = default;
  ~WriteImageFilter() noexcept override = default;

  WriteImageFilter(const WriteImageFilter&) = delete;
  WriteImageFilter(WriteImageFilter&&) noexcept = delete;

  WriteImageFilter& operator=(const WriteImageFilter&) = delete;
  WriteImageFilter& operator=(WriteImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_Plane_Key = "plane_index";
  static constexpr StringLiteral k_FileName_Key = "file_name";
  static constexpr StringLiteral k_IndexOffset_Key = "index_offset";
  static constexpr StringLiteral k_ImageArrayPath_Key = "image_array_path";
  static constexpr StringLiteral k_ImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_TotalIndexDigits_Key = "total_index_digits";
  static constexpr StringLiteral k_LeadingDigitCharacter_Key = "leading_digit_character";
  static constexpr StringLiteral k_CreateColorTable_Key = "create_color_table";
  static constexpr StringLiteral k_SelectedPreset_Key = "selected_preset";
  static constexpr StringLiteral k_UseMask_Key = "use_mask";
  static constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static constexpr StringLiteral k_InvalidColorValue_Key = "invalid_color_value";

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
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteImageFilter, "a8b920c7-5445-4c8a-b7d7-6cabc578d587");
