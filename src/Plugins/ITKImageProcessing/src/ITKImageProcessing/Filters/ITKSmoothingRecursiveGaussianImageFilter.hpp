#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKSmoothingRecursiveGaussianImageFilter
 * @brief Computes the smoothing of an image by convolution with the Gaussian kernels implemented as IIR filters.
 *
 * This filter is implemented using the recursive gaussian filters. For multi-component images, the filter works on each component independently.
 *
 * For this filter to be able to run in-place the input and output image types need to be the same and/or the same type as the RealImageType.
 *
 * ITK Module: ITKSmoothing
 * ITK Group: Smoothing
 */
class ITKIMAGEPROCESSING_EXPORT ITKSmoothingRecursiveGaussianImageFilter : public IFilter
{
public:
  ITKSmoothingRecursiveGaussianImageFilter() = default;
  ~ITKSmoothingRecursiveGaussianImageFilter() noexcept override = default;

  ITKSmoothingRecursiveGaussianImageFilter(const ITKSmoothingRecursiveGaussianImageFilter&) = delete;
  ITKSmoothingRecursiveGaussianImageFilter(ITKSmoothingRecursiveGaussianImageFilter&&) noexcept = delete;

  ITKSmoothingRecursiveGaussianImageFilter& operator=(const ITKSmoothingRecursiveGaussianImageFilter&) = delete;
  ITKSmoothingRecursiveGaussianImageFilter& operator=(ITKSmoothingRecursiveGaussianImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_Sigma_Key = "sigma";
  static inline constexpr StringLiteral k_NormalizeAcrossScale_Key = "normalize_across_scale";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKSmoothingRecursiveGaussianImageFilter, "74859077-69ba-40ad-965f-8699dde1c22d");
