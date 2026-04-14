#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct QuickSurfaceMeshInputValues;

/**
 * @class QuickSurfaceMeshDirect
 * @brief In-core algorithm for QuickSurfaceMesh that uses direct operator[]
 * access on DataStore references.
 *
 * Selected by DispatchAlgorithm when all input arrays are backed by in-memory
 * DataStore. This is the original algorithm implementation and serves as the
 * reference for correctness.
 *
 * The algorithm runs in three phases:
 *   1. **correctProblemVoxels** -- Iteratively resolves diagonal voxel
 *      configurations that would produce non-manifold mesh geometry by
 *      randomly reassigning one of the conflicting voxels to a neighbor's
 *      FeatureId. Up to 20 iterations until no problem voxels remain.
 *   2. **determineActiveNodes** -- First pass over all voxels to count how
 *      many unique mesh vertices (nodes) and triangles will be needed.
 *      Allocates a nodeIds array of size (xP+1)*(yP+1)*(zP+1) to map
 *      grid-corner indices to sequential vertex IDs.
 *   3. **createNodesAndTriangles** -- Second pass that writes vertex
 *      coordinates, triangle connectivity, face labels, node types, and
 *      transferred cell/feature data into the output TriangleGeom.
 *
 * Memory: O((xP+1)*(yP+1)*(zP+1)) for the nodeIds mapping array.
 *
 * @see QuickSurfaceMeshScanline for the OOC-optimized variant
 */
class SIMPLNXCORE_EXPORT QuickSurfaceMeshDirect
{
public:
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;
  using TriStore = AbstractDataStore<IGeometry::SharedTriList::value_type>;
  using MeshIndexType = IGeometry::MeshIndexType;

  /**
   * @brief Constructs the in-core algorithm. Seeds the RNG used by problem-voxel correction.
   * @param dataStructure The DataStructure containing all input/output objects
   * @param mesgHandler Callback for progress and status messages
   * @param shouldCancel Atomic flag checked periodically for user cancellation
   * @param inputValues Pointer to the parameter struct (must outlive this object)
   */
  QuickSurfaceMeshDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const QuickSurfaceMeshInputValues* inputValues);
  ~QuickSurfaceMeshDirect() noexcept;

  QuickSurfaceMeshDirect(const QuickSurfaceMeshDirect&) = delete;
  QuickSurfaceMeshDirect(QuickSurfaceMeshDirect&&) noexcept = delete;
  QuickSurfaceMeshDirect& operator=(const QuickSurfaceMeshDirect&) = delete;
  QuickSurfaceMeshDirect& operator=(QuickSurfaceMeshDirect&&) noexcept = delete;

  /**
   * @brief Executes the full in-core meshing pipeline: problem voxel correction,
   * node counting, mesh generation, and optional winding repair.
   * @return Result<> indicating success or an error from winding repair
   */
  Result<> operator()();

private:
  /**
   * @brief Iteratively resolves non-manifold diagonal voxel configurations.
   *
   * Examines every 2x2x2 block of voxels and detects configurations where
   * diagonally-opposite voxels share a FeatureId but no face-adjacent voxel
   * does. These produce degenerate triangles. The fix randomly reassigns one
   * voxel to a neighbor's value using a seeded RNG for reproducibility.
   */
  void correctProblemVoxels();

  /**
   * @brief First pass: counts active mesh nodes and triangles.
   * @param[in,out] nodeIds Maps grid-corner linear index to sequential vertex ID (initially max)
   * @param[out] nodeCount Total number of unique mesh vertices found
   * @param[out] triangleCount Total number of triangles that will be generated
   */
  void determineActiveNodes(std::vector<MeshIndexType>& nodeIds, MeshIndexType& nodeCount, MeshIndexType& triangleCount);

  /**
   * @brief Second pass: writes vertex coordinates, triangle connectivity,
   * face labels, node types, and runs TupleTransfer for cell/feature arrays.
   * @param[in] nodeIds Grid-corner-to-vertex mapping from determineActiveNodes
   * @param nodeCount Number of vertices (used for resizing)
   * @param triangleCount Number of triangles (used for resizing)
   */
  void createNodesAndTriangles(std::vector<MeshIndexType>& nodeIds, MeshIndexType nodeCount, MeshIndexType triangleCount);

  DataStructure& m_DataStructure;                             ///< Reference to the active DataStructure
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr; ///< User parameters and created array paths
  const std::atomic_bool& m_ShouldCancel;                     ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;            ///< Progress message callback
};

} // namespace nx::core
