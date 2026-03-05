#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceAreaToVolumeInputValues;

/**
 * @class ComputeSurfaceAreaToVolumeScanline
 * @brief Out-of-core algorithm for ComputeSurfaceAreaToVolume. Wraps the voxel iteration in
 * chunk-sequential access for guaranteed sequential disk I/O on ZarrStore-backed arrays.
 * Feature-level ratio and sphericity computations are unchanged. Selected by DispatchAlgorithm
 * when any input array is backed by ZarrStore.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolumeScanline
{
public:
  ComputeSurfaceAreaToVolumeScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     const ComputeSurfaceAreaToVolumeInputValues* inputValues);
  ~ComputeSurfaceAreaToVolumeScanline() noexcept;

  ComputeSurfaceAreaToVolumeScanline(const ComputeSurfaceAreaToVolumeScanline&) = delete;
  ComputeSurfaceAreaToVolumeScanline(ComputeSurfaceAreaToVolumeScanline&&) noexcept = delete;
  ComputeSurfaceAreaToVolumeScanline& operator=(const ComputeSurfaceAreaToVolumeScanline&) = delete;
  ComputeSurfaceAreaToVolumeScanline& operator=(ComputeSurfaceAreaToVolumeScanline&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceAreaToVolumeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
