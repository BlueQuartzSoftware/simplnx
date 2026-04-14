#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct KMedoidsInputValues;

/**
 * @class ComputeKMedoidsDirect
 * @brief In-core algorithm for K-Medoids clustering using direct per-element array access.
 *
 * Uses operator[] for distance computation, cluster assignment, and medoid optimization.
 * This is optimal when all arrays reside in memory, where operator[] is essentially a
 * pointer dereference.
 *
 * The algorithm uses Voronoi iteration:
 *   1. Randomly select k initial medoids from masked data points
 *   2. Assign each point to the nearest medoid (findClusters)
 *   3. For each cluster, find the member that minimizes total intra-cluster distance (optimizeClusters)
 *   4. Repeat steps 2-3 until medoids stop changing
 *
 * Selected by DispatchAlgorithm when all input arrays are backed by in-memory DataStore.
 *
 * @see ComputeKMedoidsScanline for the out-of-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT ComputeKMedoidsDirect
{
public:
  /**
   * @brief Constructs the in-core algorithm with all resources it needs.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  ComputeKMedoidsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const KMedoidsInputValues* inputValues);
  ~ComputeKMedoidsDirect() noexcept;

  ComputeKMedoidsDirect(const ComputeKMedoidsDirect&) = delete;
  ComputeKMedoidsDirect(ComputeKMedoidsDirect&&) noexcept = delete;
  ComputeKMedoidsDirect& operator=(const ComputeKMedoidsDirect&) = delete;
  ComputeKMedoidsDirect& operator=(ComputeKMedoidsDirect&&) noexcept = delete;

  /**
   * @brief Executes the in-core K-Medoids clustering.
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
