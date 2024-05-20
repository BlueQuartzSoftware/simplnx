#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <mutex>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeNeighborhoodsInputValues
{
  float32 MultiplesOfAverage;
  DataPath EquivalentDiametersArrayPath;
  DataPath FeaturePhasesArrayPath;
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

  const std::atomic_bool& getCancel();

  void updateNeighborHood(usize sourceIndex, usize targetIndex);
  void updateProgress(float64 counter, const std::chrono::steady_clock::time_point& now = std::chrono::steady_clock::now());

private:
  DataStructure& m_DataStructure;
  const ComputeNeighborhoodsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::mutex m_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
  float64 m_TotalFeatures = 0;
  float64 m_ProgressCounter = 0;

  Int32Array* m_Neighborhoods = nullptr;
  std::vector<std::vector<int32_t>> m_LocalNeighborhoodList;
};

} // namespace nx::core
