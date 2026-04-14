#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeFeatureNeighborsInputValues;

/**
 * @class ComputeFeatureNeighborsScanline
 * @brief Out-of-core algorithm for ComputeFeatureNeighbors using Z-slice bulk I/O
 * with per-face surface area accumulation.
 *
 * Reads FeatureIds one Z-slice at a time via copyIntoBuffer using a 3-slice rolling
 * window (prev/cur/next) to resolve all 6 face neighbors with sequential disk access.
 * BoundaryCells output is written per-slice via copyFromBuffer.
 *
 * Uses map-based per-feature surface area accumulation with per-face area values,
 * matching Nathan Young's bug fix for correct surface area computation across
 * faces of different sizes.
 *
 * Selected by DispatchAlgorithm when any input array is backed by ZarrStore.
 *
 * @see ComputeFeatureNeighborsDirect for the in-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureNeighborsScanline
{
public:
  /**
   * @brief Constructs the out-of-core algorithm with all resources it needs.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  ComputeFeatureNeighborsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  const ComputeFeatureNeighborsInputValues* inputValues);
  ~ComputeFeatureNeighborsScanline() noexcept;

  ComputeFeatureNeighborsScanline(const ComputeFeatureNeighborsScanline&) = delete;
  ComputeFeatureNeighborsScanline(ComputeFeatureNeighborsScanline&&) noexcept = delete;
  ComputeFeatureNeighborsScanline& operator=(const ComputeFeatureNeighborsScanline&) = delete;
  ComputeFeatureNeighborsScanline& operator=(ComputeFeatureNeighborsScanline&&) noexcept = delete;

  /**
   * @brief Executes the OOC-optimized feature neighbor computation.
   *
   * Uses a 3-slice rolling window (prev/cur/next Z-slices) with bulk I/O:
   *   - copyIntoBuffer() reads one Z-slice of FeatureIds at a time
   *   - All 6 face-neighbor lookups are resolved from in-memory slice buffers
   *   - copyFromBuffer() writes the BoundaryCells output one Z-slice at a time
   *
   * The rolling window ensures only 3 slices of FeatureIds plus 1 slice of
   * BoundaryCells are in memory at any time, regardless of volume size.
   * Surface area accumulation uses per-face area values matching the Direct variant.
   *
   * @return Result<> with any errors encountered during execution
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                    ///< Reference to the DataStructure containing all arrays
  const ComputeFeatureNeighborsInputValues* m_InputValues = nullptr; ///< Non-owning pointer to input parameters
  const std::atomic_bool& m_ShouldCancel;                            ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;                   ///< Message handler for progress updates
};

} // namespace nx::core
