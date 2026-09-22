#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <mutex>

namespace nx::core
{

/**
 * @struct ComputeArrayStatisticsInputValues
 * @brief Defines requested statistics, grouping, mask, ranges, and output paths.
 */
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
  DataPath FeatureIdMapArrayPath;
};

/**
 * @class ComputeArrayStatistics
 * @brief Computes selected statistics for a scalar array.
 *
 * Available measures include length, range, mean, median, mode, standard
 * deviation, summation, and unique count. Feature or ensemble IDs can group the
 * calculation.
 *
 * In-memory inputs use the original direct implementation. If any enabled
 * input or output is out-of-core, the algorithm uses bounded bulk reads and
 * writes. Exact median, mode, and unique-value calculations use the registered
 * external-sort capability, with an exact bounded multi-pass fallback when no
 * provider is available.
 */
class SIMPLNXCORE_EXPORT ComputeArrayStatistics
{
public:
  /**
   * @brief Initializes array-statistics calculation.
   * @param dataStructure Owns all inputs and precreated outputs.
   * @param msgHandler Receives phase progress from the bounded implementation.
   * @param shouldCancel Checked between bounded pages and expensive reduction passes.
   * @param inputValues Defines requested calculations and paths.
   * @pre All arguments outlive this executor.
   */
  ComputeArrayStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeArrayStatisticsInputValues* inputValues);
  ~ComputeArrayStatistics() noexcept;

  ComputeArrayStatistics(const ComputeArrayStatistics&) = delete;
  ComputeArrayStatistics(ComputeArrayStatistics&&) noexcept = delete;
  ComputeArrayStatistics& operator=(const ComputeArrayStatistics&) = delete;
  ComputeArrayStatistics& operator=(ComputeArrayStatistics&&) noexcept = delete;

  /**
   * @enum FeatureIdRangeControls
   * @brief Selects feature-ID range handling.
   *
   * Numeric values are sequence-dependent and must remain stable.
   */
  enum FeatureIdRangeControls : uint8
  {
    None = 0,
    IgnoreZero = 1,
    ShrinkToFit = 2,
    PaddedCustomRange = 3,
    CustomRange = 4
  };

  /**
   * @brief Resolves the requested feature layout and dispatches the resident or
   * bounded algorithm according to the participating stores.
   * @return The first validation, storage, or reduction error; cancellation is
   * reported as a valid early return consistent with filter execution.
   */
  Result<> operator()();

  /**
   * @brief Resets aggregate feature-statistics progress before workers start.
   * @param totalWork Total feature-tuple scans and feature finalizations.
   */
  void resetProgress(usize totalWork);

  /**
   * @brief Adds completed work under the progress mutex.
   * @param completedWork Completed feature-tuple scans or feature finalizations.
   */
  void sendThreadSafeProgressMessage(usize completedWork);

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
