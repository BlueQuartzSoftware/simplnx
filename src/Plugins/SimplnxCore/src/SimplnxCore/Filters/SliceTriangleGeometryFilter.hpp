#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class SliceTriangleGeometryFilter
 * @brief This filter slices an input Triangle Geometry, producing an Edge Geometry
 */
class SIMPLNXCORE_EXPORT SliceTriangleGeometryFilter : public IFilter
{
public:
  SliceTriangleGeometryFilter() = default;
  ~SliceTriangleGeometryFilter() noexcept override = default;

  SliceTriangleGeometryFilter(const SliceTriangleGeometryFilter&) = delete;
  SliceTriangleGeometryFilter(SliceTriangleGeometryFilter&&) noexcept = delete;

  SliceTriangleGeometryFilter& operator=(const SliceTriangleGeometryFilter&) = delete;
  SliceTriangleGeometryFilter& operator=(SliceTriangleGeometryFilter&&) noexcept = delete;

  // Parameter Keys
  // static inline constexpr StringLiteral k_SliceDirection_Key = "slice_direction";
  static inline constexpr StringLiteral k_SliceRange_Key = "slice_range_index";
  static inline constexpr StringLiteral k_Zstart_Key = "z_start_value";
  static inline constexpr StringLiteral k_Zend_Key = "z_end_value";
  static inline constexpr StringLiteral k_SliceResolution_Key = "slice_spacing_value";
  static inline constexpr StringLiteral k_HaveRegionIds_Key = "have_region_ids";
  static inline constexpr StringLiteral k_RegionIdArrayPath_Key = "region_ids_array_path";

  static inline constexpr StringLiteral k_TriangleGeometryDataPath_Key = "input_triangle_geometry_path";

  static inline constexpr StringLiteral k_OutputEdgeGeometryPath_Key = "output_edge_geometry_path";
  static inline constexpr StringLiteral k_EdgeAttributeMatrixName_Key = "edge_attribute_matrix_name";

  static inline constexpr StringLiteral k_SliceAttributeMatrixName_Key = "slice_attribute_matrix_name";
  static inline constexpr StringLiteral k_SliceIdArrayName_Key = "slice_ids_array_name";
  static inline constexpr StringLiteral k_RemoveDuplicateNodes_Key = "remove_duplicate_nodes";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, SliceTriangleGeometryFilter, "754a47c0-149a-4109-a810-2b2968743895");
/* LEGACY UUID FOR THIS FILTER 222307a4-67fd-5cb5-a12e-d80f9fb970ae */
