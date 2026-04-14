#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

namespace nx::core
{
/**
 * @struct KMedoidsInputValues
 * @brief Input parameter bundle for the ComputeKMedoids algorithm.
 *
 * Aggregates all DataPaths and configuration values needed by both the in-core
 * (Direct) and out-of-core (Scanline) variants of K-Medoids clustering.
 */
struct SIMPLNXCORE_EXPORT KMedoidsInputValues
{
  uint64 InitClusters;                             ///< Number of clusters (k) to partition the data into
  ClusterUtilities::DistanceMetric DistanceMetric; ///< Distance metric used for cluster assignment and medoid optimization
  DataPath ClusteringArrayPath;                    ///< Input array containing the data to be clustered (any numeric type)
  DataPath MaskArrayPath;                          ///< Input Bool/UInt8 mask array; false elements are assigned to cluster 0
  DataPath FeatureIdsArrayPath;                    ///< Output Int32 array storing per-element cluster assignments
  DataPath MedoidsArrayPath;                       ///< Output array storing the medoid (representative point) for each cluster
  uint64 Seed;                                     ///< Random seed for reproducible initial medoid selection
};

/**
 * @class ComputeKMedoids
 * @brief Dispatcher algorithm for K-Medoids clustering.
 *
 * K-Medoids is a partitioning clustering algorithm that assigns each data point to the
 * nearest medoid (an actual data point that minimizes intra-cluster distance), then
 * iteratively updates medoids until convergence.
 *
 * This class acts as a thin dispatcher that selects between two concrete implementations:
 *
 * - **ComputeKMedoidsDirect** (in-core): Uses per-element operator[] access for distance
 *   computation and cluster assignment. Optimal when all arrays reside in memory.
 *
 * - **ComputeKMedoidsScanline** (out-of-core / OOC): Uses chunked copyIntoBuffer() /
 *   copyFromBuffer() bulk I/O to read input data and write cluster assignments in
 *   fixed-size chunks (64K tuples), avoiding random per-element OOC access.
 *
 * The dispatch decision is made by DispatchAlgorithm<Direct, Scanline>() in
 * AlgorithmDispatch.hpp, which checks whether any input IDataArray uses OOC storage.
 *
 * **Why two variants exist**: K-Medoids requires computing pairwise distances between
 * data points and medoids. When data is stored out-of-core, each operator[] access may
 * trigger a chunk load/evict cycle. The Scanline variant caches medoids locally and
 * processes the input array in sequential 64K-tuple chunks, converting random access
 * into sequential bulk reads.
 *
 * @see ComputeKMedoidsDirect
 * @see ComputeKMedoidsScanline
 * @see AlgorithmDispatch.hpp
 */
class SIMPLNXCORE_EXPORT ComputeKMedoids
{
public:
  /**
   * @brief Constructs the dispatcher with all resources needed by either algorithm variant.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  ComputeKMedoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMedoidsInputValues* inputValues);
  ~ComputeKMedoids() noexcept;

  ComputeKMedoids(const ComputeKMedoids&) = delete;
  ComputeKMedoids(ComputeKMedoids&&) noexcept = delete;
  ComputeKMedoids& operator=(const ComputeKMedoids&) = delete;
  ComputeKMedoids& operator=(ComputeKMedoids&&) noexcept = delete;

  /**
   * @brief Dispatches to the Direct or Scanline algorithm based on storage type.
   * @return Result<> with any errors encountered during execution
   */
  Result<> operator()();

  /**
   * @brief Sends a progress message through the filter's message handler.
   * @param message The progress message text
   */
  void updateProgress(const std::string& message);

  /**
   * @brief Returns a reference to the cancellation flag for checking in inner loops.
   * @return Reference to the atomic bool cancellation flag
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;                     ///< Reference to the DataStructure containing all arrays
  const KMedoidsInputValues* m_InputValues = nullptr; ///< Non-owning pointer to input parameters
  const std::atomic_bool& m_ShouldCancel;             ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;    ///< Message handler for progress updates
};

} // namespace nx::core
