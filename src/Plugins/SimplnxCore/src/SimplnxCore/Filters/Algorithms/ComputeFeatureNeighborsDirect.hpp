#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeFeatureNeighborsInputValues;

/**
 * @class ComputeFeatureNeighborsDirect
 * @brief In-core algorithm for ComputeFeatureNeighbors. Preserves the original two-phase
 * algorithm: Phase 1 iterates all voxels to build per-feature neighbor lists, Phase 2
 * computes shared surface areas. Selected by DispatchAlgorithm when all input arrays
 * are backed by in-memory DataStore.
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
