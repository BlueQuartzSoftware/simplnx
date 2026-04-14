#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct MultiThresholdObjectsInputValues;

/**
 * @class MultiThresholdObjectsScanline
 * @brief Out-of-core algorithm for multi-threshold filtering using chunked bulk I/O.
 *
 * Instead of the Direct variant's per-element access and O(n) temporary result vector,
 * this variant processes data in fixed-size 64K-tuple chunks:
 *
 * For each threshold condition and each chunk:
 *   1. Read a chunk of the input array via copyIntoBuffer() (sequential bulk read)
 *   2. Apply the comparison operator to produce a chunk-sized temporary result buffer
 *   3. For the first condition: write the temp buffer directly to the output mask
 *      via copyFromBuffer()
 *   4. For subsequent conditions: read the current output chunk via copyIntoBuffer(),
 *      merge using AND/OR logic, then write back via copyFromBuffer()
 *
 * This approach has two advantages for OOC data:
 *   - All input array reads use sequential bulk I/O instead of per-element access
 *   - Peak memory per threshold condition is O(chunkSize) instead of O(n)
 *
 * The temporary buffers use std::unique_ptr<T[]> instead of std::vector<T> to avoid
 * the std::vector<bool> specialization that would prevent direct memory access.
 *
 * Selected by DispatchAlgorithm when any input array is backed by out-of-core storage.
 *
 * @see MultiThresholdObjectsDirect for the in-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT MultiThresholdObjectsScanline
{
public:
  /**
   * @brief Constructs the out-of-core algorithm with all resources it needs.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  MultiThresholdObjectsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const MultiThresholdObjectsInputValues* inputValues);
  ~MultiThresholdObjectsScanline() noexcept;

  MultiThresholdObjectsScanline(const MultiThresholdObjectsScanline&) = delete;
  MultiThresholdObjectsScanline(MultiThresholdObjectsScanline&&) noexcept = delete;
  MultiThresholdObjectsScanline& operator=(const MultiThresholdObjectsScanline&) = delete;
  MultiThresholdObjectsScanline& operator=(MultiThresholdObjectsScanline&&) noexcept = delete;

  /**
   * @brief Executes the OOC-optimized multi-threshold filtering.
   * @return Result<> with any errors encountered during execution
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                  ///< Reference to the DataStructure containing all arrays
  const MultiThresholdObjectsInputValues* m_InputValues = nullptr; ///< Non-owning pointer to input parameters
  const std::atomic_bool& m_ShouldCancel;                          ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;                 ///< Message handler for progress updates
};

} // namespace nx::core
