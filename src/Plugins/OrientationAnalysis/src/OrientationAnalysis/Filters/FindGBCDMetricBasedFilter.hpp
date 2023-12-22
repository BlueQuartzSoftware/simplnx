#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FindGBCDMetricBasedFilter
 * @brief This filter computes a section through the five-dimensional grain boundary distribution for a fixed mis orientation.
 */
class ORIENTATIONANALYSIS_EXPORT FindGBCDMetricBasedFilter : public IFilter
{
public:
  FindGBCDMetricBasedFilter() = default;
  ~FindGBCDMetricBasedFilter() noexcept override = default;

  FindGBCDMetricBasedFilter(const FindGBCDMetricBasedFilter&) = delete;
  FindGBCDMetricBasedFilter(FindGBCDMetricBasedFilter&&) noexcept = delete;

  FindGBCDMetricBasedFilter& operator=(const FindGBCDMetricBasedFilter&) = delete;
  FindGBCDMetricBasedFilter& operator=(FindGBCDMetricBasedFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PhaseOfInterest_Key = "phase_of_interest";
  static inline constexpr StringLiteral k_MisorientationRotation_Key = "misorientation_rotation";
  static inline constexpr StringLiteral k_ChosenLimitDists_Key = "chosen_limit_dists";
  static inline constexpr StringLiteral k_NumSamplPts_Key = "num_sampl_pts";
  static inline constexpr StringLiteral k_ExcludeTripleLines_Key = "exclude_triple_lines";
  static inline constexpr StringLiteral k_DistOutputFile_Key = "dist_output_file";
  static inline constexpr StringLiteral k_ErrOutputFile_Key = "err_output_file";
  static inline constexpr StringLiteral k_SaveRelativeErr_Key = "save_relative_err";
  static inline constexpr StringLiteral k_TriangleGeometryPath_Key = "triangle_geometry_path";
  static inline constexpr StringLiteral k_NodeTypesArrayPath_Key = "node_types_array_path";
  static inline constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPath_Key = "surface_mesh_face_labels_array_path";
  static inline constexpr StringLiteral k_SurfaceMeshFaceNormalsArrayPath_Key = "surface_mesh_face_normals_array_path";
  static inline constexpr StringLiteral k_SurfaceMeshFaceAreasArrayPath_Key = "surface_mesh_face_areas_array_path";
  static inline constexpr StringLiteral k_SurfaceMeshFeatureFaceLabelsArrayPath_Key = "surface_mesh_feature_face_labels_array_path";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayPath_Key = "feature_euler_angles_array_path";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";

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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FindGBCDMetricBasedFilter, "fc0d695a-e381-4f11-a8fd-26d5b5cda30a");
/* LEGACY UUID FOR THIS FILTER d67e9f28-2fe5-5188-b0f8-323a7e603de6 */
