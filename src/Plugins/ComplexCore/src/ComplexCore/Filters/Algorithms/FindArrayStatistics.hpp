#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

#include <chrono>
#include <mutex>

namespace complex
{

struct COMPLEXCORE_EXPORT FindArrayStatisticsInputValues
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
  bool FindMode;
  bool FindModalBinRanges;
  bool FindStdDeviation;
  bool FindSummation;
  bool UseMask;
  ;
  bool ComputeByIndex;
  bool StandardizeData;
  bool FindNumUniqueValues;
  DataPath SelectedArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath MaskArrayPath;
  DataPath DestinationAttributeMatrix;
  DataPath HistogramArrayName;
  DataPath MostPopulatedBinArrayName;
  DataPath ModalBinArrayName;
  DataPath FeatureHasDataArrayName;
  DataPath LengthArrayName;
  DataPath MinimumArrayName;
  DataPath MaximumArrayName;
  DataPath MeanArrayName;
  DataPath MedianArrayName;
  DataPath ModeArrayName;
  DataPath StdDeviationArrayName;
  DataPath SummationArrayName;
  DataPath StandardizedArrayName;
  DataPath NumUniqueValuesName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT FindArrayStatistics
{
public:
  FindArrayStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, FindArrayStatisticsInputValues* inputValues);
  ~FindArrayStatistics() noexcept;

  FindArrayStatistics(const FindArrayStatistics&) = delete;
  FindArrayStatistics(FindArrayStatistics&&) noexcept = delete;
  FindArrayStatistics& operator=(const FindArrayStatistics&) = delete;
  FindArrayStatistics& operator=(FindArrayStatistics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendThreadSafeProgressMessage(usize counter);
  void sendThreadSafeInfoMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const FindArrayStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  usize m_MilliDelay = 1000;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  size_t m_LastProgressInt = 0;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
  /**
   * @brief
   * @param featureIds
   * @return
   */
  usize findNumFeatures(const Int32Array& featureIds) const;
};

} // namespace complex
