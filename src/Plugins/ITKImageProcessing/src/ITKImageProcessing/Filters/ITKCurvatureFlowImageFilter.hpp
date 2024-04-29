#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKCurvatureFlowImageFilter
 * @brief Denoise an image using curvature driven flow.
 *
 * CurvatureFlowImageFilter implements a curvature driven image denoising algorithm. Iso-brightness contours in the grayscale input image are viewed as a level set. The level set is then evolved using
 * a curvature-based speed function:
 *
 *  \f[ I_t = \kappa |\nabla I| \f] where \f$ \kappa \f$ is the curvature.
 *
 * The advantage of this approach is that sharp boundaries are preserved with smoothing occurring only within a region. However, it should be noted that continuous application of this scheme will
 * result in the eventual removal of all information as each contour shrinks to zero and disappear.
 *
 * Note that unlike level set segmentation algorithms, the image to be denoised is already the level set and can be set directly as the input using the SetInput() method.
 *
 * This filter has two parameters: the number of update iterations to be performed and the timestep between each update.
 *
 * The timestep should be "small enough" to ensure numerical stability. Stability is guarantee when the timestep meets the CFL (Courant-Friedrichs-Levy) condition. Broadly speaking, this condition
 * ensures that each contour does not move more than one grid position at each timestep. In the literature, the timestep is typically user specified and have to manually tuned to the application.
 *
 * This filter make use of the multi-threaded finite difference solver hierarchy. Updates are computed using a CurvatureFlowFunction object. A zero flux Neumann boundary condition when computing
 * derivatives near the data boundary.
 *
 * This filter may be streamed. To support streaming this filter produces a padded output which takes into account edge effects. The size of the padding is m_NumberOfIterations on each edge. Users of
 * this filter should only make use of the center valid central region.
 *
 * \warning This filter assumes that the input and output types have the same dimensions. This filter also requires that the output image pixels are of a floating point type. This filter works for any
 * dimensional images.
 *
 *
 * Reference: "Level Set Methods and Fast Marching Methods", J.A. Sethian, Cambridge Press, Chapter 16, Second edition, 1999.
 *
 * @see DenseFiniteDifferenceImageFilter
 *
 *
 * @see CurvatureFlowFunction
 *
 *
 * @see MinMaxCurvatureFlowImageFilter
 *
 *
 * @see BinaryMinMaxCurvatureFlowImageFilter
 *
 * ITK Module: ITKCurvatureFlow
 * ITK Group: CurvatureFlow
 */
class ITKIMAGEPROCESSING_EXPORT ITKCurvatureFlowImageFilter : public IFilter
{
public:
  ITKCurvatureFlowImageFilter() = default;
  ~ITKCurvatureFlowImageFilter() noexcept override = default;

  ITKCurvatureFlowImageFilter(const ITKCurvatureFlowImageFilter&) = delete;
  ITKCurvatureFlowImageFilter(ITKCurvatureFlowImageFilter&&) noexcept = delete;

  ITKCurvatureFlowImageFilter& operator=(const ITKCurvatureFlowImageFilter&) = delete;
  ITKCurvatureFlowImageFilter& operator=(ITKCurvatureFlowImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_TimeStep_Key = "time_step";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKCurvatureFlowImageFilter, "fe5b2ed3-54dd-4207-ad88-48a95134684a");
