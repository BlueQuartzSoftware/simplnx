#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SurfaceNetsInputValues;

/**
 * @class SurfaceNetsDirect
 * @brief In-core algorithm for SurfaceNets. Preserves the original sequential
 * voxel iteration using MMSurfaceNet. Selected by DispatchAlgorithm when all
 * input arrays are backed by in-memory DataStore.
 */
class SIMPLNXCORE_EXPORT SurfaceNetsDirect
{
public:
  SurfaceNetsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues);
  ~SurfaceNetsDirect() noexcept;

  SurfaceNetsDirect(const SurfaceNetsDirect&) = delete;
  SurfaceNetsDirect(SurfaceNetsDirect&&) noexcept = delete;
  SurfaceNetsDirect& operator=(const SurfaceNetsDirect&) = delete;
  SurfaceNetsDirect& operator=(SurfaceNetsDirect&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const SurfaceNetsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
