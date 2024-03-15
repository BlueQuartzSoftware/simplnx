#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKCurvatureAnisotropicDiffusionImage
 * @brief This filter performs anisotropic diffusion on a scalar itk::Image using the modified curvature diffusion equation (MCDE).
 *
 * For detailed information on anisotropic diffusion and the MCDE see itkAnisotropicDiffusionFunction and itkCurvatureNDAnisotropicDiffusionFunction.
 *
 * \par Inputs and Outputs
 * The input and output to this filter must be a scalar itk::Image with numerical pixel types (float or double). A user defined type which correctly defines arithmetic operations with floating point
 * accuracy should also give correct results.
 *
 *
 * \par Parameters
 * Please first read all the documentation found in AnisotropicDiffusionImageFilter and AnisotropicDiffusionFunction . Also see CurvatureNDAnisotropicDiffusionFunction .
 *
 *
 * The default time step for this filter is set to the maximum theoretically stable value: 0.5 / 2^N, where N is the dimensionality of the image. For a 2D image, this means valid time steps are below
 * 0.1250. For a 3D image, valid time steps are below 0.0625.
 *
 * @see AnisotropicDiffusionImageFilter
 *
 *
 * @see AnisotropicDiffusionFunction
 *
 *
 * @see CurvatureNDAnisotropicDiffusionFunction
 *
 * ITK Module: ITKAnisotropicSmoothing
 * ITK Group: AnisotropicSmoothing
 */
class ITKIMAGEPROCESSING_EXPORT ITKCurvatureAnisotropicDiffusionImage : public IFilter
{
public:
  ITKCurvatureAnisotropicDiffusionImage() = default;
  ~ITKCurvatureAnisotropicDiffusionImage() noexcept override = default;

  ITKCurvatureAnisotropicDiffusionImage(const ITKCurvatureAnisotropicDiffusionImage&) = delete;
  ITKCurvatureAnisotropicDiffusionImage(ITKCurvatureAnisotropicDiffusionImage&&) noexcept = delete;

  ITKCurvatureAnisotropicDiffusionImage& operator=(const ITKCurvatureAnisotropicDiffusionImage&) = delete;
  ITKCurvatureAnisotropicDiffusionImage& operator=(ITKCurvatureAnisotropicDiffusionImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "selected_image_geom_path";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKCurvatureAnisotropicDiffusionImage, "ada68f29-b1f2-44a2-86dc-f0cd28f54633");
