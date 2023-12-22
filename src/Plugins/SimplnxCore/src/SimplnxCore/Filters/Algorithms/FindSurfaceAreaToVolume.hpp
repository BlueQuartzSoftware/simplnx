#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FindSurfaceAreaToVolumeInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath NumCellsArrayPath;
  DataPath SurfaceAreaVolumeRatioArrayName;
  bool CalculateSphericity;
  DataPath SphericityArrayName;
  DataPath InputImageGeometry;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT FindSurfaceAreaToVolume
{
public:
  FindSurfaceAreaToVolume(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindSurfaceAreaToVolumeInputValues* inputValues);
  ~FindSurfaceAreaToVolume() noexcept;

  FindSurfaceAreaToVolume(const FindSurfaceAreaToVolume&) = delete;
  FindSurfaceAreaToVolume(FindSurfaceAreaToVolume&&) noexcept = delete;
  FindSurfaceAreaToVolume& operator=(const FindSurfaceAreaToVolume&) = delete;
  FindSurfaceAreaToVolume& operator=(FindSurfaceAreaToVolume&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindSurfaceAreaToVolumeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
