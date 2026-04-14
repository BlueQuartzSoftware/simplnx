#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeFeatureNeighborsInputValues;

/**
 * @class ComputeFeatureNeighborsDirect
 * @brief In-core algorithm for ComputeFeatureNeighbors using compile-time dimension
 * specialization and per-face surface area accumulation.
 *
 * Uses Nathan Young's rewritten algorithm with two-stage processing:
 *   Stage 1: Boundary cells (corners, edges, faces) with validity checks
 *   Stage 2: Internal cells (3D only) with all 6 neighbors guaranteed valid
 *
 * Accumulates per-face surface areas using precomputed face dimensions rather
 * than a uniform area, fixing a surface area calculation bug from DREAM3D 6.5.
 * Handles 0D/1D/2D/3D geometries via constexpr template specialization.
 *
 * Selected by DispatchAlgorithm when all input arrays are backed by in-memory DataStore.
 *
 * @see ComputeFeatureNeighborsScanline for the out-of-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureNeighborsDirect
{
public:
  /**
   * @brief Constructs the in-core algorithm with all resources it needs.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  ComputeFeatureNeighborsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeFeatureNeighborsInputValues* inputValues);
  ~ComputeFeatureNeighborsDirect() noexcept;

  ComputeFeatureNeighborsDirect(const ComputeFeatureNeighborsDirect&) = delete;
  ComputeFeatureNeighborsDirect(ComputeFeatureNeighborsDirect&&) noexcept = delete;
  ComputeFeatureNeighborsDirect& operator=(const ComputeFeatureNeighborsDirect&) = delete;
  ComputeFeatureNeighborsDirect& operator=(ComputeFeatureNeighborsDirect&&) noexcept = delete;

  /**
   * @brief Executes the in-core feature neighbor computation.
   *
   * Uses Nathan Young's two-stage algorithm with compile-time dimension specialization:
   *   - Stage 1: Process boundary cells (corners, edges, faces) with validity checks
   *   - Stage 2: Process internal cells with all 6 neighbors guaranteed valid
   *
   * Per-face surface areas are computed using precomputed face dimensions rather
   * than a uniform area, fixing a bug from DREAM3D 6.5.
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
