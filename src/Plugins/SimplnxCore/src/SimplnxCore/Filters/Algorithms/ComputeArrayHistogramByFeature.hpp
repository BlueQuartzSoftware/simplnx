#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
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

  void sendThreadSafeProgressMessage(usize calculatedFeatureCount, usize totalFeatures);
  void sendThreadSafeInfoMessage(const std::string& message);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeArrayHistogramByFeatureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  usize m_MilliDelay = 1000;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};
} // namespace nx::core
