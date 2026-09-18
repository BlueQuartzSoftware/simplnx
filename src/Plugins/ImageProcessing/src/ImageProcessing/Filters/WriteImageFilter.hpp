#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WriteImageFilter
 * @brief Writes two-dimensional slices from an Image Geometry through ITK-free image backends.
 */
class IMAGEPROCESSING_EXPORT WriteImageFilter : public IFilter
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
  static constexpr StringLiteral k_FlipMode_Key = "flip_mode_index";
  static constexpr StringLiteral k_AddScaleBar_Key = "add_scale_bar";

  /**
   * @brief Returns the filter name.
   * @return Filter name.
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ class name.
   * @return C++ class name.
   */
  std::string className() const override;

  /**
   * @brief Returns the filter UUID.
   * @return Filter UUID.
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the user-visible filter name.
   * @return User-visible filter name.
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default search tags.
   * @return Default search tags.
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the filter parameters.
   * @return Filter parameters.
   */
  Parameters parameters() const override;

  /**
   * @brief Returns the parameter schema version.
   * @return Parameter schema version.
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Creates a copy of the filter.
   * @return New filter instance.
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Validates the source data and output settings.
   * @param dataStructure Contains the selected Image Geometry and Data Arrays.
   * @param filterArgs Contains the filter parameter values.
   * @param messageHandler Receives preflight messages.
   * @param shouldCancel Indicates that preflight must stop.
   * @param executionContext Resolves relative file paths.
   * @return Output actions, display values, warnings, or errors.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Writes the selected image slices.
   * @param dataStructure Contains the selected Image Geometry and Data Arrays.
   * @param filterArgs Contains the filter parameter values.
   * @param pipelineNode Identifies the pipeline node that runs the filter.
   * @param messageHandler Receives slice progress messages.
   * @param shouldCancel Stops output between slices when set.
   * @param executionContext Resolves relative file paths.
   * @return An error if slice extraction, compression, or file output fails.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteImageFilter, "a8b920c7-5445-4c8a-b7d7-6cabc578d587");
