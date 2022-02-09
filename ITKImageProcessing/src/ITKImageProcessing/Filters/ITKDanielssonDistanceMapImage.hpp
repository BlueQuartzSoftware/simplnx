#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKDanielssonDistanceMapImage
 * @brief This filter computes the distance map of the input image as an approximation with pixel accuracy to the Euclidean distance.
 *
 * TInputImage
 *
 * Input Image Type
 *
 *
 *
 *
 * TOutputImage
 *
 * Output Image Type
 *
 *
 *
 *
 * TVoronoiImage
 *
 * Voronoi Image Type. Note the default value is TInputImage.
 *
 *
 *
 * The input is assumed to contain numeric codes defining objects. The filter will produce as output the following images:
 *
 *
 *
 * @li A Voronoi partition using the same numeric codes as the input.
 *
 *
 * @li A distance map with the approximation to the euclidean distance. from a particular pixel to the nearest object to this pixel in the input image.
 *
 *
 * @li A vector map containing the component of the vector relating the current pixel with the closest point of the closest object to this pixel. Given that the components of the distance are computed
 * in "pixels", the vector is represented by an itk::Offset . That is, physical coordinates are not used.
 *
 *
 *
 * This filter is N-dimensional and known to be efficient in computational time. The algorithm is the N-dimensional version of the 4SED algorithm given for two dimensions in:
 *
 * Danielsson, Per-Erik. Euclidean Distance Mapping. Computer Graphics and Image Processing 14, 227-248 (1980).
 *
 * ITK Module: ITKDistanceMap
 * ITK Group: DistanceMap
 */
class ITKIMAGEPROCESSING_EXPORT ITKDanielssonDistanceMapImage : public IFilter
{
public:
  ITKDanielssonDistanceMapImage() = default;
  ~ITKDanielssonDistanceMapImage() noexcept override = default;

  ITKDanielssonDistanceMapImage(const ITKDanielssonDistanceMapImage&) = delete;
  ITKDanielssonDistanceMapImage(ITKDanielssonDistanceMapImage&&) noexcept = delete;

  ITKDanielssonDistanceMapImage& operator=(const ITKDanielssonDistanceMapImage&) = delete;
  ITKDanielssonDistanceMapImage& operator=(ITKDanielssonDistanceMapImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_InputIsBinary_Key = "InputIsBinary";
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
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ITKDanielssonDistanceMapImage, "53d5b289-a716-559b-89d9-5ebb34f714ca");
