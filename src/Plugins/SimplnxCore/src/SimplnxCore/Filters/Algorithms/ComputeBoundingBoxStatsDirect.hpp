#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeBoundingBoxStatsInputValues;

/**
 * @class ComputeBoundingBoxStatsDirect
 * @brief Computes bounding-box statistics with direct element access.
 *
 * Contiguous in-memory input permits parallel reads. A forced direct path uses
 * serial abstract-store reads for non-contiguous input because generic
 * DataStore access does not guarantee thread safety. Framework output stores
 * are populated serially after the workers join.
 */
class SIMPLNXCORE_EXPORT ComputeBoundingBoxStatsDirect
{
public:
  /**
   * @brief Initializes direct bounding-box statistics.
   * @param dataStructure Contains the geometry, arrays, and outputs.
   * @param mesgHandler Receives phase announcements and throttled progress.
   * @param shouldCancel Signals cancellation between bounded scan operations.
   * @param inputValues Selects statistics and identifies required paths.
   * @pre All arguments outlive this executor.
   */
  ComputeBoundingBoxStatsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeBoundingBoxStatsInputValues* inputValues);
  ~ComputeBoundingBoxStatsDirect() noexcept;

  ComputeBoundingBoxStatsDirect(const ComputeBoundingBoxStatsDirect&) = delete;
  ComputeBoundingBoxStatsDirect(ComputeBoundingBoxStatsDirect&&) noexcept = delete;
  ComputeBoundingBoxStatsDirect& operator=(const ComputeBoundingBoxStatsDirect&) = delete;
  ComputeBoundingBoxStatsDirect& operator=(ComputeBoundingBoxStatsDirect&&) noexcept = delete;

  /**
   * @brief Computes the selected statistics with direct element access.
   * @return Success, or an input or output-store error.
   *
   * Cancellation returns success. Workers check the flag between bounded scan
   * blocks. Staged results are not published after a cancelled worker pass;
   * outputs from an earlier completed pass can remain.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundingBoxStatsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
