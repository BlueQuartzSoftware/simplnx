#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class VoxelizePointCloudFilter
 * @brief This filter creates a grid mask of a point cloud in an existing Image Geometry or optionally
 * creates a new image geometry that fits the point cloud and includes the mask
 */
class SIMPLNXCORE_EXPORT VoxelizePointCloudFilter : public IFilter
{
public:
  VoxelizePointCloudFilter() = default;
  ~VoxelizePointCloudFilter() noexcept override = default;

  VoxelizePointCloudFilter(const VoxelizePointCloudFilter&) = delete;
  VoxelizePointCloudFilter(VoxelizePointCloudFilter&&) noexcept = delete;

  VoxelizePointCloudFilter& operator=(const VoxelizePointCloudFilter&) = delete;
  VoxelizePointCloudFilter& operator=(VoxelizePointCloudFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_UseExistingGeom_Key = "use_existing_geom";
  static constexpr StringLiteral k_PointCloudGeometryPath_Key = "point_cloud_geometry_path";
  static constexpr StringLiteral k_OutputGeometryPath_Key = "output_geometry_path";
  static constexpr StringLiteral k_MaskName_Key = "mask_name";
  static constexpr StringLiteral k_NewGeometryPath_Key = "output_image_geometry_path";

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
   * @brief Returns the human-readable name of the filter.
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
   * The Initial version should always be 1.
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
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, VoxelizePointCloudFilter, "10140e3c-84d4-43fc-889e-bbdd69b0c258");
/* LEGACY UUID FOR THIS FILTER 52b2918a-4fb5-57aa-97d4-ccc084b89572 */
