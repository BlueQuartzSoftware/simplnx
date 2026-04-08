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
 * @brief In-core algorithm for QuickSurfaceMesh. Preserves the original
 * sequential voxel iteration using operator[] on DataStore references.
 * Selected by DispatchAlgorithm when all input arrays are backed by
 * in-memory DataStore.
 */
class SIMPLNXCORE_EXPORT QuickSurfaceMeshDirect
{
public:
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;
  using TriStore = AbstractDataStore<IGeometry::SharedTriList::value_type>;
  using MeshIndexType = IGeometry::MeshIndexType;

  QuickSurfaceMeshDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const QuickSurfaceMeshInputValues* inputValues);
  ~QuickSurfaceMeshDirect() noexcept;

  QuickSurfaceMeshDirect(const QuickSurfaceMeshDirect&) = delete;
  QuickSurfaceMeshDirect(QuickSurfaceMeshDirect&&) noexcept = delete;
  QuickSurfaceMeshDirect& operator=(const QuickSurfaceMeshDirect&) = delete;
  QuickSurfaceMeshDirect& operator=(QuickSurfaceMeshDirect&&) noexcept = delete;

  Result<> operator()();

private:
  void correctProblemVoxels();
  void determineActiveNodes(std::vector<MeshIndexType>& nodeIds, MeshIndexType& nodeCount, MeshIndexType& triangleCount);
  void createNodesAndTriangles(std::vector<MeshIndexType>& nodeIds, MeshIndexType nodeCount, MeshIndexType triangleCount);

  DataStructure& m_DataStructure;
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
