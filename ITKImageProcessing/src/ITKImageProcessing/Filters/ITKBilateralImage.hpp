#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKBilateralImage
 * @brief Blurs an image while preserving edges.
 *
 * This filter uses bilateral filtering to blur an image using both domain and range "neighborhoods". Pixels that are close to a pixel in the image domain and similar to a pixel in the image range are
 * used to calculate the filtered value. Two gaussian kernels (one in the image domain and one in the image range) are used to smooth the image. The result is an image that is smoothed in homogeneous
 * regions yet has edges preserved. The result is similar to anisotropic diffusion but the implementation in non-iterative. Another benefit to bilateral filtering is that any distance metric can be
 * used for kernel smoothing the image range. Hence, color images can be smoothed as vector images, using the CIE distances between intensity values as the similarity metric (the Gaussian kernel for
 * the image domain is evaluated using CIE distances). A separate version of this filter will be designed for color and vector images.
 *
 * Bilateral filtering is capable of reducing the noise in an image by an order of magnitude while maintaining edges.
 *
 * The bilateral operator used here was described by Tomasi and Manduchi (Bilateral Filtering for Gray and ColorImages. IEEE ICCV. 1998.)
 *
 * @see GaussianOperator
 *
 *
 * @see RecursiveGaussianImageFilter
 *
 *
 * @see DiscreteGaussianImageFilter
 *
 *
 * @see AnisotropicDiffusionImageFilter
 *
 *
 * @see Image
 *
 *
 * @see Neighborhood
 *
 *
 * @see NeighborhoodOperator
 *
 *
 * TodoSupport color images
 *
 * Support vector images
 *
 * ITK Module: ITKImageFeature
 * ITK Group: ImageFeature
 */
class ITKIMAGEPROCESSING_EXPORT ITKBilateralImage : public IFilter
{
public:
  ITKBilateralImage() = default;
  ~ITKBilateralImage() noexcept override = default;

  ITKBilateralImage(const ITKBilateralImage&) = delete;
  ITKBilateralImage(ITKBilateralImage&&) noexcept = delete;

  ITKBilateralImage& operator=(const ITKBilateralImage&) = delete;
  ITKBilateralImage& operator=(ITKBilateralImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_DomainSigma_Key = "DomainSigma";
  static inline constexpr StringLiteral k_RangeSigma_Key = "RangeSigma";
  static inline constexpr StringLiteral k_NumberOfRangeGaussianSamples_Key = "NumberOfRangeGaussianSamples";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKBilateralImage, "18ab754c-3219-59c8-928e-5fb4a09174e0");
