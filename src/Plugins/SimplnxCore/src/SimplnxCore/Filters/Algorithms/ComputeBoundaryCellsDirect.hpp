#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeBoundaryCellsInputValues;

/**
 * @class ComputeBoundaryCellsDirect
 * @brief In-core algorithm for ComputeBoundaryCells. Preserves the original sequential
 * Z-Y-X voxel iteration with face-neighbor boundary counting. Selected by DispatchAlgorithm
 * when all input arrays are backed by in-memory DataStore.
 */
class SIMPLNXCORE_EXPORT ComputeBoundaryCellsDirect
{
public:
  ComputeBoundaryCellsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCellsDirect() noexcept;

  ComputeBoundaryCellsDirect(const ComputeBoundaryCellsDirect&) = delete;
  ComputeBoundaryCellsDirect(ComputeBoundaryCellsDirect&&) noexcept = delete;
  ComputeBoundaryCellsDirect& operator=(const ComputeBoundaryCellsDirect&) = delete;
  ComputeBoundaryCellsDirect& operator=(ComputeBoundaryCellsDirect&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryCellsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
