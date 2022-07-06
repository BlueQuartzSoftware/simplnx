#pragma once

#include "VolumeMeshing/VolumeMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CleaveTetVolumeMeshInputValues inputValues;

  inputValues.SignedDistanceArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SignedDistanceArrayPaths_Key);
  inputValues.Lipschitz = filterArgs.value<float32>(k_Lipschitz_Key);
  inputValues.Scale = filterArgs.value<float32>(k_Scale_Key);
  inputValues.Multiplier = filterArgs.value<float32>(k_Multiplier_Key);
  inputValues.Alpha = filterArgs.value<float32>(k_Alpha_Key);
  inputValues.Padding = filterArgs.value<int32>(k_Padding_Key);
  inputValues.AdaptiveSurface = filterArgs.value<bool>(k_AdaptiveSurface_Key);

  return CleaveTetVolumeMesh(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct VOLUMEMESHING_EXPORT CleaveTetVolumeMeshInputValues
{
  MultiArraySelectionParameter::ValueType SignedDistanceArrayPaths;
  float32 Lipschitz;
  float32 Scale;
  float32 Multiplier;
  float32 Alpha;
  int32 Padding;
  bool AdaptiveSurface;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class VOLUMEMESHING_EXPORT CleaveTetVolumeMesh
{
public:
  CleaveTetVolumeMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CleaveTetVolumeMeshInputValues* inputValues);
  ~CleaveTetVolumeMesh() noexcept;

  CleaveTetVolumeMesh(const CleaveTetVolumeMesh&) = delete;
  CleaveTetVolumeMesh(CleaveTetVolumeMesh&&) noexcept = delete;
  CleaveTetVolumeMesh& operator=(const CleaveTetVolumeMesh&) = delete;
  CleaveTetVolumeMesh& operator=(CleaveTetVolumeMesh&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CleaveTetVolumeMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
