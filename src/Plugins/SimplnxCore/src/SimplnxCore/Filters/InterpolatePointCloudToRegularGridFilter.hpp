#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class InterpolatePointCloudToRegularGridFilter
 * @brief This Filter interpolates the values of arrays stored in a Vertex Geometry onto a user-selected Image Geometry.
 */
class SIMPLNXCORE_EXPORT InterpolatePointCloudToRegularGridFilter : public IFilter
{
public:
  InterpolatePointCloudToRegularGridFilter() = default;
  ~InterpolatePointCloudToRegularGridFilter() noexcept override = default;

  InterpolatePointCloudToRegularGridFilter(const InterpolatePointCloudToRegularGridFilter&) = delete;
  InterpolatePointCloudToRegularGridFilter(InterpolatePointCloudToRegularGridFilter&&) noexcept = delete;

  InterpolatePointCloudToRegularGridFilter& operator=(const InterpolatePointCloudToRegularGridFilter&) = delete;
  InterpolatePointCloudToRegularGridFilter& operator=(InterpolatePointCloudToRegularGridFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_StoreKernelDistances_Key = "store_kernel_distances";
  static inline constexpr StringLiteral k_InterpolationTechnique_Key = "interpolation_technique";
  static inline constexpr StringLiteral k_KernelSize_Key = "kernel_size";
  static inline constexpr StringLiteral k_GaussianSigmas_Key = "guassian_sigmas";
  static inline constexpr StringLiteral k_VertexGeom_Key = "vertex_geom";
  static inline constexpr StringLiteral k_ImageGeom_Key = "image_geom";
  static inline constexpr StringLiteral k_VoxelIndices_Key = "voxel_indices";
  static inline constexpr StringLiteral k_Mask_Key = "mask";
  static inline constexpr StringLiteral k_InterpolateArrays_Key = "interpolate_arrays";
  static inline constexpr StringLiteral k_CopyArrays_Key = "copy_arrays";
  static inline constexpr StringLiteral k_InterpolatedGroup_Key = "interpolated_group";
  static inline constexpr StringLiteral k_KernelDistancesArray_Key = "kernel_distances_array";
  static inline constexpr uint64 k_Uniform = 0;
  static inline constexpr uint64 k_Gaussian = 1;

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the filter's name.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the filter's uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the filter's human-readable name.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns a collection of filter parameters.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Creates a copy of the filter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, InterpolatePointCloudToRegularGridFilter, "996c4464-08f0-4268-a8a6-f9796c88cf58");
