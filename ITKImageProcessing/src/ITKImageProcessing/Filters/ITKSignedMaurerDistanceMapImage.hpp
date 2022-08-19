#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKSignedMaurerDistanceMapImage
 * @brief This filter calculates the Euclidean distance transform of a binary image in linear time for arbitrary dimensions.
 *
 * \par Inputs and Outputs
 * This is an image-to-image filter. The dimensionality is arbitrary. The only dimensionality constraint is that the input and output images be of the same dimensions and size. To maintain integer
 * arithmetic within the filter, the default output is the signed squared distance. This implies that the input image should be of type "unsigned int" or "int" whereas the output image is of type
 * "int". Obviously, if the user wishes to utilize the image spacing or to have a filter with the Euclidean distance (as opposed to the squared distance), output image types of float or double should
 * be used.
 *
 *
 * The inside is considered as having negative distances. Outside is treated as having positive distances. To change the convention, use the InsideIsPositive(bool) function.
 *
 * \par Parameters
 * Set/GetBackgroundValue specifies the background of the value of the input binary image. Normally this is zero and, as such, zero is the default value. Other than that, the usage is completely
 * analogous to the itk::DanielssonDistanceImageFilter class except it does not return the Voronoi map.
 *
 *
 * Reference: C. R. Maurer, Jr., R. Qi, and V. Raghavan, "A Linear Time Algorithm for Computing Exact Euclidean Distance Transforms of Binary Images in Arbitrary Dimensions", IEEE - Transactions on
 * Pattern Analysis and Machine Intelligence, 25(2): 265-270, 2003.
 *
 * ITK Module: ITKDistanceMap
 * ITK Group: DistanceMap
 */
class ITKIMAGEPROCESSING_EXPORT ITKSignedMaurerDistanceMapImage : public IFilter
{
public:
  ITKSignedMaurerDistanceMapImage() = default;
  ~ITKSignedMaurerDistanceMapImage() noexcept override = default;

  ITKSignedMaurerDistanceMapImage(const ITKSignedMaurerDistanceMapImage&) = delete;
  ITKSignedMaurerDistanceMapImage(ITKSignedMaurerDistanceMapImage&&) noexcept = delete;

  ITKSignedMaurerDistanceMapImage& operator=(const ITKSignedMaurerDistanceMapImage&) = delete;
  ITKSignedMaurerDistanceMapImage& operator=(ITKSignedMaurerDistanceMapImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_InsideIsPositive_Key = "InsideIsPositive";
  static inline constexpr StringLiteral k_SquaredDistance_Key = "SquaredDistance";
  static inline constexpr StringLiteral k_UseImageSpacing_Key = "UseImageSpacing";
  static inline constexpr StringLiteral k_BackgroundValue_Key = "BackgroundValue";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKSignedMaurerDistanceMapImage, "e81f72d3-e806-4afe-ab4c-795c6a3f526f");
