#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include <chrono>
#include <mutex>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CalculateArrayHistogramInputValues
{
  int32 NumberOfBins = 0;
  bool UserDefinedRange = false;
  float64 MinRange = 0.0;
  float64 MaxRange = 0.0;
  MultiArraySelectionParameter::ValueType SelectedArrayPaths = {};
  MultiArraySelectionParameter::ValueType CreatedHistogramDataPaths = {};
};

/**
 * @class CalculateArrayHistogram
 * @brief This filter calculates a Histogram according to user specification and stores it accordingly
 */

class SIMPLNXCORE_EXPORT CalculateArrayHistogram
{
public:
  CalculateArrayHistogram(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CalculateArrayHistogramInputValues* inputValues);
  ~CalculateArrayHistogram() noexcept;

  CalculateArrayHistogram(const CalculateArrayHistogram&) = delete;
  CalculateArrayHistogram(CalculateArrayHistogram&&) noexcept = delete;
  CalculateArrayHistogram& operator=(const CalculateArrayHistogram&) = delete;
  CalculateArrayHistogram& operator=(CalculateArrayHistogram&&) noexcept = delete;

  Result<> operator()();

  void updateProgress(const std::string& progMessage);
  void updateThreadSafeProgress(size_t counter);
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CalculateArrayHistogramInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Threadsafe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};

} // namespace nx::core
