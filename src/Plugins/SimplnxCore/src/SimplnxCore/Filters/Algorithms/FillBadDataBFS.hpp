#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct FillBadDataInputValues;

/**
 * @class FillBadDataBFS
 * @brief Classifies and fills defects with resident BFS state.
 *
 * BFS discovers face-connected Feature ID zero regions. Regions below the size
 * threshold become -1 and fill one boundary layer per iteration. A majority
 * vote selects a positive neighbor. Ties keep the first neighbor in -Z, -Y,
 * -X, +X, +Y, +Z order. Sibling arrays update before Feature IDs.
 *
 * The visited bitmap and int64 source map scale with cell count. The active BFS
 * queue scales with component size. Vote state scales with maximum Feature ID.
 * Random resident access makes this path unsuitable for OOC stores.
 *
 * The BFS seed is not marked when first enqueued. A back-edge can add it a
 * second time, so a multi-cell defect's measured size can exceed its unique
 * voxel count by one.
 */
class SIMPLNXCORE_EXPORT FillBadDataBFS
{
public:
  /**
   * @brief Initializes resident BFS filling.
   * @param dataStructure Contains geometry and cell arrays.
   * @param mesgHandler Receives unfillable-region warnings.
   * @param shouldCancel Signals cancellation between components and fill iterations.
   * @param inputValues Selects defect size, phase behavior, and ignored arrays.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  FillBadDataBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues);
  ~FillBadDataBFS() noexcept;

  FillBadDataBFS(const FillBadDataBFS&) = delete;
  FillBadDataBFS(FillBadDataBFS&&) noexcept = delete;
  FillBadDataBFS& operator=(const FillBadDataBFS&) = delete;
  FillBadDataBFS& operator=(FillBadDataBFS&&) noexcept = delete;

  /**
   * @brief Classifies defects and fills small regions in place.
   * @return Success.
   * @pre Image dimensions and cell-array tuple counts agree and are nonzero.
   * @pre The defect threshold is nonnegative.
   * @pre Cell phases are nonnegative when new-phase storage is enabled.
   * @pre Maximum Feature ID and maximum phase plus one fit their output types.
   *
   * Cancellation is not checked inside a BFS component, a vote scan, or an
   * array-copy pass. It returns success at the next checkpoint and leaves prior
   * mutations. An unfillable region emits a warning and leaves negative Feature
   * IDs while returning success.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
