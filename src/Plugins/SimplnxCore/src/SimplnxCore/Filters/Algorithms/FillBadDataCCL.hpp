#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ExternalEquivalence.hpp"

namespace nx::core
{

// Forward declarations
template <typename T>
class DataArray;
using Int32Array = DataArray<int32>;

template <typename T>
class AbstractDataStore;
using Int32AbstractDataStore = AbstractDataStore<int32>;

struct FillBadDataInputValues;

/**
 * @class FillBadDataCCL
 * @brief Classifies and fills defects with scanline CCL and external records.
 *
 * A forward Z-Y-X scan checks only -X, -Y, and -Z neighbors. It writes
 * provisional labels into FeatureIds and stores label equivalence externally.
 * Region classification resolves roots lazily and changes small regions to -1.
 * Large regions return to zero and can receive a new phase.
 *
 * Each fill iteration records destination/source pairs in Z-Y-X order. Replays
 * use three-slice array windows. Feature IDs replay before sibling arrays because
 * source decisions are frozen in the pair records. This order makes cancellation
 * between replays capable of producing inconsistent tuple state.
 *
 * Genuine OOC execution uses provider-backed equivalence and pair records.
 * Temporary record capacity scales with cell count plus maximum Feature ID.
 * RAM uses slice windows, bounded record caches, and a vote array sized by
 * maximum Feature ID. A forced resident CCL path can use similarly scaled
 * in-memory record fallbacks.
 */
class SIMPLNXCORE_EXPORT FillBadDataCCL
{
public:
  /**
   * @brief Initializes scanline CCL filling.
   * @param dataStructure Contains geometry and cell arrays.
   * @param mesgHandler Receives phase and warning messages.
   * @param shouldCancel Signals cancellation between bounded operations.
   * @param inputValues Selects defect size, phase behavior, and ignored arrays.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  FillBadDataCCL(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues);
  ~FillBadDataCCL() noexcept;

  FillBadDataCCL(const FillBadDataCCL&) = delete;
  FillBadDataCCL(FillBadDataCCL&&) noexcept = delete;
  FillBadDataCCL& operator=(const FillBadDataCCL&) = delete;
  FillBadDataCCL& operator=(FillBadDataCCL&&) noexcept = delete;

  /**
   * @brief Labels defects, classifies their sizes, and fills small regions.
   * @return Success, or a validation, overflow, provider, record, equivalence, or bulk-transfer error.
   * @pre Image dimensions and cell-array tuple counts agree and are nonzero.
   * @pre The defect threshold is nonnegative.
   * @pre Cell phases are nonnegative when new-phase storage is enabled.
   * @pre Maximum phase plus one fits Int32 when new-phase storage is enabled.
   *
   * Phase 1 immediately replaces zero Feature IDs with provisional labels.
   * Later errors do not restore the input.
   *
   * Initial maximum-ID and maximum-phase scans do not check cancellation.
   * Cancellation during labeling or relabeling can reach pair-store creation
   * and return error -87022. Cancellation during iterative filling normally
   * returns success. All cases can leave partial Feature IDs, phases, or sibling
   * arrays.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel() const;

private:
  /**
   * @brief Assigns provisional labels and records backward-neighbor equivalence.
   *
   * @param featureIdsStore Supplies input IDs and receives provisional labels.
   * @param equivalences Stores label parent, rank, and component size.
   * @param nextLabel Supplies and receives the next provisional label.
   * @param dims Image dimensions in XYZ order.
   * @return Success, including cancellation, or an overflow, equivalence, or bulk-transfer error.
   */
  Result<> phaseOneCCL(Int32AbstractDataStore& featureIdsStore, ExternalEquivalence& equivalences, int32& nextLabel, const std::array<int64, 3>& dims) const;

  /**
   * @brief Classifies provisional labels and rewrites Feature IDs.
   *
   * @param featureIdsStore Supplies provisional labels and receives classified IDs.
   * @param cellPhasesPtr Optional phase output.
   * @param startLabel First provisional label.
   * @param nextLabel One past the last provisional label.
   * @param equivalences Supplies component roots and sizes.
   * @param maxPhase Maximum existing phase.
   * @return Success, including cancellation, or an equivalence or bulk-transfer error.
   */
  Result<> phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, int32 startLabel, int32 nextLabel, ExternalEquivalence& equivalences, usize maxPhase) const;

  /**
   * @brief Iteratively fills negative Feature IDs with deferred pair records.
   *
   * @param featureIdsStore Supplies and receives Feature IDs.
   * @param dims Image dimensions in XYZ order.
   * @param numFeatures Maximum positive Feature ID.
   * @param allowInMemoryFallback Permits resident temporary records when true.
   * @return Success, or a provider, record, adjacency, overflow, or bulk-transfer error.
   */
  Result<> phaseFourIterativeFill(Int32AbstractDataStore& featureIdsStore, const std::array<int64, 3>& dims, usize numFeatures, bool allowInMemoryFallback) const;

  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
