#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKBinaryErodeImage
 * @brief Fast binary erosion of a single intensity value in the image.
 *
 * BinaryErodeImageFilter is a binary erosion morphologic operation on the foreground of an image. Only the value designated by the intensity value "SetForegroundValue()" (alias as SetErodeValue() )
 * is considered as foreground, and other intensity values are considered background.
 *
 * Gray scale images can be processed as binary images by selecting a "ForegroundValue" (alias "ErodeValue"). Pixel values matching the erode value are considered the "foreground" and all other pixels
 * are "background". This is useful in processing segmented images where all pixels in segment #1 have value 1 and pixels in segment #2 have value 2, etc. A particular "segment number" can be
 * processed. ForegroundValue defaults to the maximum possible value of the PixelType. The eroded pixels will receive the BackgroundValue (defaults to NumericTraits::NonpositiveMin() ).
 *
 * The structuring element is assumed to be composed of binary values (zero or one). Only elements of the structuring element having values > 0 are candidates for affecting the center pixel. A
 * reasonable choice of structuring element is itk::BinaryBallStructuringElement .
 *
 * This implementation is based on the papers:
 *
 * L.Vincent "Morphological transformations of binary images with
 * arbitrary structuring elements", and
 *
 * N.Nikopoulos et al. "An efficient algorithm for 3d binary morphological transformations with 3d structuring elements for arbitrary size and shape". IEEE Transactions on Image Processing. Vol. 9.
 * No. 3. 2000. pp. 283-286.
 *
 * @see ImageToImageFilter BinaryDilateImageFilter BinaryMorphologyImageFilter
 *
 * ITK Module: ITKBinaryMathematicalMorphology
 * ITK Group: BinaryMathematicalMorphology
 */
class ITKIMAGEPROCESSING_EXPORT ITKBinaryErodeImage : public IFilter
{
public:
  ITKBinaryErodeImage() = default;
  ~ITKBinaryErodeImage() noexcept override = default;

  ITKBinaryErodeImage(const ITKBinaryErodeImage&) = delete;
  ITKBinaryErodeImage(ITKBinaryErodeImage&&) noexcept = delete;

  ITKBinaryErodeImage& operator=(const ITKBinaryErodeImage&) = delete;
  ITKBinaryErodeImage& operator=(ITKBinaryErodeImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "selected_image_geom_path";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "output_image_data_path";
  static inline constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static inline constexpr StringLiteral k_KernelType_Key = "kernel_type";
  static inline constexpr StringLiteral k_BackgroundValue_Key = "background_value";
  static inline constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";
  static inline constexpr StringLiteral k_BoundaryToForeground_Key = "boundary_to_foreground";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKBinaryErodeImage, "fcb447ba-c875-4c74-8341-89ba9657a772");
