#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeFeatureCentroidsInputValues
 * @brief Holds all user-configured parameters for the ComputeFeatureCentroids algorithm.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureCentroidsInputValues
{
  DataPath FeatureIdsArrayPath;        ///< Path to the per-cell Feature ID array (int32).
  DataPath CentroidsArrayPath;         ///< Output: per-feature centroid array (float32, 3-component).
  DataPath ImageGeometryPath;          ///< Path to the ImageGeom providing voxel coordinates.
  DataPath FeatureAttributeMatrixPath; ///< Path to the Feature-level Attribute Matrix.
  bool IsPeriodic = false;             ///< If true, adjust centroids for features wrapping around periodic boundaries.
};

/**
 * @class ComputeFeatureCentroids
 * @brief Computes the centroid (average XYZ position) of each feature in an
 * ImageGeom by iterating over all voxels and accumulating coordinates using
 * Kahan summation for numerical stability.
 *
 * @section ooc_optimization Out-of-Core Optimization
 * The original implementation used a ParallelDataAlgorithm that accessed the
 * FeatureIds array element-by-element through AbstractDataStore virtual dispatch.
 * For OOC data this caused a chunk load/evict cycle per voxel.
 *
 * The optimized implementation reads the FeatureIds array in fixed-size chunks
 * (64K tuples) via copyIntoBuffer() into a stack-allocated buffer, then processes
 * each chunk purely from local memory. All accumulation arrays (Kahan sums,
 * compensators, voxel counts, XYZ ranges) are plain std::vectors rather than
 * DataStore-backed arrays, eliminating virtual dispatch in the hot loop. The
 * final centroids are written back to the output DataStore in a single
 * copyFromBuffer() call.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureCentroids
{
public:
  ComputeFeatureCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureCentroidsInputValues* inputValues);
  ~ComputeFeatureCentroids() noexcept;

  ComputeFeatureCentroids(const ComputeFeatureCentroids&) = delete;
  ComputeFeatureCentroids(ComputeFeatureCentroids&&) noexcept = delete;
  ComputeFeatureCentroids& operator=(const ComputeFeatureCentroids&) = delete;
  ComputeFeatureCentroids& operator=(ComputeFeatureCentroids&&) noexcept = delete;

  /**
   * @brief Executes the centroid computation over all voxels using chunked bulk I/O.
   * @return Result<> indicating success or error.
   */
  Result<> operator()();

  /**
   * @brief Returns the cancellation flag reference.
   * @return const reference to the atomic cancellation boolean.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;                                    ///< Reference to the DataStructure.
  const ComputeFeatureCentroidsInputValues* m_InputValues = nullptr; ///< User-configured parameters.
  const std::atomic_bool& m_ShouldCancel;                            ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                   ///< Message handler for progress.
};

} // namespace nx::core
