#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKConnectedComponentImage
 * @brief Label the objects in a binary image.
 *
 * ConnectedComponentImageFilter labels the objects in a binary image (non-zero pixels are considered to be objects, zero-valued pixels are considered to be background). Each distinct object is
 * assigned a unique label. The filter experiments with some improvements to the existing implementation, and is based on run length encoding along raster lines. If the output background value is set
 * to zero (the default), the final object labels start with 1 and are consecutive. If the output background is set to a non-zero value (by calling the SetBackgroundValue() routine of the filter), the
 * final labels start at 0, and remain consecutive except for skipping the background value as needed. Objects that are reached earlier by a raster order scan have a lower label. This is different to
 * the behaviour of the original connected component image filter which did not produce consecutive labels or impose any particular ordering.
 *
 * After the filter is executed, ObjectCount holds the number of connected components.
 *
 * @see ImageToImageFilter
 *
 * ITK Module: ITKConnectedComponents
 * ITK Group: ConnectedComponents
 */
class ITKIMAGEPROCESSING_EXPORT ITKConnectedComponentImage : public IFilter
{
public:
  ITKConnectedComponentImage() = default;
  ~ITKConnectedComponentImage() noexcept override = default;

  ITKConnectedComponentImage(const ITKConnectedComponentImage&) = delete;
  ITKConnectedComponentImage(ITKConnectedComponentImage&&) noexcept = delete;

  ITKConnectedComponentImage& operator=(const ITKConnectedComponentImage&) = delete;
  ITKConnectedComponentImage& operator=(ITKConnectedComponentImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_FullyConnected_Key = "FullyConnected";
  static inline constexpr StringLiteral k_MaskImageDataPath_Key = "MaskImageDataPath";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKConnectedComponentImage, "bf554dd5-a927-5969-b651-1c47d386afce");
