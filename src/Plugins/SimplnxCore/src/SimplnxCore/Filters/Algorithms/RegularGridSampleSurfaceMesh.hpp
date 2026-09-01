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
/**
 * @struct RegularGridSampleSurfaceMeshInputValues
 * @brief Stores output-grid settings and source/output paths.
 */
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
 * @brief Rasterizes a labeled TriangleGeom into an ImageGeom.
 *
 * Each Z worker intersects triangles with the slice plane. Sorted X crossings
 * toggle face labels to assign output cells along each Y row.
 *
 * The algorithm materializes all faces, vertices, and face labels before parallel
 * work. Each active worker also owns one slice buffer and triangle-edge lists.
 * A mutex serializes output-slice writes because generic DataStore writes are not concurrent.
 */
class SIMPLNXCORE_EXPORT RegularGridSampleSurfaceMesh
{
public:
  /**
   * @brief Creates a regular-grid surface sampler.
   * @param dataStructure Provides source mesh and output ImageGeom arrays.
   * @param mesgHandler Receives phase messages.
   * @param shouldCancel Stops later preprocessing or worker scheduling when true.
   * @param inputValues Specifies validated settings and paths. The caller must keep
   * this object alive for the sampler lifetime.
   */
  RegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RegularGridSampleSurfaceMeshInputValues* inputValues);
  /**
   * @brief Destroys the non-owning sampler.
   */
  ~RegularGridSampleSurfaceMesh() noexcept;

  RegularGridSampleSurfaceMesh(const RegularGridSampleSurfaceMesh&) = delete;
  RegularGridSampleSurfaceMesh(RegularGridSampleSurfaceMesh&&) noexcept = delete;
  RegularGridSampleSurfaceMesh& operator=(const RegularGridSampleSurfaceMesh&) = delete;
  RegularGridSampleSurfaceMesh& operator=(RegularGridSampleSurfaceMesh&&) noexcept = delete;

  /**
   * @brief Rasterizes all scheduled Z slices.
   * @return Success after scheduled workers finish.
   *
   * Input and output bulk-I/O errors are not inspected. Cancellation stops new
   * worker scheduling, but scheduled workers finish and can write output slices.
   */
  Result<> operator()();

  /**
   * @brief Writes one completed Z-slice while holding the output mutex.
   * @tparam T Specifies the Feature-ID scalar type.
   * @param zSlice Specifies the destination Z index.
   * @param sliceData Provides rasterized Feature IDs.
   * @param count Specifies values in the slice buffer.
   * @pre operator() initialized the slice size and output path.
   *
   * The method does not inspect the copyFromBuffer() result.
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
