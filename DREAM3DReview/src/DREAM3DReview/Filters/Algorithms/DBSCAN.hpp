#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DBSCANInputValues inputValues;

  inputValues.Epsilon = filterArgs.value<float32>(k_Epsilon_Key);
  inputValues.MinPnts = filterArgs.value<int32>(k_MinPnts_Key);
  inputValues.DistanceMetric = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  inputValues.FeatureAttributeMatrixName = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);

  return DBSCAN(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT DBSCANInputValues
{
  float32 Epsilon;
  int32 MinPnts;
  ChoicesParameter::ValueType DistanceMetric;
  bool UseMask;
  DataPath SelectedArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayName;
  DataPath FeatureAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT DBSCAN
{
public:
  DBSCAN(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues);
  ~DBSCAN() noexcept;

  DBSCAN(const DBSCAN&) = delete;
  DBSCAN(DBSCAN&&) noexcept = delete;
  DBSCAN& operator=(const DBSCAN&) = delete;
  DBSCAN& operator=(DBSCAN&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const DBSCANInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
