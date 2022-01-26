#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKCurvatureAnisotropicDiffusionImage
 * @brief
 *
 * This filter performs anisotropic diffusion on a scalar itk::Image using the modified curvature diffusion equation (MCDE) implemented in itkCurvatureNDAnisotropicDiffusionFunction. For detailed
 * information on anisotropic diffusion and the MCDE see itkAnisotropicDiffusionFunction and itkCurvatureNDAnisotropicDiffusionFunction.
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
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_TimeStep_Key = "TimeStep";
  static inline constexpr StringLiteral k_ConductanceParameter_Key = "ConductanceParameter";
  static inline constexpr StringLiteral k_ConductanceScalingUpdateInterval_Key = "ConductanceScalingUpdateInterval";
  static inline constexpr StringLiteral k_NumberOfIterations_Key = "NumberOfIterations";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKCurvatureAnisotropicDiffusionImage, "009fb2d0-6f65-5406-bb2a-4a883d0bc18c");
