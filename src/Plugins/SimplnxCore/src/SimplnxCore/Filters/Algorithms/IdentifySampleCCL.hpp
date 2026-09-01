#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct IdentifySampleInputValues;

/**
 * @class IdentifySampleCCL
 * @brief Uses scanline connected-component labeling (CCL) for sequential access.
 *
 * The full-volume path scans in Z-Y-X order and keeps two label slices. It uses
 * deterministic replay instead of a volume-sized label array. Replay costs
 * additional sequential reads but prevents random neighbor reads from a
 * disk-backed mask.
 *
 * External equivalence and boundary records can contain O(N) entries. A genuine
 * out-of-core provider stores these records on disk and keeps a bounded page
 * cache in memory. A resident path, including a forced CCL path, permits an
 * in-memory fallback that can allocate O(N) scratch.
 *
 * Equal-sized components favor the largest provisional root label. Slice mode
 * uses a separate row-streaming CCL implementation. It keeps one plane buffer,
 * one Z-slice buffer, two label rows, and external equivalence records.
 *
 * Cancellation can stop between scan or replay units and return success. The
 * operation does not restore slices that a prior replay changed. Bulk-I/O and
 * temporary-record errors are returned.
 */
class SIMPLNXCORE_EXPORT IdentifySampleCCL
{
public:
  /**
   * @brief Initializes the sequential CCL implementation.
   * @param dataStructure Contains the ImageGeom and mask.
   * @param mesgHandler Receives slice messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects hole and slice behavior.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  IdentifySampleCCL(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const IdentifySampleInputValues* inputValues);
  ~IdentifySampleCCL() noexcept;

  IdentifySampleCCL(const IdentifySampleCCL&) = delete;
  IdentifySampleCCL(IdentifySampleCCL&&) noexcept = delete;
  IdentifySampleCCL& operator=(const IdentifySampleCCL&) = delete;
  IdentifySampleCCL& operator=(IdentifySampleCCL&&) noexcept = delete;

  /**
   * @brief Retains the largest component and optionally fills holes.
   * @return Bulk-I/O, temporary-record, or equivalence result.
   * @pre The mask is scalar Bool or UInt8 and matches ImageGeom cell dimensions.
   * @pre SliceBySlicePlaneIndex identifies XY, XZ, or YZ.
   *
   * Cancellation can return success with a partially modified mask.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IdentifySampleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
