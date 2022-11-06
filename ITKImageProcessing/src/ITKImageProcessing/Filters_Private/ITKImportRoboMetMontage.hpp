#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKImportRoboMetMontage
 * @brief This filter will ....
 */
class ITKIMAGEPROCESSING_EXPORT ITKImportRoboMetMontage : public IFilter
{
public:
  ITKImportRoboMetMontage() = default;
  ~ITKImportRoboMetMontage() noexcept override = default;

  ITKImportRoboMetMontage(const ITKImportRoboMetMontage&) = delete;
  ITKImportRoboMetMontage(ITKImportRoboMetMontage&&) noexcept = delete;

  ITKImportRoboMetMontage& operator=(const ITKImportRoboMetMontage&) = delete;
  ITKImportRoboMetMontage& operator=(ITKImportRoboMetMontage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFile_Key = "input_file";
  static inline constexpr StringLiteral k_MontageName_Key = "montage_name";
  static inline constexpr StringLiteral k_ColumnMontageLimits_Key = "column_montage_limits";
  static inline constexpr StringLiteral k_RowMontageLimits_Key = "row_montage_limits";
  static inline constexpr StringLiteral k_LengthUnit_Key = "length_unit";
  static inline constexpr StringLiteral k_SliceNumber_Key = "slice_number";
  static inline constexpr StringLiteral k_ImageFilePrefix_Key = "image_file_prefix";
  static inline constexpr StringLiteral k_ImageFileExtension_Key = "image_file_extension";
  static inline constexpr StringLiteral k_ChangeOrigin_Key = "change_origin";
  static inline constexpr StringLiteral k_Origin_Key = "origin";
  static inline constexpr StringLiteral k_ChangeSpacing_Key = "change_spacing";
  static inline constexpr StringLiteral k_Spacing_Key = "spacing";
  static inline constexpr StringLiteral k_ConvertToGrayScale_Key = "convert_to_gray_scale";
  static inline constexpr StringLiteral k_ColorWeights_Key = "color_weights";
  static inline constexpr StringLiteral k_DataContainerPath_Key = "data_container_path";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_ImageDataArrayName_Key = "image_data_array_name";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKImportRoboMetMontage, "56b6e862-13f4-4c90-87e9-ec76db1c584b");
