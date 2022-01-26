#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKHistogramMatchingImage
 * @brief Normalize the grayscale values for a source image by matching the shape of the source image histogram to a reference histogram.
 *
 * HistogramMatchingImageFilter normalizes the grayscale values of a source image based on the grayscale values of either a reference image or a reference histogram. This filter uses a histogram
 * matching technique where the histograms of the are matched only at a specified number of quantile values.
 *
 * This filter was originally designed to normalize MR images of the same MR protocol and same body part. The algorithm works best if background pixels are excluded from both the source and reference
 * histograms. A simple background exclusion method is to exclude all pixels whose grayscale values are smaller than the mean grayscale value. ThresholdAtMeanIntensityOn() switches on this simple
 * background exclusion method. With ThresholdAtMeanIntensityOn() , The reference histogram returned from this filter will expand the first and last bin bounds to include the minimum and maximum
 * intensity values of the entire reference image, but only intensity values greater than the mean will be used to populate the histogram.
 *
 * The source image can be set via either SetInput() or SetSourceImage() . The reference object used is selected with can be set via SetReferenceImage() or SetReferenceHistogram() .
 *
 * SetNumberOfHistogramLevels() sets the number of bins used when creating histograms of the source and reference images. SetNumberOfMatchPoints() governs the number of quantile values to be matched.
 *
 * This filter assumes that both the source and reference are of the same type and that the input and output image type have the same number of dimension and have scalar pixel types.
 *
 * \par REFERENCE
 * Laszlo G. Nyul, Jayaram K. Udupa, and Xuan Zhang, "New Variants of a Method
 * of MRI Scale Standardization", IEEE Transactions on Medical Imaging, 19(2):143-150, 2000.
 *
 * ITK Module: ITKImageIntensity
 * ITK Group: ImageIntensity
 */
class ITKIMAGEPROCESSING_EXPORT ITKHistogramMatchingImage : public IFilter
{
public:
  ITKHistogramMatchingImage() = default;
  ~ITKHistogramMatchingImage() noexcept override = default;

  ITKHistogramMatchingImage(const ITKHistogramMatchingImage&) = delete;
  ITKHistogramMatchingImage(ITKHistogramMatchingImage&&) noexcept = delete;

  ITKHistogramMatchingImage& operator=(const ITKHistogramMatchingImage&) = delete;
  ITKHistogramMatchingImage& operator=(ITKHistogramMatchingImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_NumberOfHistogramLevels_Key = "NumberOfHistogramLevels";
  static inline constexpr StringLiteral k_NumberOfMatchPoints_Key = "NumberOfMatchPoints";
  static inline constexpr StringLiteral k_ThresholdAtMeanIntensity_Key = "ThresholdAtMeanIntensity";
  static inline constexpr StringLiteral k_ReferenceImageDataPath_Key = "ReferenceImageDataPath";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKHistogramMatchingImage, "33ca886c-42b9-524a-984a-dab24f8bb74c");
