#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKSignedDanielssonDistanceMapImage
 * @brief This filter computes the signed distance map of the input image as an approximation with pixel accuracy to the Euclidean distance.
 *
 * This class is parameterized over the type of the input image and the type of the output image.
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
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "selected_image_geom_path";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "output_image_data_path";
  static inline constexpr StringLiteral k_InsideIsPositive_Key = "inside_is_positive";
  static inline constexpr StringLiteral k_SquaredDistance_Key = "squared_distance";
  static inline constexpr StringLiteral k_UseImageSpacing_Key = "use_image_spacing";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKSignedDanielssonDistanceMapImage, "2f42e771-1d84-4468-8991-9a1fad7eb740");
