#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKAdaptiveHistogramEqualizationImageFilter
 * @brief Power Law Adaptive Histogram Equalization.
 *
 * Histogram equalization modifies the contrast in an image. The AdaptiveHistogramEqualizationImageFilter is a superset of many contrast enhancing filters. By modifying its parameters (alpha, beta,
 * and window), the AdaptiveHistogramEqualizationImageFilter can produce an adaptively equalized histogram or a version of unsharp mask (local mean subtraction). Instead of applying a strict histogram
 * equalization in a window about a pixel, this filter prescribes a mapping function (power law) controlled by the parameters alpha and beta.
 *
 * The parameter alpha controls how much the filter acts like the classical histogram equalization method (alpha=0) to how much the filter acts like an unsharp mask (alpha=1).
 *
 * The parameter beta controls how much the filter acts like an unsharp mask (beta=0) to much the filter acts like pass through (beta=1, with alpha=1).
 *
 * The parameter window controls the size of the region over which local statistics are calculated. The size of the window is controlled by SetRadius the default Radius is 5 in all directions.
 *
 * By altering alpha, beta and window, a host of equalization and unsharp masking filters is available.
 *
 * The boundary condition ignores the part of the neighborhood outside the image, and over-weights the valid part of the neighborhood.
 *
 * For detail description, reference "Adaptive Image Contrast
 * Enhancement using Generalizations of Histogram Equalization." J.Alex Stark. IEEE Transactions on Image Processing, May 2000.
 *
 * ITK Module: ITKImageStatistics
 * ITK Group: ImageStatistics
 */
class ITKIMAGEPROCESSING_EXPORT ITKAdaptiveHistogramEqualizationImageFilter : public IFilter
{
public:
  ITKAdaptiveHistogramEqualizationImageFilter() = default;
  ~ITKAdaptiveHistogramEqualizationImageFilter() noexcept override = default;

  ITKAdaptiveHistogramEqualizationImageFilter(const ITKAdaptiveHistogramEqualizationImageFilter&) = delete;
  ITKAdaptiveHistogramEqualizationImageFilter(ITKAdaptiveHistogramEqualizationImageFilter&&) noexcept = delete;

  ITKAdaptiveHistogramEqualizationImageFilter& operator=(const ITKAdaptiveHistogramEqualizationImageFilter&) = delete;
  ITKAdaptiveHistogramEqualizationImageFilter& operator=(ITKAdaptiveHistogramEqualizationImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_Radius_Key = "radius";
  static inline constexpr StringLiteral k_Alpha_Key = "alpha";
  static inline constexpr StringLiteral k_Beta_Key = "beta";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKAdaptiveHistogramEqualizationImageFilter, "ea3e7439-8327-4190-8ff7-49ecc321718f");
