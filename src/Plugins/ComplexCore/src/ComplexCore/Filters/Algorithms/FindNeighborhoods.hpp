#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/IFilter.hpp"

#include <mutex>

namespace complex
{

struct COMPLEXCORE_EXPORT FindNeighborhoodsInputValues
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
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT FindNeighborhoods
{
public:
  FindNeighborhoods(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindNeighborhoodsInputValues* inputValues);
  ~FindNeighborhoods() noexcept;

  FindNeighborhoods(const FindNeighborhoods&) = delete;
  FindNeighborhoods(FindNeighborhoods&&) noexcept = delete;
  FindNeighborhoods& operator=(const FindNeighborhoods&) = delete;
  FindNeighborhoods& operator=(FindNeighborhoods&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void updateNeighborHood(size_t sourceIndex, size_t targetIndex);
  void updateProgress(size_t numCompleted, size_t totalFeatures);

private:
  DataStructure& m_DataStructure;
  const FindNeighborhoodsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::mutex m_Mutex;

  Int32Array* m_Neighborhoods = nullptr;
  std::vector<std::vector<int32_t>> m_LocalNeighborhoodList;
  size_t m_NumCompleted = 0;
  size_t m_ProgIncrement = 0;
  size_t m_IncCount = 0;
};

} // namespace complex
