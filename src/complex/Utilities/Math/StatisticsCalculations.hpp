#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>
#include <type_traits>
#include <vector>

namespace StatisticsCalculations
{
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
std::vector<uint64_t> findHistogram(const C<T, Ts...>& source, float histmin, float histmax, bool histfullrange, int32_t numBins)
{
  if(source.empty())
  {
    return std::vector<uint64_t>(numBins, 0);
  }

  float min = 0.0f;
  float max = 0.0f;

  if(histfullrange)
  {
    min = static_cast<float>(findMin(source));
    max = static_cast<float>(findMax(source));
  }
  else
  {
    min = histmin;
    max = histmax;
  }

  const float increment = (max - min) / (numBins);
  if(std::abs(increment) < 1E-10)
  {
    numBins = 1;
  }

  std::vector<uint64_t> histogram(numBins, 0);

  if(numBins == 1) // if one bin, just set the first element to total number of points
  {
    histogram[0] = static_cast<uint64_t>(source.size());
  }
  else
  {
    for(const auto s : source)
    {
      float value = static_cast<float>(s);
      const size_t bin = static_cast<size_t>((value - min) / increment); // find bin for this input array value
      if((bin >= 0) && (bin < numBins))                                  // make certain bin is in range
      {
        histogram[bin]++; // increment histogram element corresponding to this input array value
      }
      else if(value == max)
      {
        histogram[numBins - 1]++;
      }
    }
  }

  return histogram;
}
} // namespace StatisticsCalculations
