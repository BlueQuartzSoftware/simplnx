#pragma once

#include "simplnx/Common/Types.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace StatisticsCalculations
{
enum class HistogramBinPolicy
{
  Raw,
  LegacyInt32
};

/**
 * @brief Classifies a value with the selected legacy arithmetic and conversion sequence.
 * @tparam Policy Raw returns uint8 for bool and T otherwise. LegacyInt32 returns int32.
 */
template <HistogramBinPolicy Policy = HistogramBinPolicy::Raw, typename T>
auto CalculateBin(T value, T minimum, nx::core::float32 increment)
{
  if constexpr(Policy == HistogramBinPolicy::LegacyInt32)
  {
    if constexpr(std::is_signed_v<T> && std::is_integral_v<T>)
    {
      if(value > 0 && minimum < 0 && value > std::numeric_limits<T>::max() + minimum)
      {
        return static_cast<nx::core::int32>(std::floor((static_cast<nx::core::float32>(value) - static_cast<nx::core::float32>(minimum)) / increment));
      }
    }
    return static_cast<nx::core::int32>(CalculateBin<HistogramBinPolicy::Raw>(value, minimum, increment));
  }
  else if constexpr(std::is_same_v<T, bool>)
  {
    return static_cast<nx::core::uint8>(std::floor(static_cast<nx::core::float32>(static_cast<nx::core::uint8>(value) - static_cast<nx::core::uint8>(minimum)) / increment));
  }
  else
  {
    return static_cast<T>(std::floor(static_cast<nx::core::float32>(value - minimum) / increment));
  }
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
T findMin(const C<T, Ts...>& source)
{
  if(source.empty())
  {
    return static_cast<T>(0);
  }
  return (*std::min_element(std::cbegin(source), std::cend(source)));
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
T findMax(const C<T, Ts...>& source)
{
  if(source.empty())
  {
    return static_cast<T>(0);
  }
  return (*std::max_element(std::cbegin(source), std::cend(source)));
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
std::pair<T, T> FindMinMax(const C<T, Ts...>& source)
{
  if(source.empty())
  {
    return {static_cast<T>(0), static_cast<T>(0)};
  }
  // We use minmax_element due to std::vector<bool> specialization with bits...
  const auto [min, max] = std::minmax_element(std::cbegin(source), std::cend(source));
  return {*min, *max};
}

// -----------------------------------------------------------------------------
template <class Container>
auto computeSum(const Container& source)
{
  using T = typename Container::value_type;
  if constexpr(std::is_integral_v<T>)
  {
    if constexpr(std::is_signed_v<T>)
    {
      return std::accumulate(std::cbegin(source), std::cend(source), static_cast<int64_t>(0));
    }
    else
    {
      return std::accumulate(std::cbegin(source), std::cend(source), static_cast<uint64_t>(0));
    }
  }
  else
  {
    return std::accumulate(std::cbegin(source), std::cend(source), 0.0);
  }
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
float findMean(const C<T, Ts...>& source)
{
  if(source.empty())
  {
    return 0.0f;
  }
  float sum = static_cast<float>(computeSum(source));

  return sum / static_cast<float>(source.size());
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename... Ts>
bool findMean(const C<bool, Ts...>& source)
{
  if(source.empty())
  {
    return false;
  }
  size_t count = std::count(std::cbegin(source), std::cend(source), true);
  return count >= (source.size() - count);
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
std::pair<float, float> FindSumMean(const C<T, Ts...>& source)
{
  if(source.empty())
  {
    return {0.0f, 0.0f};
  }
  float sum = static_cast<float>(computeSum(source));
  return {sum, sum / static_cast<float>(source.size())};
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
float findMedian(const C<T, Ts...>& source)
{
  // Need a copy, not a reference, since we will be sorting the input array
  std::vector<T> tmpList{std::cbegin(source), std::cend(source)};
  if(tmpList.empty())
  {
    return 0.0f;
  }
  std::sort(tmpList.begin(), tmpList.end());
  float medVal = 0.0f;
  if(tmpList.size() % 2 == 1)
  {
    size_t halfElements = static_cast<size_t>(std::floor(tmpList.size() / 2.0f));
    medVal = tmpList[halfElements];
  }
  else
  {
    size_t idxLow = (tmpList.size() / 2) - 1;
    size_t idxHigh = tmpList.size() / 2;
    medVal = (tmpList[idxLow] + tmpList[idxHigh]) * 0.5f;
  }
  return medVal;
}

// -----------------------------------------------------------------------------
template <class Container, typename T>
std::vector<T> computeMode(const Container& source)
{
  std::map<T, uint64_t> modalMap;
  for(const auto val : source)
  {
    modalMap[val]++;
  }

  // Find the maximum occurrence
  auto pr = std::max_element(modalMap.begin(), modalMap.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
  int maxCount = pr->second;

  // Store all values that have this maximum occurrence under the proper feature id
  std::vector<T> modes;
  for(const auto& modalPair : modalMap)
  {
    if(modalPair.second == maxCount)
    {
      modes.push_back(modalPair.first);
    }
  }

  return modes;
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
std::vector<T> findModes(const C<T, Ts...>& source)
{
  if(source.empty())
  {
    return {};
  }

  return computeMode<C<T, Ts...>, T>(source);
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
float findStdDeviation(const C<T, Ts...>& source)
{
  if(source.empty())
  {
    return 0.0f;
  }
  std::vector<double> difference(source.size());

  const std::pair<float, float> sumMeanValues = FindSumMean(source);

  std::transform(std::cbegin(source), std::cend(source), std::begin(difference), [sumMeanValues](T a) { return static_cast<double>(a - sumMeanValues.second); });
  const double squaredSum = std::inner_product(std::cbegin(difference), std::cend(difference), std::cbegin(difference), 0.0);
  return static_cast<float>(std::sqrt(squaredSum / static_cast<double>(source.size())));
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename... Ts>
bool findStdDeviation(const C<bool, Ts...>& source)
{
  if(source.empty())
  {
    return false;
  }
  size_t count = std::count(std::cbegin(source), std::cend(source), true);
  return count >= (source.size() - count);
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
float FindStdDeviation(const C<T, Ts...>& source, const std::pair<float, float> sumMeanValues)
{
  if(source.empty())
  {
    return 0.0f;
  }
  std::vector<double> difference(source.size());
  std::transform(std::cbegin(source), std::cend(source), std::begin(difference), [sumMeanValues](T a) { return static_cast<double>(a - sumMeanValues.second); });
  const double squaredSum = std::inner_product(std::cbegin(difference), std::cend(difference), std::cbegin(difference), 0.0);
  return static_cast<float>(std::sqrt(squaredSum / static_cast<double>(source.size())));
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
double findSummation(const C<T, Ts...>& source)
{
  if(source.empty())
  {
    return 0.0f;
  }
  float sum = static_cast<float>(computeSum(source));
  return sum;
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
size_t findNumUniqueValues(const C<T, Ts...>& source)
{
  if(source.empty())
  {
    return 0;
  }
  std::set<T> sourceSet(std::begin(source), std::end(source));
  return sourceSet.size();
}

// -----------------------------------------------------------------------------
template <template <typename, typename...> class C, typename T, typename... Ts>
std::pair<T, T> findHistogramRange(const C<T, Ts...>& source, T histmin, T histmax, bool histfullrange)
{
  if(histfullrange)
  {
    return FindMinMax(source);
  }

  return {histmin, histmax};
}

/**
 * @brief Returns the stored lower/upper pair selected by a mode's legacy bin classifier.
 * @tparam Policy Classification arithmetic and conversion policy.
 * @tparam T Input and stored range value type.
 * @param mode Value whose bin range is requested.
 * @param histMin Histogram minimum used during counting.
 * @param increment Counting bin width.
 * @param numBins Counting bin count.
 * @param binRanges Stored lower/upper pairs for one histogram.
 * @pre binRanges.size() >= usize{2} * static_cast<usize>(numBins).
 * @return The stored pair, or nullopt for a degenerate width or an out-of-bin mode.
 *
 * The lookup allocates no memory and performs no I/O. Callers retain their own eligibility
 * checks and degenerate-width output policy. Duplicate modes can select the same pair.
 */
template <HistogramBinPolicy Policy = HistogramBinPolicy::Raw, typename T>
std::optional<std::pair<T, T>> findModalBinRange(T mode, T histMin, nx::core::float32 increment, nx::core::int32 numBins, nonstd::span<const T> binRanges)
{
  using nx::core::usize;
  assert(binRanges.size() >= usize{2} * static_cast<usize>(numBins));
  if(std::fabs(increment) < 1E-10)
  {
    return std::nullopt;
  }
  const auto bin = CalculateBin<Policy>(mode, histMin, increment);
  if((bin >= 0) && (bin < numBins))
  {
    const usize offset = usize{2} * static_cast<usize>(bin);
    return std::pair<T, T>{binRanges[offset], binRanges[offset + usize{1}]};
  }
  return std::nullopt;
}
} // namespace StatisticsCalculations
