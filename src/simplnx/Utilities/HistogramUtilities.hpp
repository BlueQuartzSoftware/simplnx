#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"

namespace nx::core::HistogramUtilities
{
namespace serial
{
template <typename Type, std::floating_point FloatType>
SIMPLNX_EXPORT Result<> GenerateHistogram(const AbstractDataStore<Type>& inputStore, const std::pair<Type, Type>& rangeMinMax, const std::atomic_bool& shouldCancel, const int32 numBins,
                                          AbstractDataStore<FloatType>& histogramStore, std::atomic<usize>& overflow)
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
    for(usize i = 0; i < end; i++)
    {
      if(shouldCancel)
      {
        return MakeErrorResult(-23761, fmt::format("HistogramUtilities::{}: Signal Interrupt Received. {}:{}", __func__, __FILE__, __LINE__));
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
    }
  }

  for(int32 i = 0; i < numBins; i++)
  {
    histogramStore[i * 2] = static_cast<FloatType>(rangeMinMax.first + (increment * (i + static_cast<FloatType>(1.0)))); // load bin maximum into respective position {(x, ), (x , ), ...}
  }

  if(overflow > 0)
  {
    return MakeWarningVoidResult(-23762, fmt::format("HistogramUtilities::{}: Overflow detected: overflow count {}. {}:{}", __func__, overflow.load(), __FILE__, __LINE__));
  }

  return {};
}

struct SIMPLNX_EXPORT GenerateHistogramFunctor
{
  template <typename Type, class... ArgsT>
  Result<> operator()(const IDataArray* inputArray, ArgsT&&... args) const
  {
    const auto& inputStore = inputArray->template getIDataStoreRefAs<AbstractDataStore<Type>>();

    auto minMax = std::minmax_element(inputStore.begin(), inputStore.end());

    return GenerateHistogram(inputStore, std::make_pair(*minMax.first - static_cast<Type>(1.0), *minMax.second + static_cast<Type>(1.0)), std::forward<ArgsT>(args)...);
  }

  template <typename Type, class... ArgsT>
  Result<> operator()(const IDataArray* inputArray, std::pair<float64, float64>&& rangeMinMax, ArgsT&&... args) const
  {
    const auto& inputStore = inputArray->template getIDataStoreRefAs<AbstractDataStore<Type>>();

    // check range ordering : should be min, max
    if(rangeMinMax.first > rangeMinMax.second)
    {
      return MakeErrorResult(-23760, fmt::format("GenerateHistogramFunctor::{}: The range min value is larger than the max value. Min value: {} | Max Value: {}. {}:{}", __func__, rangeMinMax.first,
                                                 rangeMinMax.second, __FILE__, __LINE__));
    }

    return GenerateHistogram(inputStore, std::make_pair(static_cast<Type>(rangeMinMax.first), static_cast<Type>(rangeMinMax.second)), std::forward<ArgsT>(args)...);
  }
};
} // namespace serial

namespace concurrent
{
template <typename Type, std::floating_point FloatType>
class SIMPLNX_EXPORT GenerateHistogramImpl
{
public:
  GenerateHistogramImpl(const AbstractDataStore<Type>& inputStore, std::pair<float64, float64>&& rangeMinMax, const std::atomic_bool& shouldCancel, const int32 numBins,
                        AbstractDataStore<FloatType>& histogramStore, std::atomic<usize>& overflow)
  : m_InputStore(inputStore)
  , m_ShouldCancel(shouldCancel)
  , m_NumBins(numBins)
  , m_HistogramStore(histogramStore)
  , m_Overflow(overflow)
  {
    m_Range = std::make_pair(static_cast<Type>(rangeMinMax.first), static_cast<Type>(rangeMinMax.second));
  }

  GenerateHistogramImpl(const AbstractDataStore<Type>& inputStore, const std::atomic_bool& shouldCancel, const int32 numBins, AbstractDataStore<FloatType>& histogramStore,
                        std::atomic<usize>& overflow)
  : m_InputStore(inputStore)
  , m_ShouldCancel(shouldCancel)
  , m_NumBins(numBins)
  , m_HistogramStore(histogramStore)
  , m_Overflow(overflow)
  {
    auto minMax = std::minmax_element(m_InputStore.begin(), m_InputStore.end());
    m_Range = std::make_pair(*minMax.first - static_cast<Type>(1.0), *minMax.second + static_cast<Type>(1.0));
  }

  ~GenerateHistogramImpl() = default;

  void operator()() const
  {
    serial::GenerateHistogram(m_InputStore, m_Range, m_ShouldCancel, m_NumBins, m_HistogramStore, m_Overflow);
  }

private:
  const std::atomic_bool& m_ShouldCancel;
  const int32 m_NumBins = 1;
  std::pair<Type, Type> m_Range = {static_cast<Type>(0.0), static_cast<Type>(0.0)};
  const AbstractDataStore<Type>& m_InputStore;
  AbstractDataStore<FloatType>& m_HistogramStore;
  std::atomic<usize>& m_Overflow;
};

struct InstantiateHistogramImplFunctor
{
  template <typename T, class... ArgsT>
  auto operator()(const IDataArray* iDataArray, ArgsT&&... args)
  {
    return GenerateHistogramImpl(iDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>(), std::forward<ArgsT>(args)...);
  }
};
} // namespace concurrent
} // namespace nx::core::HistogramUtilities
