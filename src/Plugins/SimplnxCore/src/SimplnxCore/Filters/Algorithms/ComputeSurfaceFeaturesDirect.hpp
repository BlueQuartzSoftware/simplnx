#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceFeaturesInputValues;

/**
 * @class ComputeSurfaceFeaturesDirect
 * @brief In-core algorithm for ComputeSurfaceFeatures. Preserves the original 2D/3D branching
 * with sequential voxel iteration and face-neighbor surface detection. Selected by
 * DispatchAlgorithm when all input arrays are backed by in-memory DataStore.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesDirect
{
public:
  ComputeSurfaceFeaturesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                               const ComputeSurfaceFeaturesInputValues* inputValues);
  ~ComputeSurfaceFeaturesDirect() noexcept;

  ComputeSurfaceFeaturesDirect(const ComputeSurfaceFeaturesDirect&) = delete;
  ComputeSurfaceFeaturesDirect(ComputeSurfaceFeaturesDirect&&) noexcept = delete;
  ComputeSurfaceFeaturesDirect& operator=(const ComputeSurfaceFeaturesDirect&) = delete;
  ComputeSurfaceFeaturesDirect& operator=(ComputeSurfaceFeaturesDirect&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
