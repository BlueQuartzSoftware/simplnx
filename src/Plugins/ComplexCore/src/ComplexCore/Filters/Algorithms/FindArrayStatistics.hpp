#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT FindArrayStatisticsInputValues
{
  bool FindHistogram;
  float64 MinRange;
  float64 MaxRange;
  bool UseFullRange;
  int32 NumBins;
  bool FindLength;
  bool FindMin;
  bool FindMax;
  bool FindMean;
  bool FindMedian;
  bool FindStdDeviation;
  bool FindSummation;
  bool UseMask;
  bool ComputeByIndex;
  bool StandardizeData;
  DataPath SelectedArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath MaskArrayPath;
  DataPath DestinationAttributeMatrix;
  DataPath HistogramArrayName;
  DataPath LengthArrayName;
  DataPath MinimumArrayName;
  DataPath MaximumArrayName;
  DataPath MeanArrayName;
  DataPath MedianArrayName;
  DataPath StdDeviationArrayName;
  DataPath SummationArrayName;
  DataPath StandardizedArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT FindArrayStatistics
{
public:
  FindArrayStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindArrayStatisticsInputValues* inputValues);
  ~FindArrayStatistics() noexcept;

  FindArrayStatistics(const FindArrayStatistics&) = delete;
  FindArrayStatistics(FindArrayStatistics&&) noexcept = delete;
  FindArrayStatistics& operator=(const FindArrayStatistics&) = delete;
  FindArrayStatistics& operator=(FindArrayStatistics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  static usize FindNumFeatures(const Int32Array& featureIds);

private:
  DataStructure& m_DataStructure;
  const FindArrayStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
