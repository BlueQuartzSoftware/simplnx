#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKFFTNormalizedCorrelationImage
 * @brief Calculate normalized cross correlation using FFTs.
 *
 * This filter calculates the normalized cross correlation (NCC) of two images using FFTs instead of spatial correlation. It is much faster than spatial correlation for reasonably large structuring
 * elements. This filter is a subclass of the more general MaskedFFTNormalizedCorrelationImageFilter and operates by essentially setting the masks in that algorithm to images of ones. As described in
 * detail in the references below, there is no computational overhead to utilizing the more general masked algorithm because the FFTs of the images of ones are still necessary for the computations.
 *
 * Inputs: Two images are required as inputs, fixedImage and movingImage. In the context of correlation, inputs are often defined as: "image" and "template". In this filter, the fixedImage plays the
 * role of the image, and the movingImage plays the role of the template. However, this filter is capable of correlating any two images and is not restricted to small movingImages (templates).
 *
 * Optional parameters: The RequiredNumberOfOverlappingPixels enables the user to specify how many voxels of the two images must overlap; any location in the correlation map that results from fewer
 * than this number of voxels will be set to zero. Larger values zero-out pixels on a larger border around the correlation image. Thus, larger values remove less stable computations but also limit the
 * capture range. If RequiredNumberOfOverlappingPixels is set to 0, the default, no zeroing will take place.
 *
 * Image size: fixedImage and movingImage need not be the same size. Furthermore, whereas some algorithms require that the "template" be smaller than the "image" because of errors in the regions where
 * the two are not fully overlapping, this filter has no such restriction.
 *
 * Image spacing: Since the computations are done in the pixel domain, all input images must have the same spacing.
 *
 * Outputs; The output is an image of RealPixelType that is the NCC of the two images and its values range from -1.0 to 1.0. The size of this NCC image is, by definition, size(fixedImage) +
 * size(movingImage) - 1.
 *
 * Example filter usage: \code
 * using FilterType = itk::FFTNormalizedCorrelationImageFilter< ShortImageType, DoubleImageType >;
 *
 * FilterType::Pointer filter = FilterType::New();
 *
 * filter->SetFixedImage( fixedImage );
 *
 * filter->SetMovingImage( movingImage );
 *
 * filter->SetRequiredNumberOfOverlappingPixels(20);
 *
 * filter->Update();
 *
 * \endcode
 *
 *
 * \warning The pixel type of the output image must be of real type (float or double). ConceptChecking is used to enforce the output pixel type. You will get a compilation error if the pixel type of
 * the output image is not float or double.
 *
 *
 * References: 1) D. Padfield. "Masked object registration in the Fourier domain." Transactions on Image Processing. 2) D. Padfield. "Masked FFT registration". In Proc. Computer Vision and Pattern
 * Recognition, 2010.
 *
 * @author : Dirk Padfield, GE Global Research, padfield@research.ge.com
 *
 * ITK Module: ITKConvolution
 * ITK Group: Convolution
 */
class ITKIMAGEPROCESSING_EXPORT ITKFFTNormalizedCorrelationImage : public IFilter
{
public:
  ITKFFTNormalizedCorrelationImage() = default;
  ~ITKFFTNormalizedCorrelationImage() noexcept override = default;

  ITKFFTNormalizedCorrelationImage(const ITKFFTNormalizedCorrelationImage&) = delete;
  ITKFFTNormalizedCorrelationImage(ITKFFTNormalizedCorrelationImage&&) noexcept = delete;

  ITKFFTNormalizedCorrelationImage& operator=(const ITKFFTNormalizedCorrelationImage&) = delete;
  ITKFFTNormalizedCorrelationImage& operator=(ITKFFTNormalizedCorrelationImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_RequiredNumberOfOverlappingPixels_Key = "RequiredNumberOfOverlappingPixels";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKFFTNormalizedCorrelationImage, "a0d962b7-9d5c-5abc-a078-1fe795df4663");
