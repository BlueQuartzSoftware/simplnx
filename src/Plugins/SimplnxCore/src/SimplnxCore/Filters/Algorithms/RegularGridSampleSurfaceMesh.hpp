#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <mutex>

namespace nx::core
{
struct SIMPLNXCORE_EXPORT RegularGridSampleSurfaceMeshInputValues
{
  VectorUInt64Parameter::ValueType Dimensions;
  VectorFloat32Parameter::ValueType Spacing;
  VectorFloat32Parameter::ValueType Origin;
  DataPath TriangleGeometryPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath ImageGeometryOutputPath;
  DataPath FeatureIdsArrayPath;
};

/**
 * @class RegularGridSampleSurfaceMesh
 * @brief Samples a TriangleGeometry onto a regular grid (ImageGeom) using
 * scanline rasterization. For each Z-slice of the grid, triangles are
 * intersected with the Z-plane to produce 2D edges. Each Y-scanline then
 * finds X-intersections with those edges, sorts them, and fills voxels
 * between crossings using face label toggling to assign feature IDs.
 *
 * Z-slices are processed in parallel. Each worker thread rasterizes into
 * a thread-local buffer and then copies results back to the output DataArray
 * under a mutex via copyFromBuffer, ensuring thread safety and efficient
 * bulk I/O with out-of-core DataStore implementations.
 *
 * All input geometry data (faces, vertices, face labels) is pre-loaded into
 * contiguous memory buffers via copyIntoBuffer at algorithm start, so worker
 * threads operate on plain memory arrays with no virtual dispatch per element.
 */
class SIMPLNXCORE_EXPORT RegularGridSampleSurfaceMesh
{
public:
  RegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RegularGridSampleSurfaceMeshInputValues* inputValues);
  ~RegularGridSampleSurfaceMesh() noexcept;

  RegularGridSampleSurfaceMesh(const RegularGridSampleSurfaceMesh&) = delete;
  RegularGridSampleSurfaceMesh(RegularGridSampleSurfaceMesh&&) noexcept = delete;
  RegularGridSampleSurfaceMesh& operator=(const RegularGridSampleSurfaceMesh&) = delete;
  RegularGridSampleSurfaceMesh& operator=(RegularGridSampleSurfaceMesh&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief Thread-safe method to copy a completed Z-slice buffer into the
   * output DataArray using bulk copyFromBuffer. Called by worker threads
   * after rasterizing a slice.
   * @tparam T The element type of the feature IDs array
   * @param zSlice The Z-slice index
   * @param sliceData Raw pointer to the thread-local buffer containing rasterized feature IDs
   * @param count Number of elements in the slice buffer
   */
  template <typename T>
  void sendThreadSafeSliceUpdate(usize zSlice, const T* sliceData, usize count)
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto& featureIdsRef = m_DataStructure.getDataRefAs<DataArray<T>>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
    usize offset = zSlice * m_CellsPerSlice;
    featureIdsRef.copyFromBuffer(offset, nonstd::span<const T>(sliceData, count));
  }

private:
  DataStructure& m_DataStructure;
  const RegularGridSampleSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_Mutex;
  usize m_CellsPerSlice = 0;
};
} // namespace nx::core
