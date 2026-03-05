#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceAreaToVolumeInputValues;

class SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolumeDirect
{
public:
  ComputeSurfaceAreaToVolumeDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   const ComputeSurfaceAreaToVolumeInputValues* inputValues);
  ~ComputeSurfaceAreaToVolumeDirect() noexcept;

  ComputeSurfaceAreaToVolumeDirect(const ComputeSurfaceAreaToVolumeDirect&) = delete;
  ComputeSurfaceAreaToVolumeDirect(ComputeSurfaceAreaToVolumeDirect&&) noexcept = delete;
  ComputeSurfaceAreaToVolumeDirect& operator=(const ComputeSurfaceAreaToVolumeDirect&) = delete;
  ComputeSurfaceAreaToVolumeDirect& operator=(ComputeSurfaceAreaToVolumeDirect&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceAreaToVolumeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
