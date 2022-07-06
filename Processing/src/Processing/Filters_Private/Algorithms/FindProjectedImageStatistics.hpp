#pragma once

#include "Processing/Processing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindProjectedImageStatisticsInputValues inputValues;

  inputValues.Plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.ProjectedImageMinArrayName = filterArgs.value<DataPath>(k_ProjectedImageMinArrayName_Key);
  inputValues.ProjectedImageMaxArrayName = filterArgs.value<DataPath>(k_ProjectedImageMaxArrayName_Key);
  inputValues.ProjectedImageAvgArrayName = filterArgs.value<DataPath>(k_ProjectedImageAvgArrayName_Key);
  inputValues.ProjectedImageStdArrayName = filterArgs.value<DataPath>(k_ProjectedImageStdArrayName_Key);
  inputValues.ProjectedImageVarArrayName = filterArgs.value<DataPath>(k_ProjectedImageVarArrayName_Key);

  return FindProjectedImageStatistics(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROCESSING_EXPORT FindProjectedImageStatisticsInputValues
{
  ChoicesParameter::ValueType Plane;
  DataPath SelectedArrayPath;
  DataPath ProjectedImageMinArrayName;
  DataPath ProjectedImageMaxArrayName;
  DataPath ProjectedImageAvgArrayName;
  DataPath ProjectedImageStdArrayName;
  DataPath ProjectedImageVarArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROCESSING_EXPORT FindProjectedImageStatistics
{
public:
  FindProjectedImageStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindProjectedImageStatisticsInputValues* inputValues);
  ~FindProjectedImageStatistics() noexcept;

  FindProjectedImageStatistics(const FindProjectedImageStatistics&) = delete;
  FindProjectedImageStatistics(FindProjectedImageStatistics&&) noexcept = delete;
  FindProjectedImageStatistics& operator=(const FindProjectedImageStatistics&) = delete;
  FindProjectedImageStatistics& operator=(FindProjectedImageStatistics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindProjectedImageStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
