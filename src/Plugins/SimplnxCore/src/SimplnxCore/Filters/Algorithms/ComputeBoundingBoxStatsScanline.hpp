#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeBoundingBoxStatsInputValues;

/**
 * @class ComputeBoundingBoxStatsScanline
 * @brief Computes bounding-box statistics with bounded bulk I/O.
 *
 * Contiguous row reads avoid per-voxel store access. Frequency statistics use
 * external merge sorting to match the direct path without cell-sized RAM
 * scratch. The sort uses two temporary files that can each approach the byte
 * size of the largest bounding box. The files are reused between boxes.
 */
class SIMPLNXCORE_EXPORT ComputeBoundingBoxStatsScanline
{
public:
  /**
   * @brief Initializes scanline bounding-box statistics.
   * @param dataStructure Contains the geometry, arrays, and outputs.
   * @param mesgHandler Supplies the common interface. This path emits no messages.
   * @param shouldCancel Signals cancellation between bounded operations.
   * @param inputValues Selects statistics and identifies required paths.
   * @pre All arguments outlive this executor.
   */
  ComputeBoundingBoxStatsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  const ComputeBoundingBoxStatsInputValues* inputValues);
  ~ComputeBoundingBoxStatsScanline() noexcept;

  ComputeBoundingBoxStatsScanline(const ComputeBoundingBoxStatsScanline&) = delete;
  ComputeBoundingBoxStatsScanline(ComputeBoundingBoxStatsScanline&&) noexcept = delete;
  ComputeBoundingBoxStatsScanline& operator=(const ComputeBoundingBoxStatsScanline&) = delete;
  ComputeBoundingBoxStatsScanline& operator=(ComputeBoundingBoxStatsScanline&&) noexcept = delete;

  /**
   * @brief Computes the selected statistics with serial scanline reads.
   * @return Success, or an input, storage, or temporary-file error.
   *
   * Cancellation returns success. Mode entries for earlier frequency groups
   * and standard deviations written before cancellation remain in their outputs.
   * Other statistics are written only after all requested passes complete.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundingBoxStatsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
