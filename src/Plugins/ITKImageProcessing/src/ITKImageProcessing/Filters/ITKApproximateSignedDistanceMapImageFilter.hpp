#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKApproximateSignedDistanceMapImageFilter
 * @brief Create a map of the approximate signed distance from the boundaries of a binary image.
 *
 * The ApproximateSignedDistanceMapImageFilter takes as input a binary image and produces a signed distance map. Each pixel value in the output contains the approximate distance from that pixel to the
 * nearest "object" in the binary image. This filter differs from the DanielssonDistanceMapImageFilter in that it calculates the distance to the "object edge" for pixels within the object.
 *
 * Negative values in the output indicate that the pixel at that position is within an object in the input image. The absolute value of a negative pixel represents the approximate distance to the
 * nearest object boundary pixel.
 *
 * WARNING: This filter requires that the output type be floating-point. Otherwise internal calculations will not be performed to the appropriate precision, resulting in completely incorrect (read:
 * zero-valued) output.
 *
 * The distances computed by this filter are Chamfer distances, which are only an approximation to Euclidean distances, and are not as exact approximations as those calculated by the
 * DanielssonDistanceMapImageFilter . On the other hand, this filter is faster.
 *
 * This filter requires that an "inside value" and "outside value" be set as parameters. The "inside value" is the intensity value of the binary image which corresponds to objects, and the "outside
 * value" is the intensity of the background. (A typical binary image often represents objects as black (0) and background as white (usually 255), or vice-versa.) Note that this filter is slightly
 * faster if the inside value is less than the outside value. Otherwise an extra iteration through the image is required.
 *
 * This filter uses the FastChamferDistanceImageFilter and the IsoContourDistanceImageFilter internally to perform the distance calculations.
 *
 * @see DanielssonDistanceMapImageFilter
 *
 *
 * @see SignedDanielssonDistanceMapImageFilter
 *
 *
 * @see SignedMaurerDistanceMapImageFilter
 *
 *
 * @see FastChamferDistanceImageFilter
 *
 *
 * @see IsoContourDistanceImageFilter
 *
 *
 * @author Zach Pincus
 *
 * ITK Module: ITKDistanceMap
 * ITK Group: DistanceMap
 */
class ITKIMAGEPROCESSING_EXPORT ITKApproximateSignedDistanceMapImageFilter : public IFilter
{
public:
  ITKApproximateSignedDistanceMapImageFilter() = default;
  ~ITKApproximateSignedDistanceMapImageFilter() noexcept override = default;

  ITKApproximateSignedDistanceMapImageFilter(const ITKApproximateSignedDistanceMapImageFilter&) = delete;
  ITKApproximateSignedDistanceMapImageFilter(ITKApproximateSignedDistanceMapImageFilter&&) noexcept = delete;

  ITKApproximateSignedDistanceMapImageFilter& operator=(const ITKApproximateSignedDistanceMapImageFilter&) = delete;
  ITKApproximateSignedDistanceMapImageFilter& operator=(ITKApproximateSignedDistanceMapImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_InsideValue_Key = "inside_value";
  static inline constexpr StringLiteral k_OutsideValue_Key = "outside_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKApproximateSignedDistanceMapImageFilter, "87ed0d3a-c394-4bb5-ac7f-6cc746984b09");
