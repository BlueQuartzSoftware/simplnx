#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct KMedoidsInputValues;

/**
 * @class ComputeKMedoidsScanline
 * @brief Out-of-core algorithm for K-Medoids clustering using chunked bulk I/O.
 *
 * Uses copyIntoBuffer()/copyFromBuffer() to read input data and write cluster
 * assignments in fixed-size chunks (64K tuples), avoiding random per-element
 * OOC access that would cause chunk thrashing.
 *
 * Key OOC optimizations over the Direct variant:
 *
 * - **Medoid initialization**: Uses copyIntoBuffer()/copyFromBuffer() per-tuple
 *   instead of operator[] to read initial medoid values.
 *
 * - **Cluster assignment (findClusters)**: Caches all medoids in a local vector
 *   (small: k * numComponents), then processes the input array and featureIds
 *   in aligned 64K-tuple chunks via bulk I/O. Each chunk is read once, all
 *   distance computations for that chunk are done in memory, then featureIds
 *   are written back in one bulk operation.
 *
 * - **Medoid optimization (optimizeClusters)**: Processes one cluster at a time.
 *   Scans featureIds in chunks to build a member index list, then computes
 *   pairwise distances using per-tuple copyIntoBuffer() reads. Peak memory is
 *   O(max_cluster_size), not O(n).
 *
 * Selected by DispatchAlgorithm when any input array is backed by out-of-core storage.
 *
 * @see ComputeKMedoidsDirect for the in-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT ComputeKMedoidsScanline
{
public:
  /**
   * @brief Constructs the out-of-core algorithm with all resources it needs.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  ComputeKMedoidsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const KMedoidsInputValues* inputValues);
  ~ComputeKMedoidsScanline() noexcept;

  ComputeKMedoidsScanline(const ComputeKMedoidsScanline&) = delete;
  ComputeKMedoidsScanline(ComputeKMedoidsScanline&&) noexcept = delete;
  ComputeKMedoidsScanline& operator=(const ComputeKMedoidsScanline&) = delete;
  ComputeKMedoidsScanline& operator=(ComputeKMedoidsScanline&&) noexcept = delete;

  /**
   * @brief Executes the OOC-optimized K-Medoids clustering.
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
