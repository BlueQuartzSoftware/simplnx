#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeBoundaryCellsInputValues;

/**
 * @class ComputeBoundaryCellsScanline
 * @brief Counts boundary faces with a three-slice rolling window.
 *
 * Previous, current, and next Feature ID slices provide all six face neighbors.
 * The algorithm reads each input slice once and writes one completed output slice.
 * Staging memory is 13 bytes per XY cell: three Int32 slices and one Int8 slice.
 *
 * The current implementation does not inspect input or output bulk-I/O Result
 * values. Cancellation returns success with prior output slices preserved.
 */
class SIMPLNXCORE_EXPORT ComputeBoundaryCellsScanline
{
public:
  /**
   * @brief Initializes scanline boundary counting.
   * @param dataStructure Provides geometry, input, and output arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between slices.
   * @param inputValues Defines paths and counting policies.
   * @pre All arguments outlive this executor.
   */
  ComputeBoundaryCellsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCellsScanline() noexcept;

  ComputeBoundaryCellsScanline(const ComputeBoundaryCellsScanline&) = delete;
  ComputeBoundaryCellsScanline(ComputeBoundaryCellsScanline&&) noexcept = delete;
  ComputeBoundaryCellsScanline& operator=(const ComputeBoundaryCellsScanline&) = delete;
  ComputeBoundaryCellsScanline& operator=(ComputeBoundaryCellsScanline&&) noexcept = delete;

  /**
   * @brief Counts boundary faces through sequential slice transfers.
   * @return Success. Bulk-I/O failures are not returned.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryCellsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
