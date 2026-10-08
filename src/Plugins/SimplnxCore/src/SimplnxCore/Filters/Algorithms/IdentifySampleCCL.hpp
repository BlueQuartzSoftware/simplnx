#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

class IDataArray;
struct IdentifySampleInputValues;

#if SIMPLNX_BUILD_TESTS
/**
 * @enum IdentifySampleSliceEventForTesting
 * @brief Names actual slice-copy and carrier-lifetime observations.
 * @note Grant, batch, and extra-lifetime events set success true. A zero-byte grant is a valid observation.
 */
enum class IdentifySampleSliceEventForTesting
{
  ReadBegin,               ///< Starts one logical bulk read.
  ReadEnd,                 ///< Ends the read, including error or exception exit.
  WriteBegin,              ///< Starts one logical bulk write.
  WriteEnd,                ///< Ends the write, including error or exception exit.
  PlaneAllocated,          ///< The original plane carrier has allocated its values.
  PlaneReleased,           ///< The original plane carrier has destroyed its values.
  XyAllocated,             ///< The original XY carrier has allocated its values.
  XyReleased,              ///< The original XY carrier has destroyed its values.
  ExtraPlanesAllocated,    ///< Owns extra values; values counts elements of elementBytes bytes.
  ExtraPlanesReleased,     ///< Destroys extra values before releasing their reservation; units match allocation.
  ExtraBytesRequested,     ///< Requests extra bytes; values is bytes and elementBytes is one.
  ExtraBytesGranted,       ///< Receives actual reserved bytes; values is bytes and elementBytes is one.
  ExtraBytesRetained,      ///< Retains whole-plane bytes after shrinking; values is bytes and elementBytes is one.
  BatchWidth,              ///< Selects an actual batch; values is planes and elementBytes is one.
  ExtraReservationReleased ///< Releases the retained token after extra values; values is bytes and elementBytes is one.
};

/**
 * @struct IdentifySampleSliceObserverForTesting
 * @brief Borrows a synchronous callback on the filter execution thread.
 * @note The callback must not allocate, throw, or reenter storage. Borrowed mask identity lasts through the callback.
 */
struct SIMPLNXCORE_EXPORT IdentifySampleSliceObserverForTesting
{
  void* context = nullptr;
  void (*callback)(void*, const IDataArray*, IdentifySampleSliceEventForTesting, usize values, usize elementBytes, bool success) noexcept = nullptr;
};

/**
 * @brief Replaces this thread's slice observer.
 * @param observer Supplies the callback and its borrowed context.
 * @return Previous observer for scoped restoration.
 * @pre No observed call or carrier owner is live on this thread.
 */
SIMPLNXCORE_EXPORT IdentifySampleSliceObserverForTesting SetIdentifySampleSliceObserverForTesting(IdentifySampleSliceObserverForTesting observer) noexcept;

/**
 * @struct IdentifySampleExtraAllocationControlForTesting
 * @brief Borrows a fault callback immediately before a nonzero extra-plane allocation.
 * @note The callback can throw. It does not change admission or report successful allocation.
 * @note The borrowed context and mask identity remain valid through the synchronous callback.
 */
struct SIMPLNXCORE_EXPORT IdentifySampleExtraAllocationControlForTesting
{
  void* context = nullptr;
  void (*beforeAllocate)(void*, const IDataArray*, usize extraValues) = nullptr;
};

/**
 * @brief Replaces this thread's extra-allocation control.
 * @param control Supplies the callback and borrowed context.
 * @return Previous control for scoped restoration.
 * @pre No slice execution or extra owner is live on this thread.
 */
SIMPLNXCORE_EXPORT IdentifySampleExtraAllocationControlForTesting SetIdentifySampleExtraAllocationControlForTesting(IdentifySampleExtraAllocationControlForTesting control) noexcept;

/**
 * @struct IdentifySampleDiagnosticControlForTesting
 * @brief Borrows a fault callback before guarded dual-failure diagnostic construction.
 * @note The callback can throw. Both original failure carriers remain intact at this boundary.
 * @note The borrowed context and mask identity remain valid through the synchronous callback.
 */
struct SIMPLNXCORE_EXPORT IdentifySampleDiagnosticControlForTesting
{
  void* context = nullptr;
  void (*beforeAggregate)(void*, const IDataArray*) = nullptr;
};

/**
 * @brief Replaces this thread's diagnostic control.
 * @param control Supplies the callback and borrowed context.
 * @return Previous control for scoped restoration.
 * @pre No slice execution is live on this thread.
 */
SIMPLNXCORE_EXPORT IdentifySampleDiagnosticControlForTesting SetIdentifySampleDiagnosticControlForTesting(IdentifySampleDiagnosticControlForTesting control) noexcept;
#endif

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
 * two label rows, and external equivalence records. YZ mode also keeps one
 * Z-slice buffer and admits up to seven extra planes through a working-memory
 * reservation. Only completed planes are published after a batch stops.
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
