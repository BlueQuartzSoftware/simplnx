#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeFeatureNeighborsInputValues
 * @brief Input parameter bundle for the ComputeFeatureNeighbors algorithm.
 *
 * Aggregates all DataPaths and boolean flags needed by both the in-core (Direct)
 * and out-of-core (Scanline) variants of the feature neighbor computation.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureNeighborsInputValues
{
  DataPath BoundaryCellsPath;                                        ///< Output Int8 array marking how many different-feature face neighbors each cell has
  AttributeMatrixSelectionParameter::ValueType CellFeatureArrayPath; ///< Attribute matrix where per-feature output arrays reside
  ArraySelectionParameter::ValueType FeatureIdsPath;                 ///< Input Int32 array of per-cell feature IDs
  GeometrySelectionParameter::ValueType InputImageGeometryPath;      ///< Input ImageGeom providing dimensions and spacing
  DataPath NeighborListPath;                                         ///< Output Int32 NeighborList storing each feature's neighbor IDs
  DataPath NumberOfNeighborsPath;                                    ///< Output Int32 array storing the count of neighbors per feature
  DataPath SharedSurfaceAreaListPath;                                ///< Output Float32 NeighborList storing shared surface area per neighbor pair
  BoolParameter::ValueType StoreBoundaryCells;                       ///< Whether to compute and store the BoundaryCells array
  BoolParameter::ValueType StoreSurfaceFeatures;                     ///< Whether to compute and store the SurfaceFeatures array
  DataPath SurfaceFeaturesPath;                                      ///< Output Bool array marking features that touch the geometry boundary
};

/**
 * @class ComputeFeatureNeighbors
 * @brief Dispatcher algorithm for computing feature neighbor lists and shared surface areas
 * on an ImageGeom.
 *
 * This class acts as a thin dispatcher that selects between two concrete algorithm
 * implementations at runtime:
 *
 * - **ComputeFeatureNeighborsDirect** (in-core): Uses per-element getValue() access with
 *   compile-time dimension specialization. Optimal when all arrays reside in memory.
 *
 * - **ComputeFeatureNeighborsScanline** (out-of-core / OOC): Uses a Z-slice rolling
 *   window with bulk copyIntoBuffer()/copyFromBuffer() I/O. Avoids random disk access
 *   when arrays are backed by chunked on-disk storage (e.g., Zarr/HDF5 chunks).
 *
 * The dispatch decision is made by DispatchAlgorithm<Direct, Scanline>() in
 * AlgorithmDispatch.hpp, which checks whether any input IDataArray uses OOC storage.
 *
 * **Why two variants exist**: When data is stored out-of-core in compressed disk chunks,
 * each random-access getValue() call may trigger a chunk load from disk, use one value,
 * then evict the chunk. For a 3D image with millions of voxels, this "chunk thrashing"
 * makes the algorithm 100-1000x slower. The Scanline variant reads entire Z-slices
 * sequentially, keeping a rolling window of 2-3 slices in memory so that all 6 face
 * neighbors can be resolved from in-memory buffers.
 *
 * @see ComputeFeatureNeighborsDirect
 * @see ComputeFeatureNeighborsScanline
 * @see AlgorithmDispatch.hpp
 */
class SIMPLNXCORE_EXPORT ComputeFeatureNeighbors
{
public:
  /**
   * @brief Constructs the dispatcher with all resources needed by either algorithm variant.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  ComputeFeatureNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureNeighborsInputValues* inputValues);
  ~ComputeFeatureNeighbors() noexcept;

  ComputeFeatureNeighbors(const ComputeFeatureNeighbors&) = delete;
  ComputeFeatureNeighbors(ComputeFeatureNeighbors&&) noexcept = delete;
  ComputeFeatureNeighbors& operator=(const ComputeFeatureNeighbors&) = delete;
  ComputeFeatureNeighbors& operator=(ComputeFeatureNeighbors&&) noexcept = delete;

  /**
   * @brief Dispatches to the Direct or Scanline algorithm based on storage type.
   * @return Result<> with any errors encountered during execution
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                    ///< Reference to the DataStructure containing all arrays
  const ComputeFeatureNeighborsInputValues* m_InputValues = nullptr; ///< Non-owning pointer to input parameters
  const std::atomic_bool& m_ShouldCancel;                            ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;                   ///< Message handler for progress updates
};

} // namespace nx::core
