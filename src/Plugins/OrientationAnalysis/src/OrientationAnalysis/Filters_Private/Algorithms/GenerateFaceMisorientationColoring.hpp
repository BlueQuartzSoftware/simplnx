#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateFaceMisorientationColoringInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SurfaceMeshFaceMisorientationColorsArrayName = filterArgs.value<DataPath>(k_SurfaceMeshFaceMisorientationColorsArrayName_Key);

  return GenerateFaceMisorientationColoring(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT GenerateFaceMisorientationColoringInputValues
{
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath SurfaceMeshFaceMisorientationColorsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT GenerateFaceMisorientationColoring
{
public:
  GenerateFaceMisorientationColoring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     GenerateFaceMisorientationColoringInputValues* inputValues);
  ~GenerateFaceMisorientationColoring() noexcept;

  GenerateFaceMisorientationColoring(const GenerateFaceMisorientationColoring&) = delete;
  GenerateFaceMisorientationColoring(GenerateFaceMisorientationColoring&&) noexcept = delete;
  GenerateFaceMisorientationColoring& operator=(const GenerateFaceMisorientationColoring&) = delete;
  GenerateFaceMisorientationColoring& operator=(GenerateFaceMisorientationColoring&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateFaceMisorientationColoringInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
