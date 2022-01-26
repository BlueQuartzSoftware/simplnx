#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKPatchBasedDenoisingImage
 * @brief Derived class implementing a specific patch-based denoising algorithm, as detailed below.
 *
 * This class is derived from the base class PatchBasedDenoisingBaseImageFilter; please refer to the documentation of the base class first. This class implements a denoising filter that uses iterative
 * non-local, or semi-local, weighted averaging of image patches for image denoising. The intensity at each pixel 'p' gets updated as a weighted average of intensities of a chosen subset of pixels
 * from the image.
 *
 * This class implements the denoising algorithm using a Gaussian kernel function for nonparametric density estimation. The class implements a scheme to automatically estimated the kernel bandwidth
 * parameter (namely, sigma) using leave-one-out cross validation. It implements schemes for random sampling of patches non-locally (from the entire image) as well as semi-locally (from the spatial
 * proximity of the pixel being denoised at the specific point in time). It implements a specific scheme for defining patch weights (mask) as described in Awate and Whitaker 2005 IEEE CVPR and 2006
 * IEEE TPAMI.
 *
 * @see PatchBasedDenoisingBaseImageFilter
 *
 * ITK Module: ITKDenoising
 * ITK Group: Denoising
 */
class ITKIMAGEPROCESSING_EXPORT ITKPatchBasedDenoisingImage : public IFilter
{
public:
  ITKPatchBasedDenoisingImage() = default;
  ~ITKPatchBasedDenoisingImage() noexcept override = default;

  ITKPatchBasedDenoisingImage(const ITKPatchBasedDenoisingImage&) = delete;
  ITKPatchBasedDenoisingImage(ITKPatchBasedDenoisingImage&&) noexcept = delete;

  ITKPatchBasedDenoisingImage& operator=(const ITKPatchBasedDenoisingImage&) = delete;
  ITKPatchBasedDenoisingImage& operator=(ITKPatchBasedDenoisingImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_KernelBandwidthSigma_Key = "KernelBandwidthSigma";
  static inline constexpr StringLiteral k_PatchRadius_Key = "PatchRadius";
  static inline constexpr StringLiteral k_NumberOfIterations_Key = "NumberOfIterations";
  static inline constexpr StringLiteral k_NumberOfSamplePatches_Key = "NumberOfSamplePatches";
  static inline constexpr StringLiteral k_SampleVariance_Key = "SampleVariance";
  static inline constexpr StringLiteral k_NoiseModel_Key = "NoiseModel";
  static inline constexpr StringLiteral k_NoiseSigma_Key = "NoiseSigma";
  static inline constexpr StringLiteral k_NoiseModelFidelityWeight_Key = "NoiseModelFidelityWeight";
  static inline constexpr StringLiteral k_AlwaysTreatComponentsAsEuclidean_Key = "AlwaysTreatComponentsAsEuclidean";
  static inline constexpr StringLiteral k_KernelBandwidthEstimation_Key = "KernelBandwidthEstimation";
  static inline constexpr StringLiteral k_KernelBandwidthMultiplicationFactor_Key = "KernelBandwidthMultiplicationFactor";
  static inline constexpr StringLiteral k_KernelBandwidthUpdateFrequency_Key = "KernelBandwidthUpdateFrequency";
  static inline constexpr StringLiteral k_KernelBandwidthFractionPixelsForEstimation_Key = "KernelBandwidthFractionPixelsForEstimation";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKPatchBasedDenoisingImage, "ed61aebd-3a47-5ee1-8c9e-4ce205111b76");
