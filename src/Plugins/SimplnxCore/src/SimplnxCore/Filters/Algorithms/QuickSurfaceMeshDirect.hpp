#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

struct QuickSurfaceMeshInputValues;

/**
 * @class QuickSurfaceMeshDirect
 * @brief Generates QuickSurfaceMesh output with direct element access.
 *
 * DispatchAlgorithm normally selects this implementation when all source and output
 * targets use in-memory storage. Tests can force the path for parity validation.
 * The implementation is the topology reference for the scanline path.
 *
 * Optional correction resolves diagonal conflicts with a fixed random-number
 * sequence. A count pass assigns exact vertex IDs and output sizes. A generation
 * pass writes topology, labels, node types, and transferred arrays.
 *
 * The direct path retains one node-ID entry for every dual-grid vertex. Its
 * memory use is proportional to the full volume.
 *
 * @warning Concurrent instances are not safe. Construction and correction use
 * shared translation-unit random-number state.
 *
 * @see QuickSurfaceMeshScanline for bounded out-of-core execution.
 */
class SIMPLNXCORE_EXPORT QuickSurfaceMeshDirect
{
public:
  /**
   * @brief Defines the storage interface for mesh vertex coordinates.
   */
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;
  /**
   * @brief Defines the storage interface for mesh triangle connectivity.
   */
  using TriStore = AbstractDataStore<IGeometry::SharedTriList::value_type>;
  /**
   * @brief Defines the integer type for mesh indices.
   */
  using MeshIndexType = IGeometry::MeshIndexType;

  /**
   * @brief Constructs the direct meshing algorithm.
   * @param dataStructure DataStructure that must outlive the algorithm.
   * @param mesgHandler Message callback that must outlive the algorithm.
   * @param shouldCancel Atomic cancellation flag that must outlive the algorithm.
   * @param inputValues Non-null inputs that must outlive the algorithm.
   * @pre inputValues is not null.
   *
   * The algorithm borrows all arguments and reseeds its problem-voxel random-number generator.
   */
  QuickSurfaceMeshDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const QuickSurfaceMeshInputValues* inputValues);

  /**
   * @brief Destroys the direct meshing algorithm.
   */
  ~QuickSurfaceMeshDirect() noexcept;

  QuickSurfaceMeshDirect(const QuickSurfaceMeshDirect&) = delete;
  QuickSurfaceMeshDirect(QuickSurfaceMeshDirect&&) noexcept = delete;
  QuickSurfaceMeshDirect& operator=(const QuickSurfaceMeshDirect&) = delete;
  QuickSurfaceMeshDirect& operator=(QuickSurfaceMeshDirect&&) noexcept = delete;

  /**
   * @brief Runs correction, counting, mesh generation, and optional winding repair.
   * @return Valid result on completion or cancellation. Returns a winding-repair error otherwise.
   *
   * Cancellation can leave corrected Feature IDs or resized, partially written mesh output.
   */
  Result<> operator()();

private:
  /**
   * @brief Resolves diagonal voxel conflicts for at most 20 passes.
   *
   * A fixed random-number sequence reassigns conflicting Feature IDs.
   * The sequence and conditional order define parity with QuickSurfaceMeshScanline.
   */
  void correctProblemVoxels();

  /**
   * @brief Counts active mesh nodes and triangles.
   * @param[in,out] nodeIds Maps dual-grid indices to vertex IDs. Maximum values identify unassigned nodes.
   * @param[out] nodeCount Receives the number of unique mesh vertices.
   * @param[out] triangleCount Receives the number of generated triangles.
   */
  void determineActiveNodes(std::vector<MeshIndexType>& nodeIds, MeshIndexType& nodeCount, MeshIndexType& triangleCount, MeshIndexType& suppressedFaceCount);

  /**
   * @brief Writes mesh topology, labels, node types, and transferred arrays.
   * @param nodeIds Dual-grid-to-vertex mapping from determineActiveNodes().
   * @param nodeCount Exact vertex count from the count pass.
   * @param triangleCount Exact triangle count from the count pass.
   */
  void createNodesAndTriangles(std::vector<MeshIndexType>& nodeIds, MeshIndexType nodeCount, MeshIndexType triangleCount);

  DataStructure& m_DataStructure;
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
