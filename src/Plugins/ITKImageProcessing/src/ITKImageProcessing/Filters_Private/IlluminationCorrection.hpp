#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class IlluminationCorrection
 * @brief This filter will ....
 */
class ITKIMAGEPROCESSING_EXPORT IlluminationCorrection : public IFilter
{
public:
  IlluminationCorrection() = default;
  ~IlluminationCorrection() noexcept override = default;

  IlluminationCorrection(const IlluminationCorrection&) = delete;
  IlluminationCorrection(IlluminationCorrection&&) noexcept = delete;

  IlluminationCorrection& operator=(const IlluminationCorrection&) = delete;
  IlluminationCorrection& operator=(IlluminationCorrection&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MontageSelection_Key = "montage_selection";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_ImageDataArrayName_Key = "image_data_array_name";
  static inline constexpr StringLiteral k_CorrectedImageDataArrayName_Key = "corrected_image_data_array_name";
  static inline constexpr StringLiteral k_BackgroundDataContainerPath_Key = "background_data_container_path";
  static inline constexpr StringLiteral k_BackgroundCellAttributeMatrixPath_Key = "background_cell_attribute_matrix_path";
  static inline constexpr StringLiteral k_BackgroundImageArrayPath_Key = "background_image_array_path";
  static inline constexpr StringLiteral k_LowThreshold_Key = "low_threshold";
  static inline constexpr StringLiteral k_HighThreshold_Key = "high_threshold";
  static inline constexpr StringLiteral k_ApplyMedianFilter_Key = "apply_median_filter";
  static inline constexpr StringLiteral k_MedianRadius_Key = "median_radius";
  static inline constexpr StringLiteral k_ApplyCorrection_Key = "apply_correction";
  static inline constexpr StringLiteral k_ExportCorrectedImages_Key = "export_corrected_images";
  static inline constexpr StringLiteral k_OutputPath_Key = "output_path";
  static inline constexpr StringLiteral k_FileExtension_Key = "file_extension";
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "selected_image_geom_path";

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
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, IlluminationCorrection, "95866cba-a92e-4d87-b7f6-8f03928ee633");
