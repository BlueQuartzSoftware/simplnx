#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <mutex>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeNeighborhoodsInputValues
{
  uint64 SearchRadiusType;
  float32 MultiplesOfAverage;
  float32 SearchRadius;
  DataPath EquivalentDiametersArrayPath;
  DataPath CentroidsArrayPath;
  DataPath NeighborhoodsArrayName;
  DataPath NeighborhoodListArrayName;
  DataPath InputImageGeometry;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeNeighborhoods
{
public:
  ComputeNeighborhoods(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeNeighborhoodsInputValues* inputValues);
  ~ComputeNeighborhoods() noexcept;

  ComputeNeighborhoods(const ComputeNeighborhoods&) = delete;
  ComputeNeighborhoods(ComputeNeighborhoods&&) noexcept = delete;
  ComputeNeighborhoods& operator=(const ComputeNeighborhoods&) = delete;
  ComputeNeighborhoods& operator=(ComputeNeighborhoods&&) noexcept = delete;

  Result<> operator()();

  void updateNeighborHood(usize sourceIndex, usize targetIndex);

  /**
   * @brief Thread-safe progress update. Safe to call from ParallelDataAlgorithm workers.
   * @param counter Items completed since the previous call
   */
  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const ComputeNeighborhoodsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  std::mutex m_Mutex;
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
  Int32Array* m_Neighborhoods = nullptr;
  std::vector<std::vector<int32_t>> m_LocalNeighborhoodList;
};

} // namespace nx::core
