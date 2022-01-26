#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKOtsuMultipleThresholdsImage
 * @brief Threshold an image using multiple Otsu Thresholds.
 *
 * This filter creates a labeled image that separates the input image into various classes. The filter computes the thresholds using the OtsuMultipleThresholdsCalculator and applies those thresholds
 * to the input image using the ThresholdLabelerImageFilter. The NumberOfHistogramBins and NumberOfThresholds can be set for the Calculator. The LabelOffset can be set for the
 * ThresholdLabelerImageFilter.
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
class ITKIMAGEPROCESSING_EXPORT ITKOtsuMultipleThresholdsImage : public IFilter
{
public:
  ITKOtsuMultipleThresholdsImage() = default;
  ~ITKOtsuMultipleThresholdsImage() noexcept override = default;

  ITKOtsuMultipleThresholdsImage(const ITKOtsuMultipleThresholdsImage&) = delete;
  ITKOtsuMultipleThresholdsImage(ITKOtsuMultipleThresholdsImage&&) noexcept = delete;

  ITKOtsuMultipleThresholdsImage& operator=(const ITKOtsuMultipleThresholdsImage&) = delete;
  ITKOtsuMultipleThresholdsImage& operator=(ITKOtsuMultipleThresholdsImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_NumberOfThresholds_Key = "NumberOfThresholds";
  static inline constexpr StringLiteral k_LabelOffset_Key = "LabelOffset";
  static inline constexpr StringLiteral k_NumberOfHistogramBins_Key = "NumberOfHistogramBins";
  static inline constexpr StringLiteral k_ValleyEmphasis_Key = "ValleyEmphasis";
  static inline constexpr StringLiteral k_ReturnBinMidpoint_Key = "ReturnBinMidpoint";

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
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ITKOtsuMultipleThresholdsImage, "6e66563a-edcf-5e11-bc1d-ceed36d8493f");
