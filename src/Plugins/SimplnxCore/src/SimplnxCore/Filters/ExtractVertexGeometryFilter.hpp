#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ExtractVertexGeometryFilter
 * @brief This filter will extract all the voxel centers of an Image Geometry or a RectilinearGrid
 * geometry into a new VertexGeometry. The user is given the option to copy or move cell arrays
 * over to the newly created VertexGeometry.
 */
class SIMPLNXCORE_EXPORT ExtractVertexGeometryFilter : public IFilter
{
public:
  ExtractVertexGeometryFilter() = default;
  ~ExtractVertexGeometryFilter() noexcept override = default;

  ExtractVertexGeometryFilter(const ExtractVertexGeometryFilter&) = delete;
  ExtractVertexGeometryFilter(ExtractVertexGeometryFilter&&) noexcept = delete;

  ExtractVertexGeometryFilter& operator=(const ExtractVertexGeometryFilter&) = delete;
  ExtractVertexGeometryFilter& operator=(ExtractVertexGeometryFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ArrayHandling_Key = "array_handling_index";
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static inline constexpr StringLiteral k_InputGeometryPath_Key = "input_grid_geometry_path";
  static inline constexpr StringLiteral k_IncludedDataArrayPaths_Key = "included_data_array_paths";
  static inline constexpr StringLiteral k_VertexGeometryPath_Key = "output_vertex_geometry_path";
  static inline constexpr StringLiteral k_VertexAttrMatrixName_Key = "output_vertex_attr_matrix_name";
  static inline constexpr StringLiteral k_SharedVertexListName_Key = "output_shared_vertex_list_name";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ExtractVertexGeometryFilter, "621a71ca-124b-4471-ad1a-02f05ffba099");
