#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class SharedFeatureFaceFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT SharedFeatureFaceFilter : public IFilter
{
public:
  SharedFeatureFaceFilter() = default;
  ~SharedFeatureFaceFilter() noexcept override = default;

  SharedFeatureFaceFilter(const SharedFeatureFaceFilter&) = delete;
  SharedFeatureFaceFilter(SharedFeatureFaceFilter&&) noexcept = delete;

  SharedFeatureFaceFilter& operator=(const SharedFeatureFaceFilter&) = delete;
  SharedFeatureFaceFilter& operator=(SharedFeatureFaceFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_TriGeometryDataPath_Key = "input_triangle_geometry_path";
  static inline constexpr StringLiteral k_FaceLabelsArrayPath_Key = "face_labels_array_path";
  static inline constexpr StringLiteral k_RandomizeFeatures_Key = "randomize_features";
  static inline constexpr StringLiteral k_FeatureFaceIdsArrayName_Key = "feature_face_ids_array_name";
  static inline constexpr StringLiteral k_GrainBoundaryAttributeMatrixName_Key = "grain_boundary_attribute_matrix_name";
  static inline constexpr StringLiteral k_FeatureNumTrianglesArrayName_Key = "feature_num_triangles_array_name";
  static inline constexpr StringLiteral k_FeatureFaceLabelsArrayName_Key = "feature_face_labels_array_name";

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
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, SharedFeatureFaceFilter, "aaf7a258-fc92-48d7-9d06-ba317a3769e8");
