#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeBoundaryCellsInputValues;

/**
 * @class ComputeBoundaryCellsScanline
 * @brief Out-of-core algorithm for ComputeBoundaryCells. Iterates Z-slices sequentially
 * using copyIntoBuffer/copyFromBuffer bulk I/O with a 3-slice rolling window
 * (prevSlice, curSlice, nextSlice) for Z-neighbor access. Selected by DispatchAlgorithm
 * when any input array is backed by ZarrStore.
 */
class SIMPLNXCORE_EXPORT ComputeBoundaryCellsScanline
{
public:
  ComputeBoundaryCellsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCellsScanline() noexcept;

  ComputeBoundaryCellsScanline(const ComputeBoundaryCellsScanline&) = delete;
  ComputeBoundaryCellsScanline(ComputeBoundaryCellsScanline&&) noexcept = delete;
  ComputeBoundaryCellsScanline& operator=(const ComputeBoundaryCellsScanline&) = delete;
  ComputeBoundaryCellsScanline& operator=(ComputeBoundaryCellsScanline&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryCellsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
