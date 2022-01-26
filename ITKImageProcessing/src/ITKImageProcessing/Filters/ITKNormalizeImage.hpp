#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKNormalizeImage
 * @brief Normalize an image by setting its mean to zero and variance to one.
 *
 * NormalizeImageFilter shifts and scales an image so that the pixels in the image have a zero mean and unit variance. This filter uses StatisticsImageFilter to compute the mean and variance of the
 * input and then applies ShiftScaleImageFilter to shift and scale the pixels.
 *
 * NB: since this filter normalizes the data such that the mean is at 0, and \f$-\sigma\f$ to \f$+\sigma\f$ is mapped to -1.0 to 1.0, output image integral types will produce an image that DOES NOT
 * HAVE a unit variance due to 68% of the intensity values being mapped to the real number range of -1.0 to 1.0 and then cast to the output integral value.
 *
 * @see NormalizeToConstantImageFilter
 *
 * ITK Module: ITKImageIntensity
 * ITK Group: ImageIntensity
 */
class ITKIMAGEPROCESSING_EXPORT ITKNormalizeImage : public IFilter
{
public:
  ITKNormalizeImage() = default;
  ~ITKNormalizeImage() noexcept override = default;

  ITKNormalizeImage(const ITKNormalizeImage&) = delete;
  ITKNormalizeImage(ITKNormalizeImage&&) noexcept = delete;

  ITKNormalizeImage& operator=(const ITKNormalizeImage&) = delete;
  ITKNormalizeImage& operator=(ITKNormalizeImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKNormalizeImage, "5b905619-c46b-5690-b6fa-8e97cf4537b8");
