#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include <string>

namespace nx::core
{

/**
 * @struct QuickSurfaceMeshInputValues
 * @brief Aggregates every user-facing parameter and internally-created
 * DataPath needed by the QuickSurfaceMesh algorithm family.
 */
struct SIMPLNXCORE_EXPORT QuickSurfaceMeshInputValues
{
  bool FixProblemVoxels;      ///< When true, run iterative problem-voxel correction before meshing
  bool RepairTriangleWinding; ///< When true, run winding repair on the output triangle mesh
  bool GenerateTripleLines;   ///< When true, generate an EdgeGeom of triple lines (currently unused)

  DataPath GridGeomDataPath;                                             ///< Path to the input IGridGeometry (ImageGeom or RectGridGeom)
  DataPath FeatureIdsArrayPath;                                          ///< Path to the Int32 FeatureIds cell array
  MultiArraySelectionParameter::ValueType SelectedCellDataArrayPaths;    ///< Cell arrays to transfer to the triangle face attribute matrix
  MultiArraySelectionParameter::ValueType SelectedFeatureDataArrayPaths; ///< Feature arrays to transfer to the triangle face attribute matrix
  DataPath TriangleGeometryPath;                                         ///< Path to the created TriangleGeom output
  DataPath VertexGroupDataPath;                                          ///< Path to the vertex attribute matrix
  DataPath NodeTypesDataPath;                                            ///< Path to the Int8 NodeTypes vertex array
  DataPath FaceGroupDataPath;                                            ///< Path to the face attribute matrix
  DataPath FaceLabelsDataPath;                                           ///< Path to the Int32 FaceLabels (2-component) face array
  MultiArraySelectionParameter::ValueType CreatedDataArrayPaths;         ///< Paths to the created face arrays (transferred cell/feature data)
};

/**
 * @class QuickSurfaceMesh
 * @brief Dispatcher that selects between QuickSurfaceMeshDirect (in-core) and
 * QuickSurfaceMeshScanline (OOC) based on the storage type of input arrays.
 *
 * The algorithm generates a triangle mesh representing feature boundaries in
 * a grid geometry. For every voxel face shared by two cells with different
 * FeatureId values, two triangles are emitted. Boundary faces at the volume
 * edges also produce triangles with FaceLabel = -1 on the exterior side.
 *
 * Dispatch is performed by DispatchAlgorithm: if the FeatureIds array is
 * backed by an in-memory DataStore, QuickSurfaceMeshDirect is used. If it
 * is backed by a chunked out-of-core store, QuickSurfaceMeshScanline is
 * selected instead to avoid chunk thrashing.
 *
 * @see QuickSurfaceMeshDirect, QuickSurfaceMeshScanline
 */
class SIMPLNXCORE_EXPORT QuickSurfaceMesh
{
public:
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;
  using TriStore = AbstractDataStore<IGeometry::SharedTriList::value_type>;
  using MeshIndexType = IGeometry::MeshIndexType;

  /**
   * @brief Constructs the dispatcher.
   * @param dataStructure The DataStructure containing all input/output objects
   * @param inputValues Pointer to the parameter struct (must outlive this object)
   * @param shouldCancel Atomic flag checked periodically for user cancellation
   * @param mesgHandler Callback for progress and status messages
   */
  QuickSurfaceMesh(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~QuickSurfaceMesh() noexcept;

  QuickSurfaceMesh(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh(QuickSurfaceMesh&&) noexcept = delete;
  QuickSurfaceMesh& operator=(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh& operator=(QuickSurfaceMesh&&) noexcept = delete;

  /**
   * @brief Dispatches to the appropriate in-core or OOC algorithm implementation.
   * @return Result<> indicating success or an error code from the selected algorithm
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                             ///< Reference to the active DataStructure
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr; ///< User parameters and created array paths
  const std::atomic_bool& m_ShouldCancel;                     ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;            ///< Progress message callback
};
} // namespace nx::core
