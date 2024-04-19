#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKGradientAnisotropicDiffusionImage
 * @brief This filter performs anisotropic diffusion on a scalar itk::Image using the classic Perona-Malik, gradient magnitude based equation.
 *
 * For detailed information on anisotropic diffusion, see itkAnisotropicDiffusionFunction and itkGradientNDAnisotropicDiffusionFunction.
 *
 * \par Inputs and Outputs
 * The input to this filter should be a scalar itk::Image of any dimensionality. The output image will be a diffused copy of the input.
 *
 *
 * \par Parameters
 * Please see the description of parameters given in itkAnisotropicDiffusionImageFilter.
 *
 *
 * @see AnisotropicDiffusionImageFilter
 *
 *
 * @see AnisotropicDiffusionFunction
 *
 *
 * @see GradientAnisotropicDiffusionFunction
 *
 * ITK Module: ITKAnisotropicSmoothing
 * ITK Group: AnisotropicSmoothing
 */
class ITKIMAGEPROCESSING_EXPORT ITKGradientAnisotropicDiffusionImage : public IFilter
{
public:
  ITKGradientAnisotropicDiffusionImage() = default;
  ~ITKGradientAnisotropicDiffusionImage() noexcept override = default;

  ITKGradientAnisotropicDiffusionImage(const ITKGradientAnisotropicDiffusionImage&) = delete;
  ITKGradientAnisotropicDiffusionImage(ITKGradientAnisotropicDiffusionImage&&) noexcept = delete;

  ITKGradientAnisotropicDiffusionImage& operator=(const ITKGradientAnisotropicDiffusionImage&) = delete;
  ITKGradientAnisotropicDiffusionImage& operator=(ITKGradientAnisotropicDiffusionImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_TimeStep_Key = "time_step";
  static inline constexpr StringLiteral k_ConductanceParameter_Key = "conductance_parameter";
  static inline constexpr StringLiteral k_ConductanceScalingUpdateInterval_Key = "conductance_scaling_update_interval";
  static inline constexpr StringLiteral k_NumberOfIterations_Key = "number_of_iterations";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKGradientAnisotropicDiffusionImage, "9dcef77b-e7d2-4a2a-b310-bfa80e8ea7c5");
