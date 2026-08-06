#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <mutex>
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT ComputeArrayHistogramByFeatureInputValues
{
  bool UserDefinedRange = false;
  int32 NumberOfBins = 0;
  float64 MinRange = 0.0;
  float64 MaxRange = 0.0;
  MultiArraySelectionParameter::ValueType SelectedArrayPaths = {};
  MultiArraySelectionParameter::ValueType CreatedBinRangeDataPaths = {};
  MultiArraySelectionParameter::ValueType CreatedHistogramCountsDataPaths = {};
  MultiArraySelectionParameter::ValueType CreatedBinMostPopulatedDataPaths = {};
  std::optional<MultiArraySelectionParameter::ValueType> CreatedBinModalRangesDataPaths;
  DataPath FeatureIdsArrayPath;
  bool UseMask;
  DataPath MaskArrayPath;
};

/**
 * @class ComputeArrayHistogramByFeature
 * @brief This filter calculates a Histogram according to user specification and stores it accordingly
 */
class SIMPLNXCORE_EXPORT ComputeArrayHistogramByFeature
{
public:
  ComputeArrayHistogramByFeature(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeArrayHistogramByFeatureInputValues* inputValues);
  ~ComputeArrayHistogramByFeature() noexcept;

  ComputeArrayHistogramByFeature(const ComputeArrayHistogramByFeature&) = delete;
  ComputeArrayHistogramByFeature(ComputeArrayHistogramByFeature&&) noexcept = delete;
  ComputeArrayHistogramByFeature& operator=(const ComputeArrayHistogramByFeature&) = delete;
  ComputeArrayHistogramByFeature& operator=(ComputeArrayHistogramByFeature&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief Thread-safe progress update. Safe to call from ParallelDataAlgorithm workers.
   * @param counter Features completed since the previous call
   */
  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const ComputeArrayHistogramByFeatureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
};
} // namespace nx::core
