#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKHMaximaImage
 * @brief Suppress local maxima whose height above the baseline is less than h.
 *
 * HMaximaImageFilter suppresses local maxima that are less than h intensity units above the (local) background. This has the effect of smoothing over the "high" parts of the noise in the image
 * without smoothing over large changes in intensity (region boundaries). See the HMinimaImageFilter to suppress the local minima whose depth is less than h intensity units below the (local)
 * background.
 *
 * If the output of HMaximaImageFilter is subtracted from the original image, the significant "peaks" in the image can be identified. This is what the HConvexImageFilter provides.
 *
 * This filter uses the ReconstructionByDilationImageFilter . It provides its own input as the "mask" input to the geodesic dilation. The "marker" image for the geodesic dilation is the input image
 * minus the height parameter h.
 *
 * Geodesic morphology and the H-Maxima algorithm is described in Chapter 6 of Pierre Soille's book "Morphological Image Analysis:
 * Principles and Applications", Second Edition, Springer, 2003.
 *
 * The height parameter is set using SetHeight.
 *
 * @see ReconstructionByDilationImageFilter , HMinimaImageFilter , HConvexImageFilter
 *
 *
 * @see MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter
 *
 * ITK Module: ITKMathematicalMorphology
 * ITK Group: MathematicalMorphology
 */
class ITKIMAGEPROCESSING_EXPORT ITKHMaximaImage : public IFilter
{
public:
  ITKHMaximaImage() = default;
  ~ITKHMaximaImage() noexcept override = default;

  ITKHMaximaImage(const ITKHMaximaImage&) = delete;
  ITKHMaximaImage(ITKHMaximaImage&&) noexcept = delete;

  ITKHMaximaImage& operator=(const ITKHMaximaImage&) = delete;
  ITKHMaximaImage& operator=(ITKHMaximaImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_Height_Key = "Height";

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
   * @param shouldCancel Boolean that gets set if the filter should stop executing and return
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Boolean that gets set if the filter should stop executing and return
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ITKHMaximaImage, "fc8c9fb6-b297-4b30-907f-14b5d5c2b336");
