#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT M3CSurfaceMeshingInputValues
{
  bool RepairTriangleWinding;
  DataPath GridGeomDataPath;
  DataPath FeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType SelectedCellDataArrayPaths;
  MultiArraySelectionParameter::ValueType SelectedFeatureDataArrayPaths;
  DataPath TriangleGeometryPath;
  DataPath VertexGroupDataPath;
  DataPath NodeTypesDataPath;
  DataPath FaceGroupDataPath;
  DataPath FaceLabelsDataPath;
  MultiArraySelectionParameter::ValueType CreatedDataArrayPaths;
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

  // Whole-volume variant (the original M3CEntireVolume port): allocates all per-site scratch
  // (squares/nodeType/newNodeIds) over the entire volume. Peak memory is O(volume).
  Result<> runEntireVolume();

  // Sliding-window variant: sweeps the volume z-slice by z-slice, keeping only a few slices of the
  // per-site square scratch resident at once, so that scratch is O(sliceArea) = O(N^2/3) instead of
  // O(volume) (nodeType remains a whole-volume int8 array). Produces byte-identical output to
  // runEntireVolume(). This is the DEFAULT path; set the environment variable M3C_WHOLE_VOLUME=1 to
  // force runEntireVolume() (kept as a byte-identical reference).
  Result<> runWindowed();
};
} // namespace nx::core
