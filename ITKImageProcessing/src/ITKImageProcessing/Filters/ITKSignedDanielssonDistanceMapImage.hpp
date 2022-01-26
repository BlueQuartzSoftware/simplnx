#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKSignedDanielssonDistanceMapImage
 * @brief
 *
 * This class is parameterized over the type of the input image and the type of the output image.
 *
 * This filter computes the distance map of the input image as an approximation with pixel accuracy to the Euclidean distance.
 *
 * For purposes of evaluating the signed distance map, the input is assumed to be binary composed of pixels with value 0 and non-zero.
 *
 * The inside is considered as having negative distances. Outside is treated as having positive distances. To change the convention, use the InsideIsPositive(bool) function.
 *
 * As a convention, the distance is evaluated from the boundary of the ON pixels.
 *
 * The filter returns
 *
 * @li A signed distance map with the approximation to the euclidean distance.
 *
 * @li A voronoi partition. (See itkDanielssonDistanceMapImageFilter)
 *
 * @li A vector map containing the component of the vector relating the current pixel with the closest point of the closest object to this pixel. Given that the components of the distance are computed
 * in "pixels", the vector is represented by an itk::Offset . That is, physical coordinates are not used. (See itkDanielssonDistanceMapImageFilter)
 *
 *
 *
 *
 * This filter internally uses the DanielssonDistanceMap filter. This filter is N-dimensional.
 *
 * @see itkDanielssonDistanceMapImageFilter
 *
 * ITK Module: ITKDistanceMap
 * ITK Group: DistanceMap
 */
class ITKIMAGEPROCESSING_EXPORT ITKSignedDanielssonDistanceMapImage : public IFilter
{
public:
  ITKSignedDanielssonDistanceMapImage() = default;
  ~ITKSignedDanielssonDistanceMapImage() noexcept override = default;

  ITKSignedDanielssonDistanceMapImage(const ITKSignedDanielssonDistanceMapImage&) = delete;
  ITKSignedDanielssonDistanceMapImage(ITKSignedDanielssonDistanceMapImage&&) noexcept = delete;

  ITKSignedDanielssonDistanceMapImage& operator=(const ITKSignedDanielssonDistanceMapImage&) = delete;
  ITKSignedDanielssonDistanceMapImage& operator=(ITKSignedDanielssonDistanceMapImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_InsideIsPositive_Key = "InsideIsPositive";
  static inline constexpr StringLiteral k_SquaredDistance_Key = "SquaredDistance";
  static inline constexpr StringLiteral k_UseImageSpacing_Key = "UseImageSpacing";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKSignedDanielssonDistanceMapImage, "fc192fa8-b6b7-539c-9763-f683724da626");
