#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKShiftScaleImage
 * @brief Shift and scale the pixels in an image.
 *
 * ShiftScaleImageFilter shifts the input pixel by Shift (default 0.0) and then scales the pixel by Scale (default 1.0). All computations are performed in the precision of the input pixel's RealType.
 * Before assigning the computed value to the output pixel, the value is clamped at the NonpositiveMin and max of the pixel type.
 *
 * ITK Module: ITKImageIntensity
 * ITK Group: ImageIntensity
 */
class ITKIMAGEPROCESSING_EXPORT ITKShiftScaleImage : public IFilter
{
public:
  ITKShiftScaleImage() = default;
  ~ITKShiftScaleImage() noexcept override = default;

  ITKShiftScaleImage(const ITKShiftScaleImage&) = delete;
  ITKShiftScaleImage(ITKShiftScaleImage&&) noexcept = delete;

  ITKShiftScaleImage& operator=(const ITKShiftScaleImage&) = delete;
  ITKShiftScaleImage& operator=(ITKShiftScaleImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_Shift_Key = "Shift";
  static inline constexpr StringLiteral k_Scale_Key = "Scale";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKShiftScaleImage, "31d325fa-e605-5415-85ab-ab93e8cbf32f");
