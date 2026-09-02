#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <chrono>
#include <mutex>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeArrayStatisticsInputValues
{
  ChoicesParameter::ValueType RangeType;
  VectorInt32Parameter::ValueType Range;
  bool FindLength;
  bool FindMin;
  bool FindMax;
  bool FindMean;
  bool FindMedian;
  bool FindMode;
  bool FindStdDeviation;
  bool FindSummation;
  bool UseMask;
  bool ComputeByIndex;
  bool StandardizeData;
  bool FindNumUniqueValues;
  DataPath SelectedArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath MaskArrayPath;
  DataPath DestinationAttributeMatrix;
  DataPath BinCountsArrayName;
  DataPath BinRangesArrayName;
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
  DataPath TempMaskArrayPath;
  DataPath FeatureIdMapArrayPath;
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

  // sequence dependent DO NOT REORDER
  enum FeatureIdRangeControls : uint8
  {
    None = 0,
    IgnoreZero = 1,
    ShrinkToFit = 2,
    PaddedCustomRange = 3,
    CustomRange = 4
  };

  Result<> operator()();

  /**
   * @brief Thread-safe throttled progress update. Takes a functor rather than a string because the
   * callers reset a progress counter inside it, so the body must run only when a message is due.
   * @param functor Callable of the form std::string func()
   */
  template <class CallableT>
  requires std::is_invocable_r_v<std::string, CallableT>
  void sendThreadSafeProgressMessage(CallableT&& functor)
  {
    std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
    m_Throttle.queueMessage(std::forward<CallableT>(functor));
  }

  /**
   * @brief Thread-safe guaranteed status message. Safe to call from the parallel range workers.
   * @param message
   */
  void sendThreadSafeInfoMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const ComputeArrayStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
};

} // namespace nx::core
