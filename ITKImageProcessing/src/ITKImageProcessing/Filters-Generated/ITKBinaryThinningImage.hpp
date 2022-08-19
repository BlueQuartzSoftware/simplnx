#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKBinaryThinningImage
 * @brief This filter computes one-pixel-wide edges of the input image.
 *
 * This class is parameterized over the type of the input image and the type of the output image.
 *
 * The input is assumed to be a binary image. If the foreground pixels of the input image do not have a value of 1, they are rescaled to 1 internally to simplify the computation.
 *
 * The filter will produce a skeleton of the object. The output background values are 0, and the foreground values are 1.
 *
 * This filter is a sequential thinning algorithm and known to be computational time dependable on the image size. The algorithm corresponds with the 2D implementation described in:
 *
 * Rafael C. Gonzales and Richard E. Woods. Digital Image Processing. Addison Wesley, 491-494, (1993).
 *
 * To do: Make this filter ND.
 *
 * @see MorphologyImageFilter
 *
 * ITK Module: ITKBinaryMathematicalMorphology
 * ITK Group: BinaryMathematicalMorphology
 */
class ITKIMAGEPROCESSING_EXPORT ITKBinaryThinningImage : public IFilter
{
public:
  ITKBinaryThinningImage() = default;
  ~ITKBinaryThinningImage() noexcept override = default;

  ITKBinaryThinningImage(const ITKBinaryThinningImage&) = delete;
  ITKBinaryThinningImage(ITKBinaryThinningImage&&) noexcept = delete;

  ITKBinaryThinningImage& operator=(const ITKBinaryThinningImage&) = delete;
  ITKBinaryThinningImage& operator=(ITKBinaryThinningImage&&) noexcept = delete;

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKBinaryThinningImage, "8c9d384f-2900-45b8-85a4-4b2af64a79f4");
