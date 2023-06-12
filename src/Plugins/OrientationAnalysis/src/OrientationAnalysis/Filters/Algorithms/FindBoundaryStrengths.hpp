#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"


/**
* This is example code to put in the Execute Method of the filter.
  FindBoundaryStrengthsInputValues inputValues;

  inputValues.Loading = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Loading_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SurfaceMeshF1sArrayName = filterArgs.value<DataPath>(k_SurfaceMeshF1sArrayName_Key);
  inputValues.SurfaceMeshF1sptsArrayName = filterArgs.value<DataPath>(k_SurfaceMeshF1sptsArrayName_Key);
  inputValues.SurfaceMeshF7sArrayName = filterArgs.value<DataPath>(k_SurfaceMeshF7sArrayName_Key);
  inputValues.SurfaceMeshmPrimesArrayName = filterArgs.value<DataPath>(k_SurfaceMeshmPrimesArrayName_Key);

  return FindBoundaryStrengths(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindBoundaryStrengthsInputValues
{
  VectorFloat32Parameter::ValueType Loading;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath SurfaceMeshF1sArrayName;
  DataPath SurfaceMeshF1sptsArrayName;
  DataPath SurfaceMeshF7sArrayName;
  DataPath SurfaceMeshmPrimesArrayName;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindBoundaryStrengths
{
public:
  FindBoundaryStrengths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBoundaryStrengthsInputValues* inputValues);
  ~FindBoundaryStrengths() noexcept;

  FindBoundaryStrengths(const FindBoundaryStrengths&) = delete;
  FindBoundaryStrengths(FindBoundaryStrengths&&) noexcept = delete;
  FindBoundaryStrengths& operator=(const FindBoundaryStrengths&) = delete;
  FindBoundaryStrengths& operator=(FindBoundaryStrengths&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindBoundaryStrengthsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
