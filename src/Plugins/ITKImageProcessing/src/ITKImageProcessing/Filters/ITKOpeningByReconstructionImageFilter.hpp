#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKOpeningByReconstructionImageFilter
 * @brief Opening by reconstruction of an image.
 *
 * This filter preserves regions, in the foreground, that can completely contain the structuring element. At the same time, this filter eliminates all other regions of foreground pixels. Contrary to
 * the morphological opening, the opening by reconstruction preserves the shape of the components that are not removed by erosion. The opening by reconstruction of an image "f" is defined as:
 *
 * OpeningByReconstruction(f) = DilationByReconstruction(f, Erosion(f)).
 *
 * Opening by reconstruction not only removes structures destroyed by the erosion, but also levels down the contrast of the brightest regions. If PreserveIntensities is on, a subsequent reconstruction
 * by dilation using a marker image that is the original image for all unaffected pixels.
 *
 * Opening by reconstruction is described in Chapter 6.3.9 of Pierre Soille's book "Morphological Image Analysis: Principles and
 * Applications", Second Edition, Springer, 2003.
 *
 * @author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.
 *
 *
 * @see GrayscaleMorphologicalOpeningImageFilter
 *
 * ITK Module: ITKMathematicalMorphology
 * ITK Group: MathematicalMorphology
 */
class ITKIMAGEPROCESSING_EXPORT ITKOpeningByReconstructionImageFilter : public IFilter
{
public:
  ITKOpeningByReconstructionImageFilter() = default;
  ~ITKOpeningByReconstructionImageFilter() noexcept override = default;

  ITKOpeningByReconstructionImageFilter(const ITKOpeningByReconstructionImageFilter&) = delete;
  ITKOpeningByReconstructionImageFilter(ITKOpeningByReconstructionImageFilter&&) noexcept = delete;

  ITKOpeningByReconstructionImageFilter& operator=(const ITKOpeningByReconstructionImageFilter&) = delete;
  ITKOpeningByReconstructionImageFilter& operator=(ITKOpeningByReconstructionImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static inline constexpr StringLiteral k_KernelType_Key = "kernel_type_index";
  static inline constexpr StringLiteral k_FullyConnected_Key = "fully_connected";
  static inline constexpr StringLiteral k_PreserveIntensities_Key = "preserve_intensities";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKOpeningByReconstructionImageFilter, "c4225a23-0b23-4782-b509-296fb39a672b");
