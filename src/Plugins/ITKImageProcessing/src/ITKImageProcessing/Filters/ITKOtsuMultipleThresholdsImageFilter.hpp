#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKOtsuMultipleThresholdsImageFilter
 * @brief Threshold an image using multiple Otsu Thresholds.
 *
 * This filter creates a labeled image that separates the input image into various classes. The filter computes the thresholds using the OtsuMultipleThresholdsCalculator and applies those thresholds
 * to the input image using the ThresholdLabelerImageFilter . The NumberOfHistogramBins and NumberOfThresholds can be set for the Calculator. The LabelOffset can be set for the
 * ThresholdLabelerImageFilter .
 *
 * This filter also includes an option to use the valley emphasis algorithm from H.F. Ng, "Automatic thresholding for defect detection", Pattern Recognition Letters, (27): 1644-1649, 2006. The valley
 * emphasis algorithm is particularly effective when the object to be thresholded is small. See the following tests for examples: itkOtsuMultipleThresholdsImageFilterTest3 and
 * itkOtsuMultipleThresholdsImageFilterTest4 To use this algorithm, simple call the setter: SetValleyEmphasis(true) It is turned off by default.
 *
 * @see ScalarImageToHistogramGenerator
 *
 *
 * @see OtsuMultipleThresholdsCalculator
 *
 *
 * @see ThresholdLabelerImageFilter
 *
 * ITK Module: ITKThresholding
 * ITK Group: Thresholding
 */
class ITKIMAGEPROCESSING_EXPORT ITKOtsuMultipleThresholdsImageFilter : public IFilter
{
public:
  ITKOtsuMultipleThresholdsImageFilter() = default;
  ~ITKOtsuMultipleThresholdsImageFilter() noexcept override = default;

  ITKOtsuMultipleThresholdsImageFilter(const ITKOtsuMultipleThresholdsImageFilter&) = delete;
  ITKOtsuMultipleThresholdsImageFilter(ITKOtsuMultipleThresholdsImageFilter&&) noexcept = delete;

  ITKOtsuMultipleThresholdsImageFilter& operator=(const ITKOtsuMultipleThresholdsImageFilter&) = delete;
  ITKOtsuMultipleThresholdsImageFilter& operator=(ITKOtsuMultipleThresholdsImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_NumberOfThresholds_Key = "number_of_thresholds";
  static inline constexpr StringLiteral k_LabelOffset_Key = "label_offset";
  static inline constexpr StringLiteral k_NumberOfHistogramBins_Key = "number_of_histogram_bins";
  static inline constexpr StringLiteral k_ValleyEmphasis_Key = "valley_emphasis";
  static inline constexpr StringLiteral k_ReturnBinMidpoint_Key = "return_bin_midpoint";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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
   * @param shouldCancel Boolean that gets set if the filter should stop executing and return
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Boolean that gets set if the filter should stop executing and return
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKOtsuMultipleThresholdsImageFilter, "30f37bcd-701f-4e64-aa9d-1181469d3fb5");
