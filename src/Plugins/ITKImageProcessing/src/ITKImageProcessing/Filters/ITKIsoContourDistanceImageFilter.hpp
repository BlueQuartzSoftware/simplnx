#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKIsoContourDistanceImageFilter
 * @brief Compute an approximate distance from an interpolated isocontour to the close grid points.
 *
 * For standard level set algorithms, it is useful to periodically reinitialize the evolving image to prevent numerical accuracy problems in computing derivatives. This reinitialization is done by
 * computing a signed distance map to the current level set. This class provides the first step in this reinitialization by computing an estimate of the distance from the interpolated isocontour to
 * the pixels (or voxels) that are close to it, i.e. for which the isocontour crosses a segment between them and one of their direct neighbors. This class supports narrowbanding. If the input
 * narrowband is provided, the algorithm will only locate the level set within the input narrowband.
 *
 * Implementation of this class is based on Fast and Accurate Redistancing for Level Set Methods Krissian K. and Westin C.F., EUROCAST NeuroImaging Workshop Las Palmas Spain, Ninth International
 * Conference on Computer Aided Systems Theory , pages 48-51, Feb 2003.
 *
 * ITK Module: ITKDistanceMap
 * ITK Group: DistanceMap
 */
class ITKIMAGEPROCESSING_EXPORT ITKIsoContourDistanceImageFilter : public IFilter
{
public:
  ITKIsoContourDistanceImageFilter() = default;
  ~ITKIsoContourDistanceImageFilter() noexcept override = default;

  ITKIsoContourDistanceImageFilter(const ITKIsoContourDistanceImageFilter&) = delete;
  ITKIsoContourDistanceImageFilter(ITKIsoContourDistanceImageFilter&&) noexcept = delete;

  ITKIsoContourDistanceImageFilter& operator=(const ITKIsoContourDistanceImageFilter&) = delete;
  ITKIsoContourDistanceImageFilter& operator=(ITKIsoContourDistanceImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_LevelSetValue_Key = "level_set_value";
  static inline constexpr StringLiteral k_FarValue_Key = "far_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKIsoContourDistanceImageFilter, "e82fa143-7ac2-4c09-a88c-9ea71d47d594");
