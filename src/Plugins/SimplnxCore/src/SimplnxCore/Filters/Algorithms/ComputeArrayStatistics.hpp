#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <chrono>
#include <mutex>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeArrayStatisticsInputValues
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
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeArrayStatistics
{
public:
  ComputeArrayStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeArrayStatisticsInputValues* inputValues);
  ~ComputeArrayStatistics() noexcept;

  ComputeArrayStatistics(const ComputeArrayStatistics&) = delete;
  ComputeArrayStatistics(ComputeArrayStatistics&&) noexcept = delete;
  ComputeArrayStatistics& operator=(const ComputeArrayStatistics&) = delete;
  ComputeArrayStatistics& operator=(ComputeArrayStatistics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendThreadSafeProgressMessage(usize counter);
  void sendThreadSafeInfoMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const ComputeArrayStatisticsInputValues* m_InputValues = nullptr;
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

} // namespace nx::core
