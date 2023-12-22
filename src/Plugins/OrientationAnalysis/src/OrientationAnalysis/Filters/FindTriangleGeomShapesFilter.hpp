#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FindTriangleGeomShapesFilter
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT FindTriangleGeomShapesFilter : public IFilter
{
public:
  FindTriangleGeomShapesFilter() = default;
  ~FindTriangleGeomShapesFilter() noexcept override = default;

  FindTriangleGeomShapesFilter(const FindTriangleGeomShapesFilter&) = delete;
  FindTriangleGeomShapesFilter(FindTriangleGeomShapesFilter&&) noexcept = delete;

  FindTriangleGeomShapesFilter& operator=(const FindTriangleGeomShapesFilter&) = delete;
  FindTriangleGeomShapesFilter& operator=(FindTriangleGeomShapesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FaceLabelsArrayPath_Key = "face_labels_array_path";
  static inline constexpr StringLiteral k_FeatureAttributeMatrixName_Key = "feature_attribute_matrix_name";
  static inline constexpr StringLiteral k_CentroidsArrayPath_Key = "centroids_array_path";
  static inline constexpr StringLiteral k_VolumesArrayPath_Key = "volumes_array_path";
  static inline constexpr StringLiteral k_Omega3sArrayName_Key = "omega3s_array_name";
  static inline constexpr StringLiteral k_AxisLengthsArrayName_Key = "axis_lengths_array_name";
  static inline constexpr StringLiteral k_AxisEulerAnglesArrayName_Key = "axis_euler_angles_array_name";
  static inline constexpr StringLiteral k_AspectRatiosArrayName_Key = "aspect_ratios_array_name";
  static inline constexpr StringLiteral k_TriGeometryDataPath_Key = "triangle_geometry_path";

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
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FindTriangleGeomShapesFilter, "e8f0fed3-d0d8-456e-b5a1-7961cc17b739");
