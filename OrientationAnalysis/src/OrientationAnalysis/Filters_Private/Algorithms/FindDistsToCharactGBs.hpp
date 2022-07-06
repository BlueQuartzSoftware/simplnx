#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindDistsToCharactGBsInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.DistToTiltArrayPath = filterArgs.value<DataPath>(k_DistToTiltArrayPath_Key);
  inputValues.DistToTwistArrayPath = filterArgs.value<DataPath>(k_DistToTwistArrayPath_Key);
  inputValues.DistToSymmetricArrayPath = filterArgs.value<DataPath>(k_DistToSymmetricArrayPath_Key);
  inputValues.DistTo180TiltArrayPath = filterArgs.value<DataPath>(k_DistTo180TiltArrayPath_Key);

  return FindDistsToCharactGBs(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindDistsToCharactGBsInputValues
{
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath DistToTiltArrayPath;
  DataPath DistToTwistArrayPath;
  DataPath DistToSymmetricArrayPath;
  DataPath DistTo180TiltArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindDistsToCharactGBs
{
public:
  FindDistsToCharactGBs(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindDistsToCharactGBsInputValues* inputValues);
  ~FindDistsToCharactGBs() noexcept;

  FindDistsToCharactGBs(const FindDistsToCharactGBs&) = delete;
  FindDistsToCharactGBs(FindDistsToCharactGBs&&) noexcept = delete;
  FindDistsToCharactGBs& operator=(const FindDistsToCharactGBs&) = delete;
  FindDistsToCharactGBs& operator=(FindDistsToCharactGBs&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindDistsToCharactGBsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
