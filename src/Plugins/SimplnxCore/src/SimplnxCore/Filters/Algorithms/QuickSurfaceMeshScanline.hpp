#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <memory>

namespace nx::core
{
struct QuickSurfaceMeshInputValues;

/**
 * @class QuickSurfaceMeshScanline
 * @brief Out-of-core algorithm for QuickSurfaceMesh. Selected by
 * DispatchAlgorithm when any input array is backed by chunked (OOC) storage.
 *
 * Buffers featureIds in z-slice pairs using copyIntoBuffer to avoid
 * per-element virtual dispatch through AbstractDataStore::operator[].
 * The correctProblemVoxels pass uses double-buffered z-slice pairs with
 * copyFromBuffer write-back. The countActiveNodesAndTriangles and
 * createNodesAndTriangles passes use rolling 2-plane node buffers of
 * size O((xP+1)*(yP+1)) instead of the O(volume) nodeIds array.
 */
class SIMPLNXCORE_EXPORT QuickSurfaceMeshScanline
{
public:
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;
  using TriStore = AbstractDataStore<IGeometry::SharedTriList::value_type>;
  using MeshIndexType = IGeometry::MeshIndexType;

  QuickSurfaceMeshScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const QuickSurfaceMeshInputValues* inputValues);
  ~QuickSurfaceMeshScanline() noexcept;

  QuickSurfaceMeshScanline(const QuickSurfaceMeshScanline&) = delete;
  QuickSurfaceMeshScanline(QuickSurfaceMeshScanline&&) noexcept = delete;
  QuickSurfaceMeshScanline& operator=(const QuickSurfaceMeshScanline&) = delete;
  QuickSurfaceMeshScanline& operator=(QuickSurfaceMeshScanline&&) noexcept = delete;

  Result<> operator()();

private:
  void correctProblemVoxels();
  void countActiveNodesAndTriangles(MeshIndexType& nodeCount, MeshIndexType& triangleCount, usize& numFeatures);
  void createNodesAndTriangles(MeshIndexType nodeCount, MeshIndexType triangleCount, usize numFeatures);

  DataStructure& m_DataStructure;
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
