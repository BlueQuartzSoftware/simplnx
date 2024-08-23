#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"

namespace nx::core::HistogramUtilities
{
template <typename Type, std::floating_point FloatType>
SIMPLNX_EXPORT Result<> GenerateHistogram(const AbstractDataStore<Type>& inputStore, const std::pair<Type, Type>& rangeMinMax, const std::function<void(usize)>& progressReportingFunction,
                                          const std::atomic_bool& shouldCancel, const int32 numBins, AbstractDataStore<FloatType>& histogramStore, std::atomic<usize>& overflow,
                                          size_t progressIncrement)
{
  auto end = inputStore.getSize();

  const Type increment = (rangeMinMax.second - rangeMinMax.first) / static_cast<Type>(numBins);
  if(numBins == 1) // if one bin, just set the first element to total number of points
  {
    histogramStore[0] = rangeMinMax.second;
    histogramStore[1] = end;
  }
  else
  {
    usize progressCounter = 0;
    for(usize i = 0; i < end; i++)
    {
      if(shouldCancel)
      {
        return MakeErrorResult(-23761, fmt::format("HistogramUtilities::{}: Signal Interrupt Received. {}:{}", __func__, __FILE__, __LINE__));
      }
      if(progressCounter > progressIncrement)
      {
        progressReportingFunction(progressCounter);
        progressCounter = 0;
      }
      const auto bin = std::floor((inputStore[i] - rangeMinMax.first) / increment);
      if((bin >= 0) && (bin < numBins))
      {
        histogramStore[bin * 2 + 1]++;
      }
      else
      {
        overflow++;
      }
      progressCounter++;
    }
  }

  for(int64 i = 0; i < numBins; i++)
  {
    histogramStore[(i * 2)] = static_cast<float64>(rangeMinMax.first + (increment * (static_cast<float64>(i) + 1.0))); // load bin maximum into respective position {(x, ), (x , ), ...}
  }

  if(overflow > 0)
  {
    return MakeWarningVoidResult(-23762, fmt::format("HistogramUtilities::{}: Overflow detected: overflow count {}. {}:{}", __func__, overflow, __FILE__, __LINE__));
  }

  return {};
}

struct SIMPLNX_EXPORT GenerateHistogramFunctor
{
  template <typename Type, class... ArgsT>
  Result<> operator()(const IDataArray* inputArray, ArgsT... args)
  {
    const auto& inputStore = inputArray->template getIDataStoreRefAs<AbstractDataStore<Type>>();

    auto minMax = std::minmax_element(inputStore.begin(), inputStore.end());

    GenerateHistogram(inputStore, std::make_pair(static_cast<Type>(*minMax.first) - 1, static_cast<Type>(*minMax.second) + 1), std::forward<ArgsT>(args)...);
  }

  template <typename Type, class... ArgsT>
  Result<> operator()(const IDataArray* inputArray, const std::pair<Type, Type>& rangeMinMax, ArgsT... args)
  {
    const auto& inputStore = inputArray->template getIDataStoreRefAs<AbstractDataStore<Type>>();

    // check range ordering : should be min, max
    if(rangeMinMax.first > rangeMinMax.second)
    {
      return MakeErrorResult(-23760, fmt::format("GenerateHistogramFunctor::{}: The range min value is larger than the max value. Min value: {} | Max Value: {}. {}:{}", __func__, rangeMinMax.first,
                                                 rangeMinMax.second, __FILE__, __LINE__));
    }

    GenerateHistogram(inputStore, rangeMinMax, std::forward<ArgsT>(args)...);
  }
};
} // namespace nx::core::HistogramUtilities
