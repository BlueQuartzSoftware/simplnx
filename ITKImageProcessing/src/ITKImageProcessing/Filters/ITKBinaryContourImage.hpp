#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKBinaryContourImage
 * @brief Labels the pixels on the border of the objects in a binary image.
 *
 * BinaryContourImageFilter takes a binary image as input, where the pixels in the objects are the pixels with a value equal to ForegroundValue. Only the pixels on the contours of the objects are
 * kept. The pixels not on the border are changed to BackgroundValue.
 *
 * The connectivity can be changed to minimum or maximum connectivity with SetFullyConnected() . Full connectivity produces thicker contours.
 *
 * https://hdl.handle.net/1926/1352
 *
 * @author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.
 *
 *
 * @see LabelContourImageFilter BinaryErodeImageFilter SimpleContourExtractorImageFilter
 *
 * ITK Module: ITKImageLabel
 * ITK Group: ImageLabel
 */
class ITKIMAGEPROCESSING_EXPORT ITKBinaryContourImage : public IFilter
{
public:
  ITKBinaryContourImage() = default;
  ~ITKBinaryContourImage() noexcept override = default;

  ITKBinaryContourImage(const ITKBinaryContourImage&) = delete;
  ITKBinaryContourImage(ITKBinaryContourImage&&) noexcept = delete;

  ITKBinaryContourImage& operator=(const ITKBinaryContourImage&) = delete;
  ITKBinaryContourImage& operator=(ITKBinaryContourImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "selected_image_geom_path";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "output_image_data_path";
  static inline constexpr StringLiteral k_FullyConnected_Key = "fully_connected";
  static inline constexpr StringLiteral k_BackgroundValue_Key = "background_value";
  static inline constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKBinaryContourImage, "ed214e76-6954-49b4-817b-13f92315e722");
