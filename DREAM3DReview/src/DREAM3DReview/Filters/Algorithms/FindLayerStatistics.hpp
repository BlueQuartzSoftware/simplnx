#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindLayerStatisticsInputValues inputValues;

  inputValues.Plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.LayerAttributeMatrixName = filterArgs.value<DataPath>(k_LayerAttributeMatrixName_Key);
  inputValues.LayerIDsArrayName = filterArgs.value<DataPath>(k_LayerIDsArrayName_Key);
  inputValues.LayerMinArrayName = filterArgs.value<DataPath>(k_LayerMinArrayName_Key);
  inputValues.LayerMaxArrayName = filterArgs.value<DataPath>(k_LayerMaxArrayName_Key);
  inputValues.LayerAvgArrayName = filterArgs.value<DataPath>(k_LayerAvgArrayName_Key);
  inputValues.LayerStdArrayName = filterArgs.value<DataPath>(k_LayerStdArrayName_Key);
  inputValues.LayerVarArrayName = filterArgs.value<DataPath>(k_LayerVarArrayName_Key);

  return FindLayerStatistics(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FindLayerStatisticsInputValues
{
  ChoicesParameter::ValueType Plane;
  DataPath SelectedArrayPath;
  DataPath LayerAttributeMatrixName;
  DataPath LayerIDsArrayName;
  DataPath LayerMinArrayName;
  DataPath LayerMaxArrayName;
  DataPath LayerAvgArrayName;
  DataPath LayerStdArrayName;
  DataPath LayerVarArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FindLayerStatistics
{
public:
  FindLayerStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindLayerStatisticsInputValues* inputValues);
  ~FindLayerStatistics() noexcept;

  FindLayerStatistics(const FindLayerStatistics&) = delete;
  FindLayerStatistics(FindLayerStatistics&&) noexcept = delete;
  FindLayerStatistics& operator=(const FindLayerStatistics&) = delete;
  FindLayerStatistics& operator=(FindLayerStatistics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindLayerStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
