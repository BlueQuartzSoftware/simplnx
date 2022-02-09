#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKCosImage
 * @brief Computes the cosine of each pixel.
 *
 * This filter is templated over the pixel type of the input image and the pixel type of the output image.
 *
 * The filter walks over all of the pixels in the input image, and for each pixel does the following:
 *
 *
 *
 * @li cast the pixel value to double ,
 *
 *
 * @li apply the std::cos() function to the double value,
 *
 *
 * @li cast the double value resulting from std::cos() to the pixel type of the output image,
 *
 *
 * @li store the cast value into the output image.
 *
 *
 *
 * The filter expects both images to have the same dimension (e.g. both 2D, or both 3D, or both ND)
 *
 * ITK Module: ITKImageIntensity
 * ITK Group: ImageIntensity
 */
class ITKIMAGEPROCESSING_EXPORT ITKCosImage : public IFilter
{
public:
  ITKCosImage() = default;
  ~ITKCosImage() noexcept override = default;

  ITKCosImage(const ITKCosImage&) = delete;
  ITKCosImage(ITKCosImage&&) noexcept = delete;

  ITKCosImage& operator=(const ITKCosImage&) = delete;
  ITKCosImage& operator=(ITKCosImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKCosImage, "2c2d7bf6-1e78-52e6-80aa-58b504ce0912");
