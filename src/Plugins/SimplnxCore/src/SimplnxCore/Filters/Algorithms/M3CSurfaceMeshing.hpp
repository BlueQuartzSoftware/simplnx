#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT M3CSurfaceMeshingInputValues
{
  bool RepairTriangleWinding;
  DataPath GridGeomDataPath;
  DataPath FeatureIdsArrayPath;
  DataPath TriangleGeometryPath;
  DataPath VertexGroupDataPath;
  DataPath NodeTypesDataPath;
  DataPath FaceGroupDataPath;
  DataPath FaceLabelsDataPath;
};

/**
 * @class M3CSurfaceMeshing
 * @brief In-memory Multi-Material Marching Cubes surface meshing.
 *
 * Port of the legacy DREAM3D `M3CEntireVolume` algorithm (the all-in-memory variant, contributed by
 * Dr. Sukbin Lee, CMU; based on Wu & Sullivan 2003). The slice-by-slice disk round-trip of
 * `M3CSliceBySlice` is intentionally NOT ported; the whole mesh is accumulated in memory and written
 * directly into the output TriangleGeom.
 *
 * The private method names mirror the legacy pipeline so the port can be filled in stage-by-stage.
 * Grafted from the slice variant: ghost-layer wrapping and FeatureId==0 renumbering (on a local copy).
 */
class SIMPLNXCORE_EXPORT M3CSurfaceMeshing
{
public:
  using MeshIndexType = IGeometry::MeshIndexType;

  M3CSurfaceMeshing(DataStructure& dataStructure, M3CSurfaceMeshingInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~M3CSurfaceMeshing() noexcept;

  M3CSurfaceMeshing(const M3CSurfaceMeshing&) = delete;
  M3CSurfaceMeshing(M3CSurfaceMeshing&&) noexcept = delete;
  M3CSurfaceMeshing& operator=(const M3CSurfaceMeshing&) = delete;
  M3CSurfaceMeshing& operator=(M3CSurfaceMeshing&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const M3CSurfaceMeshingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // --- Legacy M3CEntireVolume pipeline stages (to be ported incrementally) ---
  // get_neighbor_list, initialize_nodes, initialize_squares,
  // get_number_fEdges + get_nodes_fEdges (uses inline edgeTable_2d/nsTable_2d),
  // get_number_triangles + get_triangles (+ get_case0/2/M_triangles, get_square_index, treat_anomaly),
  // update_triangle_sides_with_fedge,
  // get_number_unique_inner_edges + get_unique_inner_edges (ISegment connectivity),
  // update_node_edge_kind, arrange_spins (per-triangle winding heuristic),
  // assign_new_nodeID, then write final TriangleGeom + FaceLabels + NodeTypes.
};
} // namespace nx::core
