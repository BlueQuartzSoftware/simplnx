#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FeatureFaceCurvatureFilter
 * @brief This filter will replace data using data from surrounding voxels. See more
 * at the help file.
 */
class SIMPLNXCORE_EXPORT FeatureFaceCurvatureFilter : public IFilter
{
public:
  using FaceIds_t = std::vector<int64_t>;
  using SharedFeatureFaces_t = std::map<int32_t, FaceIds_t>;

  FeatureFaceCurvatureFilter() = default;
  ~FeatureFaceCurvatureFilter() noexcept override = default;

  FeatureFaceCurvatureFilter(const FeatureFaceCurvatureFilter&) = delete;
  FeatureFaceCurvatureFilter(FeatureFaceCurvatureFilter&&) noexcept = delete;

  FeatureFaceCurvatureFilter& operator=(const FeatureFaceCurvatureFilter&) = delete;
  FeatureFaceCurvatureFilter& operator=(FeatureFaceCurvatureFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_TriangleGeom_Key = "triangle_geom";
  static inline constexpr StringLiteral k_NeighborhoodRing_Key = "neighborhood_ring";
  static inline constexpr StringLiteral k_ComputePrincipalDirection_Key = "compute_principal_direction";
  static inline constexpr StringLiteral k_ComputeGaussianCurvature_Key = "compute_gaussian_curvature";
  static inline constexpr StringLiteral k_ComputeMeanCurvature_Key = "compute_mean_curvature";
  static inline constexpr StringLiteral k_ComputeWeingartenMatrix_Key = "compute_weingarten_matrix";
  static inline constexpr StringLiteral k_UseFaceNormals_Key = "use_normals";
  static inline constexpr StringLiteral k_FaceAttribMatrix_Key = "face_attribute_matrix";
  static inline constexpr StringLiteral k_FaceLabels_Key = "face_labels";
  static inline constexpr StringLiteral k_FeatureFaceIds_Key = "feature_face_ids";
  static inline constexpr StringLiteral k_FaceNormals_Key = "face_normals";
  static inline constexpr StringLiteral k_FaceCentroids_Key = "face_centroids";
  static inline constexpr StringLiteral k_PrincipalCurvature1_Key = "principal_curvature_1";
  static inline constexpr StringLiteral k_PrincipalCurvature2_Key = "principal_curvature_2";
  static inline constexpr StringLiteral k_PrincipalDirection1_Key = "principal_direction_1";
  static inline constexpr StringLiteral k_PrincipalDirection2_Key = "principal_direction_2";
  static inline constexpr StringLiteral k_GaussianCurvature_Key = "gaussian_curvature";
  static inline constexpr StringLiteral k_MeanCurvature_Key = "mean_curvature";
  static inline constexpr StringLiteral k_WeingartenMatrix_Key = "weingarten_matrix";

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
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FeatureFaceCurvatureFilter, "56165fa5-dd80-4f9e-9e13-1c516f65060f");
