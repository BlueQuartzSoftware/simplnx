#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class UncertainRegularGridSampleSurfaceMeshFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT UncertainRegularGridSampleSurfaceMeshFilter : public IFilter
{
public:
  UncertainRegularGridSampleSurfaceMeshFilter() = default;
  ~UncertainRegularGridSampleSurfaceMeshFilter() noexcept override = default;

  UncertainRegularGridSampleSurfaceMeshFilter(const UncertainRegularGridSampleSurfaceMeshFilter&) = delete;
  UncertainRegularGridSampleSurfaceMeshFilter(UncertainRegularGridSampleSurfaceMeshFilter&&) noexcept = delete;

  UncertainRegularGridSampleSurfaceMeshFilter& operator=(const UncertainRegularGridSampleSurfaceMeshFilter&) = delete;
  UncertainRegularGridSampleSurfaceMeshFilter& operator=(UncertainRegularGridSampleSurfaceMeshFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_TriangleGeometryPath_Key = "input_triangle_geometry_path";
  static inline constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPath_Key = "surface_mesh_face_labels_array_path";
  static inline constexpr StringLiteral k_Spacing_Key = "spacing";
  static inline constexpr StringLiteral k_Origin_Key = "origin";
  static inline constexpr StringLiteral k_Uncertainty_Key = "uncertainty";
  static inline constexpr StringLiteral k_ImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_CellAMName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_FeatureIdsArrayName_Key = "feature_ids_array_name";
  static inline constexpr StringLiteral k_UseSeed_Key = "use_seed";
  static inline constexpr StringLiteral k_SeedValue_Key = "seed_value";
  static inline constexpr StringLiteral k_SeedArrayName_Key = "seed_array_name";
  static inline constexpr StringLiteral k_Dimensions_Key = "dimensions";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, UncertainRegularGridSampleSurfaceMeshFilter, "c594cc3b-3f45-43b1-9030-1aa56c755fc8");
/* LEGACY UUID FOR THIS FILTER 75cfeb9b-cd4b-5a20-a344-4170b39bbfaf */
