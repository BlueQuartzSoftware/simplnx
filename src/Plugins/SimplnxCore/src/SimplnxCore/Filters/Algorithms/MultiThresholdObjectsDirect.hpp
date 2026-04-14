#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct MultiThresholdObjectsInputValues;

/**
 * @class MultiThresholdObjectsDirect
 * @brief In-core algorithm for multi-threshold filtering using direct per-element array access.
 *
 * For each threshold condition, reads the input array via getComponentValue() and writes
 * TRUE/FALSE into a temporary O(n) result vector. After evaluating a condition, the
 * temporary results are merged into the output mask array using AND/OR logic.
 *
 * This is optimal when all arrays reside in memory, where getComponentValue() and
 * operator[] are essentially pointer dereferences.
 *
 * Selected by DispatchAlgorithm when all input arrays are backed by in-memory DataStore.
 *
 * @see MultiThresholdObjectsScanline for the out-of-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT MultiThresholdObjectsDirect
{
public:
  /**
   * @brief Constructs the in-core algorithm with all resources it needs.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  MultiThresholdObjectsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const MultiThresholdObjectsInputValues* inputValues);
  ~MultiThresholdObjectsDirect() noexcept;

  MultiThresholdObjectsDirect(const MultiThresholdObjectsDirect&) = delete;
  MultiThresholdObjectsDirect(MultiThresholdObjectsDirect&&) noexcept = delete;
  MultiThresholdObjectsDirect& operator=(const MultiThresholdObjectsDirect&) = delete;
  MultiThresholdObjectsDirect& operator=(MultiThresholdObjectsDirect&&) noexcept = delete;

  /**
   * @brief Executes the in-core multi-threshold filtering.
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
