#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ScaleVolumeInputValues inputValues;

  inputValues.ScaleFactor = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ScaleFactor_Key);
  inputValues.ApplyToVoxelVolume = filterArgs.value<bool>(k_ApplyToVoxelVolume_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.ApplyToSurfaceMesh = filterArgs.value<bool>(k_ApplyToSurfaceMesh_Key);
  inputValues.SurfaceDataContainerName = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  return ScaleVolume(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ScaleVolumeInputValues
{
  VectorFloat32Parameter::ValueType ScaleFactor;
  bool ApplyToVoxelVolume;
  DataPath DataContainerName;
  bool ApplyToSurfaceMesh;
  DataPath SurfaceDataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ScaleVolume
{
public:
  ScaleVolume(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ScaleVolumeInputValues* inputValues);
  ~ScaleVolume() noexcept;

  ScaleVolume(const ScaleVolume&) = delete;
  ScaleVolume(ScaleVolume&&) noexcept = delete;
  ScaleVolume& operator=(const ScaleVolume&) = delete;
  ScaleVolume& operator=(ScaleVolume&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ScaleVolumeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
