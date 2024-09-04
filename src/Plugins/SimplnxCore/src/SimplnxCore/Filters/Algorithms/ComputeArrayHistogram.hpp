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

struct SIMPLNXCORE_EXPORT ComputeArrayHistogramInputValues
{
  int32 NumberOfBins = 0;
  bool UserDefinedRange = false;
  float64 MinRange = 0.0;
  float64 MaxRange = 0.0;
  MultiArraySelectionParameter::ValueType SelectedArrayPaths = {};
  MultiArraySelectionParameter::ValueType CreatedHistogramCountsDataPaths = {};
  MultiArraySelectionParameter::ValueType CreatedBinRangeDataPaths = {};
};

/**
 * @class ComputeArrayHistogram
 * @brief This filter calculates a Histogram according to user specification and stores it accordingly
 */

class SIMPLNXCORE_EXPORT ComputeArrayHistogram
{
public:
  ComputeArrayHistogram(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeArrayHistogramInputValues* inputValues);
  ~ComputeArrayHistogram() noexcept;

  ComputeArrayHistogram(const ComputeArrayHistogram&) = delete;
  ComputeArrayHistogram(ComputeArrayHistogram&&) noexcept = delete;
  ComputeArrayHistogram& operator=(const ComputeArrayHistogram&) = delete;
  ComputeArrayHistogram& operator=(ComputeArrayHistogram&&) noexcept = delete;

  Result<> operator()();

  void updateProgress(const std::string& progMessage);
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeArrayHistogramInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Threadsafe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};

} // namespace nx::core
