#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKRescaleIntensityImage
 * @brief Applies a linear transformation to the intensity levels of the input Image .
 *
 * RescaleIntensityImageFilter applies pixel-wise a linear transformation to the intensity values of input image pixels. The linear transformation is defined by the user in terms of the minimum and
 * maximum values that the output image should have.
 *
 * The following equation gives the mapping of the intensity values
 *
 * \par
 *  \f[ outputPixel = ( inputPixel - inputMin) \cdot \frac{(outputMax - outputMin )}{(inputMax - inputMin)} + outputMin \f]
 *
 * All computations are performed in the precision of the input pixel's RealType. Before assigning the computed value to the output pixel.
 *
 * NOTE: In this filter the minimum and maximum values of the input image are computed internally using the MinimumMaximumImageCalculator . Users are not supposed to set those values in this filter.
 * If you need a filter where you can set the minimum and maximum values of the input, please use the IntensityWindowingImageFilter . If you want a filter that can use a user-defined linear
 * transformation for the intensity, then please use the ShiftScaleImageFilter .
 *
 * @see IntensityWindowingImageFilter
 *
 * ITK Module: ITKImageIntensity
 * ITK Group: ImageIntensity
 */
class ITKIMAGEPROCESSING_EXPORT ITKRescaleIntensityImage : public IFilter
{
public:
  ITKRescaleIntensityImage() = default;
  ~ITKRescaleIntensityImage() noexcept override = default;

  ITKRescaleIntensityImage(const ITKRescaleIntensityImage&) = delete;
  ITKRescaleIntensityImage(ITKRescaleIntensityImage&&) noexcept = delete;

  ITKRescaleIntensityImage& operator=(const ITKRescaleIntensityImage&) = delete;
  ITKRescaleIntensityImage& operator=(ITKRescaleIntensityImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_OutputMinimum_Key = "OutputMinimum";
  static inline constexpr StringLiteral k_OutputMaximum_Key = "OutputMaximum";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKRescaleIntensityImage, "77bf2192-851d-5127-9add-634c1ef4f67f");
