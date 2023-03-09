#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindTwinBoundariesInputValues inputValues;

  inputValues.AxisTolerance = filterArgs.value<float32>(k_AxisTolerance_Key);
  inputValues.AngleTolerance = filterArgs.value<float32>(k_AngleTolerance_Key);
  inputValues.FindCoherence = filterArgs.value<bool>(k_FindCoherence_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.SurfaceMeshTwinBoundaryArrayName = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundaryArrayName_Key);
  inputValues.SurfaceMeshTwinBoundaryIncoherenceArrayName = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundaryIncoherenceArrayName_Key);

  return FindTwinBoundaries(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindTwinBoundariesInputValues
{
  float32 AxisTolerance;
  float32 AngleTolerance;
  bool FindCoherence;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshTwinBoundaryArrayName;
  DataPath SurfaceMeshTwinBoundaryIncoherenceArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindTwinBoundaries
{
public:
  FindTwinBoundaries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindTwinBoundariesInputValues* inputValues);
  ~FindTwinBoundaries() noexcept;

  FindTwinBoundaries(const FindTwinBoundaries&) = delete;
  FindTwinBoundaries(FindTwinBoundaries&&) noexcept = delete;
  FindTwinBoundaries& operator=(const FindTwinBoundaries&) = delete;
  FindTwinBoundaries& operator=(FindTwinBoundaries&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindTwinBoundariesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
