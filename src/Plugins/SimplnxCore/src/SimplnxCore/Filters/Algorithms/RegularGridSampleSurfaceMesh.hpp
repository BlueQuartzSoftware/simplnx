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
 * under a mutex, ensuring thread safety with out-of-core DataStore implementations.
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
   * output DataArray. Called by worker threads after rasterizing a slice.
   * @param zSlice The Z-slice index
   * @param sliceBuffer The thread-local buffer containing rasterized feature IDs
   */
  template <typename OutputT>
  void sendThreadSafeSliceUpdate(usize zSlice, const std::vector<OutputT>& sliceBuffer)
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto& featureIdsRef = m_DataStructure.getDataRefAs<DataArray<OutputT>>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
    usize offset = zSlice * m_CellsPerSlice;
    for(usize i = 0; i < m_CellsPerSlice; i++)
    {
      featureIdsRef[offset + i] = sliceBuffer[i];
    }
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
