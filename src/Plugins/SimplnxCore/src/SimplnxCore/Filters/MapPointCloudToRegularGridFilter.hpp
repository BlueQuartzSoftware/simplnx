#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class MapPointCloudToRegularGridFilter
 * @brief
 */
class SIMPLNXCORE_EXPORT MapPointCloudToRegularGridFilter : public IFilter
{
public:
  MapPointCloudToRegularGridFilter() = default;
  ~MapPointCloudToRegularGridFilter() noexcept override = default;

  MapPointCloudToRegularGridFilter(const MapPointCloudToRegularGridFilter&) = delete;
  MapPointCloudToRegularGridFilter(MapPointCloudToRegularGridFilter&&) noexcept = delete;

  MapPointCloudToRegularGridFilter& operator=(const MapPointCloudToRegularGridFilter&) = delete;
  MapPointCloudToRegularGridFilter& operator=(MapPointCloudToRegularGridFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SamplingGridType_Key = "sampling_grid_index";
  static inline constexpr StringLiteral k_GridDimensions_Key = "grid_dimensions";
  static inline constexpr StringLiteral k_SelectedVertexGeometryPath_Key = "input_vertex_geometry_path";
  static inline constexpr StringLiteral k_CreatedImageGeometryPath_Key = "output_image_geometry_path";
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_OutOfBoundsHandlingType_Key = "out_of_bounds_handling_index";
  static inline constexpr StringLiteral k_OutOfBoundsValue_Key = "out_of_bounds_value";
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_InputMaskPath_Key = "mask_path";
  static inline constexpr StringLiteral k_VoxelIndicesName_Key = "voxel_indices_name";
  static inline constexpr StringLiteral k_CellDataName_Key = "cell_data_name";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
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
   * @brief
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The PipelineNode object that called this filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel The atomic boolean that holds if the filter should be canceled
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, MapPointCloudToRegularGridFilter, "af53ac60-092f-4e4a-9e13-57f0034ce2c7");
