#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKDoubleThresholdImage
 * @brief Binarize an input image using double thresholding.
 *
 * Double threshold addresses the difficulty in selecting a threshold that will select the objects of interest without selecting extraneous objects. Double threshold considers two threshold ranges: a
 * narrow range and a wide range (where the wide range encompasses the narrow range). If the wide range was used for a traditional threshold (where values inside the range map to the foreground and
 * values outside the range map to the background), many extraneous pixels may survive the threshold operation. If the narrow range was used for a traditional threshold, then too few pixels may
 * survive the threshold.
 *
 * Double threshold uses the narrow threshold image as a marker image and the wide threshold image as a mask image in the geodesic dilation. Essentially, the marker image (narrow threshold) is dilated
 * but constrained to lie within the mask image (wide threshold). Thus, only the objects of interest (those pixels that survived the narrow threshold) are extracted but the those objects appear in the
 * final image as they would have if the wide threshold was used.
 *
 * @see GrayscaleGeodesicDilateImageFilter
 *
 *
 * @see MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter
 *
 * ITK Module: ITKMathematicalMorphology
 * ITK Group: MathematicalMorphology
 */
class ITKIMAGEPROCESSING_EXPORT ITKDoubleThresholdImage : public IFilter
{
public:
  ITKDoubleThresholdImage() = default;
  ~ITKDoubleThresholdImage() noexcept override = default;

  ITKDoubleThresholdImage(const ITKDoubleThresholdImage&) = delete;
  ITKDoubleThresholdImage(ITKDoubleThresholdImage&&) noexcept = delete;

  ITKDoubleThresholdImage& operator=(const ITKDoubleThresholdImage&) = delete;
  ITKDoubleThresholdImage& operator=(ITKDoubleThresholdImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "selected_image_geom_path";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_Threshold1_Key = "threshold1";
  static inline constexpr StringLiteral k_Threshold2_Key = "threshold2";
  static inline constexpr StringLiteral k_Threshold3_Key = "threshold3";
  static inline constexpr StringLiteral k_Threshold4_Key = "threshold4";
  static inline constexpr StringLiteral k_InsideValue_Key = "inside_value";
  static inline constexpr StringLiteral k_OutsideValue_Key = "outside_value";
  static inline constexpr StringLiteral k_FullyConnected_Key = "fully_connected";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKDoubleThresholdImage, "e268a65f-33f1-493f-a6c7-4635e57df3c4");
