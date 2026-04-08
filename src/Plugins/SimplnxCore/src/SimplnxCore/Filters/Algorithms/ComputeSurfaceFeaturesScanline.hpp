#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceFeaturesInputValues;

/**
 * @class ComputeSurfaceFeaturesScanline
 * @brief Out-of-core algorithm for ComputeSurfaceFeatures. Uses chunk-sequential 3D iteration
 * with 2D coordinate remapping for degenerate dimensions, ensuring sequential disk I/O
 * on ZarrStore-backed arrays. Selected by DispatchAlgorithm when any input array is
 * backed by ZarrStore.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesScanline
{
public:
  ComputeSurfaceFeaturesScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeSurfaceFeaturesInputValues* inputValues);
  ~ComputeSurfaceFeaturesScanline() noexcept;

  ComputeSurfaceFeaturesScanline(const ComputeSurfaceFeaturesScanline&) = delete;
  ComputeSurfaceFeaturesScanline(ComputeSurfaceFeaturesScanline&&) noexcept = delete;
  ComputeSurfaceFeaturesScanline& operator=(const ComputeSurfaceFeaturesScanline&) = delete;
  ComputeSurfaceFeaturesScanline& operator=(ComputeSurfaceFeaturesScanline&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
