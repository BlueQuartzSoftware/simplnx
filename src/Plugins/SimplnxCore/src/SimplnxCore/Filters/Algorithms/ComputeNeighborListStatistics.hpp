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

struct SIMPLNXCORE_EXPORT ComputeNeighborListStatisticsInputValues
{
  bool FindLength = false;
  bool FindMin = false;
  bool FindMax = false;
  bool FindMean = false;
  bool FindMedian = false;
  bool FindStdDeviation = false;
  bool FindSummation = false;

  DataPath TargetNeighborListPath = {};

  DataPath LengthPath = {};
  DataPath MinPath = {};
  DataPath MaxPath = {};
  DataPath MeanPath = {};
  DataPath MedianPath = {};
  DataPath StdDeviationPath = {};
  DataPath SummationPath = {};
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeNeighborListStatistics
{
public:
  ComputeNeighborListStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeNeighborListStatisticsInputValues* inputValues);
  ~ComputeNeighborListStatistics() noexcept;

  ComputeNeighborListStatistics(const ComputeNeighborListStatistics&) = delete;
  ComputeNeighborListStatistics(ComputeNeighborListStatistics&&) noexcept = delete;
  ComputeNeighborListStatistics& operator=(const ComputeNeighborListStatistics&) = delete;
  ComputeNeighborListStatistics& operator=(ComputeNeighborListStatistics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const ComputeNeighborListStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};

} // namespace nx::core
