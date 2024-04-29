#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKHMinimaImageFilter
 * @brief Suppress local minima whose depth below the baseline is less than h.
 *
 * HMinimaImageFilter suppresses local minima that are less than h intensity units below the (local) background. This has the effect of smoothing over the "low" parts of the noise in the image without
 * smoothing over large changes in intensity (region boundaries). See the HMaximaImageFilter to suppress the local maxima whose height is less than h intensity units above the (local) background.
 *
 * If original image is subtracted from the output of HMinimaImageFilter , the significant "valleys" in the image can be identified. This is what the HConcaveImageFilter provides.
 *
 * This filter uses the GrayscaleGeodesicErodeImageFilter . It provides its own input as the "mask" input to the geodesic dilation. The "marker" image for the geodesic dilation is the input image plus
 * the height parameter h.
 *
 * Geodesic morphology and the H-Minima algorithm is described in Chapter 6 of Pierre Soille's book "Morphological Image Analysis:
 * Principles and Applications", Second Edition, Springer, 2003.
 *
 * @see GrayscaleGeodesicDilateImageFilter , HMinimaImageFilter , HConvexImageFilter
 *
 *
 * @see MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter
 *
 * ITK Module: ITKMathematicalMorphology
 * ITK Group: MathematicalMorphology
 */
class ITKIMAGEPROCESSING_EXPORT ITKHMinimaImageFilter : public IFilter
{
public:
  ITKHMinimaImageFilter() = default;
  ~ITKHMinimaImageFilter() noexcept override = default;

  ITKHMinimaImageFilter(const ITKHMinimaImageFilter&) = delete;
  ITKHMinimaImageFilter(ITKHMinimaImageFilter&&) noexcept = delete;

  ITKHMinimaImageFilter& operator=(const ITKHMinimaImageFilter&) = delete;
  ITKHMinimaImageFilter& operator=(ITKHMinimaImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_Height_Key = "height";
  static inline constexpr StringLiteral k_FullyConnected_Key = "fully_connected";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKHMinimaImageFilter, "3ce4692a-8009-4a14-93ed-32b4f47d8c6f");
