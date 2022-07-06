#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindCSLBoundariesInputValues inputValues;

  inputValues.CSL = filterArgs.value<float32>(k_CSL_Key);
  inputValues.AxisTolerance = filterArgs.value<float32>(k_AxisTolerance_Key);
  inputValues.AngleTolerance = filterArgs.value<float32>(k_AngleTolerance_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.SurfaceMeshCSLBoundaryArrayName = filterArgs.value<DataPath>(k_SurfaceMeshCSLBoundaryArrayName_Key);
  inputValues.SurfaceMeshCSLBoundaryIncoherenceArrayName = filterArgs.value<DataPath>(k_SurfaceMeshCSLBoundaryIncoherenceArrayName_Key);

  return FindCSLBoundaries(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FindCSLBoundariesInputValues
{
  float32 CSL;
  float32 AxisTolerance;
  float32 AngleTolerance;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshCSLBoundaryArrayName;
  DataPath SurfaceMeshCSLBoundaryIncoherenceArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FindCSLBoundaries
{
public:
  FindCSLBoundaries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindCSLBoundariesInputValues* inputValues);
  ~FindCSLBoundaries() noexcept;

  FindCSLBoundaries(const FindCSLBoundaries&) = delete;
  FindCSLBoundaries(FindCSLBoundaries&&) noexcept = delete;
  FindCSLBoundaries& operator=(const FindCSLBoundaries&) = delete;
  FindCSLBoundaries& operator=(FindCSLBoundaries&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindCSLBoundariesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
