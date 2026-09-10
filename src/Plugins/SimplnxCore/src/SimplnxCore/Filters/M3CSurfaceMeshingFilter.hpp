#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class M3CSurfaceMeshingFilter
 * @brief Generates a watertight, conformal multi-material triangle surface mesh from a segmented
 * (FeatureIds) Image geometry using the Multi-Material Marching Cubes (M3C) algorithm.
 *
 * This is a port of the legacy DREAM3D `M3CSliceBySlice`/`M3CEntireVolume` filters. The algorithm
 * was originally contributed by Dr. Sukbin Lee (CMU) and is based on Wu & Sullivan (2003),
 * "Multiple material marching cubes algorithm," Int. J. Numer. Methods Eng. 58(2):189-207.
 *
 * Unlike SurfaceNets (a dual method), M3C is a primal marching-cubes method: mesh vertices are
 * placed on cell edges/faces where the FeatureId changes, and a multi-material case table drives
 * triangle emission. Output data model is identical to QuickSurfaceMesh / SurfaceNets:
 * a TriangleGeom + 2-component FaceLabels (int32) + NodeTypes (int8).
 */
class SIMPLNXCORE_EXPORT M3CSurfaceMeshingFilter : public IFilter
{
public:
  M3CSurfaceMeshingFilter() = default;
  ~M3CSurfaceMeshingFilter() noexcept override = default;

  M3CSurfaceMeshingFilter(const M3CSurfaceMeshingFilter&) = delete;
  M3CSurfaceMeshingFilter(M3CSurfaceMeshingFilter&&) noexcept = delete;

  M3CSurfaceMeshingFilter& operator=(const M3CSurfaceMeshingFilter&) = delete;
  M3CSurfaceMeshingFilter& operator=(M3CSurfaceMeshingFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_GridGeometryDataPath_Key = "input_grid_geometry_path";
  static constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_path";
  static constexpr StringLiteral k_SelectedDataArrayPaths_Key = "input_data_array_paths";
  static constexpr StringLiteral k_SelectedFeatureDataArrayPaths_Key = "input_feature_data_array_paths";
  static constexpr StringLiteral k_RepairTriangleWinding_Key = "repair_triangle_winding";
  static constexpr StringLiteral k_BoundingBoxSkinMode_Key = "bounding_box_skin_mode_index";
  static constexpr StringLiteral k_SharpBoundingBoxEdges_Key = "sharp_bounding_box_edges";

  static constexpr StringLiteral k_CreatedTriangleGeometryPath_Key = "output_triangle_geometry_path";

  static constexpr StringLiteral k_VertexDataGroupName_Key = "vertex_data_group_name";
  static constexpr StringLiteral k_NodeTypesArrayName_Key = "node_types_array_name";

  static constexpr StringLiteral k_FaceDataGroupName_Key = "face_data_group_name";
  static constexpr StringLiteral k_FaceLabelsArrayName_Key = "face_labels_array_name";

  /**
   * @brief Reads SIMPL json and converts it to simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  std::string name() const override;
  std::string className() const override;
  Uuid uuid() const override;
  std::string humanName() const override;
  std::vector<std::string> defaultTags() const override;
  Parameters parameters() const override;
  VersionType parametersVersion() const override;
  UniquePointer clone() const override;

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, M3CSurfaceMeshingFilter, "382ee804-ac3c-48d1-8632-a48e15c32779");
