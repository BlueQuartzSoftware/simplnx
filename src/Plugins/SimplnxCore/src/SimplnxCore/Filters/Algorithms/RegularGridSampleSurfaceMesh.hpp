#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <mutex>
#include <type_traits>
#include <vector>

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
   * @return The first input or output store error.
   *
   * Cancellation stops new worker scheduling. Scheduled workers finish unless another worker reports an error.
   */
  Result<> operator()();

  /**
   * @brief Writes one completed Z-slice while holding the output mutex.
   * @tparam T Specifies the source Face Label scalar type.
   * @param zSlice Specifies the destination Z index.
   * @param sliceData Provides rasterized Feature IDs.
   * @param count Specifies values in the slice buffer.
   * @return The output-store write result.
   * @pre operator() initialized the slice size and output path and validated the label range.
   *
   */
  template <typename T>
  Result<> sendThreadSafeSliceUpdate(usize zSlice, const T* sliceData, usize count)
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto& featureIdsRef = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
    const usize offset = zSlice * m_CellsPerSlice;
    return ExecuteDataFunctionIntType(
        [&]<typename OutputT>() -> Result<> {
          auto& outputStore = dynamic_cast<DataArray<OutputT>&>(featureIdsRef).getDataStoreRef();
          if constexpr(std::is_same_v<T, OutputT>)
          {
            return outputStore.copyFromBuffer(offset, nonstd::span<const OutputT>(sliceData, count));
          }
          else
          {
            // Only custom output types need a converted slice buffer.
            std::vector<OutputT> convertedSlice(count);
            std::transform(sliceData, sliceData + count, convertedSlice.begin(), [](T label) { return static_cast<OutputT>(label); });
            return outputStore.copyFromBuffer(offset, nonstd::span<const OutputT>(convertedSlice.data(), count));
          }
        },
        featureIdsRef.getDataType());
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
