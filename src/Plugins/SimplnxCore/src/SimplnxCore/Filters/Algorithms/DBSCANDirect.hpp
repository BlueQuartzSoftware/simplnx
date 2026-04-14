#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct DBSCANInputValues;

/**
 * @class DBSCANDirect
 * @brief In-core algorithm for grid-based DBSCAN using direct per-element array access.
 *
 * Uses operator[] for all data access phases:
 *   - Grid construction: reads each tuple's coordinates to compute bounds and bin assignments
 *   - Distance computation (canMerge): directly indexes into the input array by tuple index
 *     for pairwise distance checks between grid cell members
 *   - Labeling: writes cluster IDs via setValue()
 *
 * This is optimal when all arrays reside in memory, where operator[] is essentially a
 * pointer dereference. For out-of-core data, each operator[] call may trigger chunk
 * load/evict cycles, so DBSCANScanline should be used instead.
 *
 * Selected by DispatchAlgorithm when all input arrays are backed by in-memory DataStore.
 *
 * @see DBSCANScanline for the out-of-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT DBSCANDirect
{
public:
  /**
   * @brief Constructs the in-core algorithm with all resources it needs.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  DBSCANDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const DBSCANInputValues* inputValues);
  ~DBSCANDirect() noexcept;

  DBSCANDirect(const DBSCANDirect&) = delete;
  DBSCANDirect(DBSCANDirect&&) noexcept = delete;
  DBSCANDirect& operator=(const DBSCANDirect&) = delete;
  DBSCANDirect& operator=(DBSCANDirect&&) noexcept = delete;

  /**
   * @brief Executes the in-core DBSCAN clustering: grid construction, clustering, labeling.
   * @return Result<> with any errors encountered during execution
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                   ///< Reference to the DataStructure containing all arrays
  const DBSCANInputValues* m_InputValues = nullptr; ///< Non-owning pointer to input parameters
  const std::atomic_bool& m_ShouldCancel;           ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;  ///< Message handler for progress updates
};

} // namespace nx::core
