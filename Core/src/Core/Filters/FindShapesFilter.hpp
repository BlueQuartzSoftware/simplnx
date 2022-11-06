#pragma once

#include "Core/Core_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindShapesFilter
 *
 * @brief This filter will ....
 */
class CORE_EXPORT FindShapesFilter : public IFilter
{
public:
  FindShapesFilter() = default;
  ~FindShapesFilter() noexcept override = default;

  FindShapesFilter(const FindShapesFilter&) = delete;
  FindShapesFilter(FindShapesFilter&&) noexcept = delete;

  FindShapesFilter& operator=(const FindShapesFilter&) = delete;
  FindShapesFilter& operator=(FindShapesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_CentroidsArrayPath_Key = "centroids_array_path";
  static inline constexpr StringLiteral k_Omega3sArrayName_Key = "omega3s_array_name";
  static inline constexpr StringLiteral k_AxisLengthsArrayName_Key = "axis_lengths_array_name";
  static inline constexpr StringLiteral k_AxisEulerAnglesArrayName_Key = "axis_euler_angles_array_name";
  static inline constexpr StringLiteral k_AspectRatiosArrayName_Key = "aspect_ratios_array_name";
  static inline constexpr StringLiteral k_VolumesArrayName_Key = "volumes_array_name";
  static inline constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindShapesFilter, "036b17d5-23bb-4a24-9187-c4a8dd918792");
