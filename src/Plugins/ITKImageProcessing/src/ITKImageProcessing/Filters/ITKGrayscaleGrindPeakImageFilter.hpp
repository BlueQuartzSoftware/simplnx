#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKGrayscaleGrindPeakImageFilter
 * @brief Remove local maxima not connected to the boundary of the image.
 *
 * GrayscaleGrindPeakImageFilter removes peaks in a grayscale image. Peaks are local maxima in the grayscale topography that are not connected to boundaries of the image. Gray level values adjacent to
 * a peak are extrapolated through the peak.
 *
 * This filter is used to smooth over local maxima without affecting the values of local minima. If you take the difference between the output of this filter and the original image (and perhaps
 * threshold the difference above a small value), you'll obtain a map of the local maxima.
 *
 * This filter uses the GrayscaleGeodesicDilateImageFilter . It provides its own input as the "mask" input to the geodesic erosion. The "marker" image for the geodesic erosion is constructed such that
 * boundary pixels match the boundary pixels of the input image and the interior pixels are set to the minimum pixel value in the input image.
 *
 * This filter is the dual to the GrayscaleFillholeImageFilter which implements the Fillhole algorithm. Since it is a dual, it is somewhat superfluous but is provided as a convenience.
 *
 * Geodesic morphology and the Fillhole algorithm is described in Chapter 6 of Pierre Soille's book "Morphological Image Analysis:
 * Principles and Applications", Second Edition, Springer, 2003.
 *
 * @see GrayscaleGeodesicDilateImageFilter
 *
 *
 * @see MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter
 *
 * ITK Module: ITKMathematicalMorphology
 * ITK Group: MathematicalMorphology
 */
class ITKIMAGEPROCESSING_EXPORT ITKGrayscaleGrindPeakImageFilter : public IFilter
{
public:
  ITKGrayscaleGrindPeakImageFilter() = default;
  ~ITKGrayscaleGrindPeakImageFilter() noexcept override = default;

  ITKGrayscaleGrindPeakImageFilter(const ITKGrayscaleGrindPeakImageFilter&) = delete;
  ITKGrayscaleGrindPeakImageFilter(ITKGrayscaleGrindPeakImageFilter&&) noexcept = delete;

  ITKGrayscaleGrindPeakImageFilter& operator=(const ITKGrayscaleGrindPeakImageFilter&) = delete;
  ITKGrayscaleGrindPeakImageFilter& operator=(ITKGrayscaleGrindPeakImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKGrayscaleGrindPeakImageFilter, "6aa5b193-c290-4fe3-a409-7759a62d48ea");
