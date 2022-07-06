#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindShapesInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.Omega3sArrayName = filterArgs.value<DataPath>(k_Omega3sArrayName_Key);
  inputValues.AxisLengthsArrayName = filterArgs.value<DataPath>(k_AxisLengthsArrayName_Key);
  inputValues.AxisEulerAnglesArrayName = filterArgs.value<DataPath>(k_AxisEulerAnglesArrayName_Key);
  inputValues.AspectRatiosArrayName = filterArgs.value<DataPath>(k_AspectRatiosArrayName_Key);
  inputValues.VolumesArrayName = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

  return FindShapes(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FindShapesInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixName;
  DataPath CentroidsArrayPath;
  DataPath Omega3sArrayName;
  DataPath AxisLengthsArrayName;
  DataPath AxisEulerAnglesArrayName;
  DataPath AspectRatiosArrayName;
  DataPath VolumesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FindShapes
{
public:
  FindShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindShapesInputValues* inputValues);
  ~FindShapes() noexcept;

  FindShapes(const FindShapes&) = delete;
  FindShapes(FindShapes&&) noexcept = delete;
  FindShapes& operator=(const FindShapes&) = delete;
  FindShapes& operator=(FindShapes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindShapesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
