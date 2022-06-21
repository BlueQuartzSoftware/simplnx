#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindArrayStatisticsInputValues inputValues;

  inputValues.FindHistogram = filterArgs.value<bool>(k_FindHistogram_Key);
  inputValues.MinRange = filterArgs.value<float64>(k_MinRange_Key);
  inputValues.MaxRange = filterArgs.value<float64>(k_MaxRange_Key);
  inputValues.UseFullRange = filterArgs.value<bool>(k_UseFullRange_Key);
  inputValues.NumBins = filterArgs.value<int32>(k_NumBins_Key);
  inputValues.FindLength = filterArgs.value<bool>(k_FindLength_Key);
  inputValues.FindMin = filterArgs.value<bool>(k_FindMin_Key);
  inputValues.FindMax = filterArgs.value<bool>(k_FindMax_Key);
  inputValues.FindMean = filterArgs.value<bool>(k_FindMean_Key);
  inputValues.FindMedian = filterArgs.value<bool>(k_FindMedian_Key);
  inputValues.FindStdDeviation = filterArgs.value<bool>(k_FindStdDeviation_Key);
  inputValues.FindSummation = filterArgs.value<bool>(k_FindSummation_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.ComputeByIndex = filterArgs.value<bool>(k_ComputeByIndex_Key);
  inputValues.StandardizeData = filterArgs.value<bool>(k_StandardizeData_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.DestinationAttributeMatrix = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  inputValues.HistogramArrayName = filterArgs.value<DataPath>(k_HistogramArrayName_Key);
  inputValues.LengthArrayName = filterArgs.value<DataPath>(k_LengthArrayName_Key);
  inputValues.MinimumArrayName = filterArgs.value<DataPath>(k_MinimumArrayName_Key);
  inputValues.MaximumArrayName = filterArgs.value<DataPath>(k_MaximumArrayName_Key);
  inputValues.MeanArrayName = filterArgs.value<DataPath>(k_MeanArrayName_Key);
  inputValues.MedianArrayName = filterArgs.value<DataPath>(k_MedianArrayName_Key);
  inputValues.StdDeviationArrayName = filterArgs.value<DataPath>(k_StdDeviationArrayName_Key);
  inputValues.SummationArrayName = filterArgs.value<DataPath>(k_SummationArrayName_Key);
  inputValues.StandardizedArrayName = filterArgs.value<DataPath>(k_StandardizedArrayName_Key);

  return FindArrayStatistics(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FindArrayStatisticsInputValues
{
  bool FindHistogram;
  float64 MinRange;
  float64 MaxRange;
  bool UseFullRange;
  int32 NumBins;
  bool FindLength;
  bool FindMin;
  bool FindMax;
  bool FindMean;
  bool FindMedian;
  bool FindStdDeviation;
  bool FindSummation;
  bool UseMask;
  bool ComputeByIndex;
  bool StandardizeData;
  DataPath SelectedArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath MaskArrayPath;
  DataPath DestinationAttributeMatrix;
  DataPath HistogramArrayName;
  DataPath LengthArrayName;
  DataPath MinimumArrayName;
  DataPath MaximumArrayName;
  DataPath MeanArrayName;
  DataPath MedianArrayName;
  DataPath StdDeviationArrayName;
  DataPath SummationArrayName;
  DataPath StandardizedArrayName;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FindArrayStatistics
{
public:
  FindArrayStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindArrayStatisticsInputValues* inputValues);
  ~FindArrayStatistics() noexcept;

  FindArrayStatistics(const FindArrayStatistics&) = delete;
  FindArrayStatistics(FindArrayStatistics&&) noexcept = delete;
  FindArrayStatistics& operator=(const FindArrayStatistics&) = delete;
  FindArrayStatistics& operator=(FindArrayStatistics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindArrayStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
