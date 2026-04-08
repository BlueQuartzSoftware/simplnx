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
  ComputeFeatureNeighborsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeFeatureNeighborsInputValues* inputValues);
  ~ComputeFeatureNeighborsDirect() noexcept;

  ComputeFeatureNeighborsDirect(const ComputeFeatureNeighborsDirect&) = delete;
  ComputeFeatureNeighborsDirect(ComputeFeatureNeighborsDirect&&) noexcept = delete;
  ComputeFeatureNeighborsDirect& operator=(const ComputeFeatureNeighborsDirect&) = delete;
  ComputeFeatureNeighborsDirect& operator=(ComputeFeatureNeighborsDirect&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
