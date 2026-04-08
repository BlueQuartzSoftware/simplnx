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
  ComputeFeatureNeighborsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  const ComputeFeatureNeighborsInputValues* inputValues);
  ~ComputeFeatureNeighborsScanline() noexcept;

  ComputeFeatureNeighborsScanline(const ComputeFeatureNeighborsScanline&) = delete;
  ComputeFeatureNeighborsScanline(ComputeFeatureNeighborsScanline&&) noexcept = delete;
  ComputeFeatureNeighborsScanline& operator=(const ComputeFeatureNeighborsScanline&) = delete;
  ComputeFeatureNeighborsScanline& operator=(ComputeFeatureNeighborsScanline&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
