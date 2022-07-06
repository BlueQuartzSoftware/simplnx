#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindNeighborListStatisticsInputValues inputValues;

  inputValues.FindLength = filterArgs.value<bool>(k_FindLength_Key);
  inputValues.FindMin = filterArgs.value<bool>(k_FindMin_Key);
  inputValues.FindMax = filterArgs.value<bool>(k_FindMax_Key);
  inputValues.FindMean = filterArgs.value<bool>(k_FindMean_Key);
  inputValues.FindMedian = filterArgs.value<bool>(k_FindMedian_Key);
  inputValues.FindStdDeviation = filterArgs.value<bool>(k_FindStdDeviation_Key);
  inputValues.FindSummation = filterArgs.value<bool>(k_FindSummation_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.DestinationAttributeMatrix = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  inputValues.LengthArrayName = filterArgs.value<DataPath>(k_LengthArrayName_Key);
  inputValues.MinimumArrayName = filterArgs.value<DataPath>(k_MinimumArrayName_Key);
  inputValues.MaximumArrayName = filterArgs.value<DataPath>(k_MaximumArrayName_Key);
  inputValues.MeanArrayName = filterArgs.value<DataPath>(k_MeanArrayName_Key);
  inputValues.MedianArrayName = filterArgs.value<DataPath>(k_MedianArrayName_Key);
  inputValues.StdDeviationArrayName = filterArgs.value<DataPath>(k_StdDeviationArrayName_Key);
  inputValues.SummationArrayName = filterArgs.value<DataPath>(k_SummationArrayName_Key);

  return FindNeighborListStatistics(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FindNeighborListStatisticsInputValues
{
  bool FindLength;
  bool FindMin;
  bool FindMax;
  bool FindMean;
  bool FindMedian;
  bool FindStdDeviation;
  bool FindSummation;
  DataPath SelectedArrayPath;
  DataPath DestinationAttributeMatrix;
  DataPath LengthArrayName;
  DataPath MinimumArrayName;
  DataPath MaximumArrayName;
  DataPath MeanArrayName;
  DataPath MedianArrayName;
  DataPath StdDeviationArrayName;
  DataPath SummationArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FindNeighborListStatistics
{
public:
  FindNeighborListStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindNeighborListStatisticsInputValues* inputValues);
  ~FindNeighborListStatistics() noexcept;

  FindNeighborListStatistics(const FindNeighborListStatistics&) = delete;
  FindNeighborListStatistics(FindNeighborListStatistics&&) noexcept = delete;
  FindNeighborListStatistics& operator=(const FindNeighborListStatistics&) = delete;
  FindNeighborListStatistics& operator=(FindNeighborListStatistics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindNeighborListStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
