#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKErodeObjectMorphologyImageFilter
 * @brief Erosion of an object in an image.
 *
 * Erosion of an image using binary morphology. Pixel values matching the object value are considered the "object" and all other pixels are "background". This is useful in processing mask images
 * containing only one object.
 *
 * If the pixel covered by the center of the kernel has the pixel value ObjectValue and the pixel is adjacent to a non-object valued pixel, then the kernel is centered on the object-value pixel and
 * neighboring pixels covered by the kernel are assigned the background value. The structuring element is assumed to be composed of binary values (zero or one).
 *
 * @see ObjectMorphologyImageFilter , BinaryFunctionErodeImageFilter
 *
 *
 * @see BinaryErodeImageFilter
 *
 * ITK Module: ITKBinaryMathematicalMorphology
 * ITK Group: BinaryMathematicalMorphology
 */
class ITKIMAGEPROCESSING_EXPORT ITKErodeObjectMorphologyImageFilter : public IFilter
{
public:
  ITKErodeObjectMorphologyImageFilter() = default;
  ~ITKErodeObjectMorphologyImageFilter() noexcept override = default;

  ITKErodeObjectMorphologyImageFilter(const ITKErodeObjectMorphologyImageFilter&) = delete;
  ITKErodeObjectMorphologyImageFilter(ITKErodeObjectMorphologyImageFilter&&) noexcept = delete;

  ITKErodeObjectMorphologyImageFilter& operator=(const ITKErodeObjectMorphologyImageFilter&) = delete;
  ITKErodeObjectMorphologyImageFilter& operator=(ITKErodeObjectMorphologyImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static inline constexpr StringLiteral k_KernelType_Key = "kernel_type_index";
  static inline constexpr StringLiteral k_ObjectValue_Key = "object_value";
  static inline constexpr StringLiteral k_BackgroundValue_Key = "background_value";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKErodeObjectMorphologyImageFilter, "1e047c6e-da5b-4a11-8800-adb8a191feac");
