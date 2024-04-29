#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKMinMaxCurvatureFlowImageFilter
 * @brief Denoise an image using min/max curvature flow.
 *
 * MinMaxCurvatureFlowImageFilter implements a curvature driven image denoising algorithm. Iso-brightness contours in the grayscale input image are viewed as a level set. The level set is then evolved
 * using a curvature-based speed function:
 *
 *  \f[ I_t = F_{\mbox{minmax}} |\nabla I| \f]
 *
 * where \f$ F_{\mbox{minmax}} = \max(\kappa,0) \f$ if \f$ \mbox{Avg}_{\mbox{stencil}}(x) \f$ is less than or equal to \f$ T_{threshold} \f$ and \f$ \min(\kappa,0) \f$ , otherwise. \f$ \kappa \f$ is
 * the mean curvature of the iso-brightness contour at point \f$ x \f$ .
 *
 * In min/max curvature flow, movement is turned on or off depending on the scale of the noise one wants to remove. Switching depends on the average image value of a region of radius \f$ R \f$ around
 * each point. The choice of \f$ R \f$ , the stencil radius, governs the scale of the noise to be removed.
 *
 * The threshold value \f$ T_{threshold} \f$ is the average intensity obtained in the direction perpendicular to the gradient at point \f$ x \f$ at the extrema of the local neighborhood.
 *
 * This filter make use of the multi-threaded finite difference solver hierarchy. Updates are computed using a MinMaxCurvatureFlowFunction object. A zero flux Neumann boundary condition is used when
 * computing derivatives near the data boundary.
 *
 * \warning This filter assumes that the input and output types have the same dimensions. This filter also requires that the output image pixels are of a real type. This filter works for any
 * dimensional images, however for dimensions greater than 3D, an expensive brute-force search is used to compute the local threshold.
 *
 *
 * Reference: "Level Set Methods and Fast Marching Methods", J.A. Sethian, Cambridge Press, Chapter 16, Second edition, 1999.
 *
 * @see MinMaxCurvatureFlowFunction
 *
 *
 * @see CurvatureFlowImageFilter
 *
 *
 * @see BinaryMinMaxCurvatureFlowImageFilter
 *
 * ITK Module: ITKCurvatureFlow
 * ITK Group: CurvatureFlow
 */
class ITKIMAGEPROCESSING_EXPORT ITKMinMaxCurvatureFlowImageFilter : public IFilter
{
public:
  ITKMinMaxCurvatureFlowImageFilter() = default;
  ~ITKMinMaxCurvatureFlowImageFilter() noexcept override = default;

  ITKMinMaxCurvatureFlowImageFilter(const ITKMinMaxCurvatureFlowImageFilter&) = delete;
  ITKMinMaxCurvatureFlowImageFilter(ITKMinMaxCurvatureFlowImageFilter&&) noexcept = delete;

  ITKMinMaxCurvatureFlowImageFilter& operator=(const ITKMinMaxCurvatureFlowImageFilter&) = delete;
  ITKMinMaxCurvatureFlowImageFilter& operator=(ITKMinMaxCurvatureFlowImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_TimeStep_Key = "time_step";
  static inline constexpr StringLiteral k_NumberOfIterations_Key = "number_of_iterations";
  static inline constexpr StringLiteral k_StencilRadius_Key = "stencil_radius";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKMinMaxCurvatureFlowImageFilter, "b836c081-6692-411d-81d0-a50afce6b288");
