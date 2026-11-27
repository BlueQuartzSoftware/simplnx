#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <chrono>
#include <mutex>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeNeighborListStatisticsInputValues
{
  bool find_length = false;
  bool find_min = false;
  bool find_max = false;
  bool find_mean = false;
  bool find_median = false;
  bool find_std_deviation = false;
  bool find_summation = false;

  DataPath target_neighbor_list_path;

  DataPath length_path;
  DataPath min_path;
  DataPath max_path;
  DataPath mean_path;
  DataPath median_path;
  DataPath std_deviation_path;
  DataPath summation_path;
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

  /**
   * @brief Thread-safe progress update. Safe to call from ParallelDataAlgorithm workers.
   * @param counter Items completed since the previous call
   */
  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const ComputeNeighborListStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessageMutex;
  ThrottledMessageHandler m_Throttle;
};

} // namespace nx::core
