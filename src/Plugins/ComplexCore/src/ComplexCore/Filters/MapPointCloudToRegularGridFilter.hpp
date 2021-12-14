#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class MapPointCloudToRegularGridFilter
 * @brief 
 */
class COMPLEXCORE_EXPORT MapPointCloudToRegularGridFilter : public IFilter
{
public:
  MapPointCloudToRegularGridFilter() = default;
  ~MapPointCloudToRegularGridFilter() noexcept override = default;

  MapPointCloudToRegularGridFilter(const MapPointCloudToRegularGridFilter&) = delete;
  MapPointCloudToRegularGridFilter(MapPointCloudToRegularGridFilter&&) noexcept = delete;

  MapPointCloudToRegularGridFilter& operator=(const MapPointCloudToRegularGridFilter&) = delete;
  MapPointCloudToRegularGridFilter& operator=(MapPointCloudToRegularGridFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SamplingGridType_Key = "sampling_grid_type";
  static inline constexpr StringLiteral k_GridDimensions_Key = "grid_dimensions";
  static inline constexpr StringLiteral k_VertexGeometry_Key = "vertex_geometry";
  static inline constexpr StringLiteral k_ImageGeometry_Key = "image_geometry";
  static inline constexpr StringLiteral k_ArraysToMap_Key = "arrays_to_map";
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_MaskPath_Key = "mask";
  static inline constexpr StringLiteral k_VoxelIndices_Key = "voxel_indices";

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
   * @brief
   * @return
   */
  Parameters parameters() const override;

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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, MapPointCloudToRegularGridFilter, "9fe34deb-99e1-5f3a-a9cc-e90c655b47ee");
