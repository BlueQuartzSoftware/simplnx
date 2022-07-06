#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  KDistanceGraphInputValues inputValues;

  inputValues.MinDist = filterArgs.value<int32>(k_MinDist_Key);
  inputValues.DistanceMetric = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.KDistanceArrayPath = filterArgs.value<DataPath>(k_KDistanceArrayPath_Key);

  return KDistanceGraph(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT KDistanceGraphInputValues
{
  int32 MinDist;
  ChoicesParameter::ValueType DistanceMetric;
  bool UseMask;
  DataPath MaskArrayPath;
  DataPath SelectedArrayPath;
  DataPath KDistanceArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT KDistanceGraph
{
public:
  KDistanceGraph(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KDistanceGraphInputValues* inputValues);
  ~KDistanceGraph() noexcept;

  KDistanceGraph(const KDistanceGraph&) = delete;
  KDistanceGraph(KDistanceGraph&&) noexcept = delete;
  KDistanceGraph& operator=(const KDistanceGraph&) = delete;
  KDistanceGraph& operator=(KDistanceGraph&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const KDistanceGraphInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
