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
   * @param mesgHandler Supplies the common interface. This path emits no messages.
   * @param shouldCancel Supplies the common algorithm interface.
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
   * @warning This implementation does not inspect the cancellation flag.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundingBoxStatsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
