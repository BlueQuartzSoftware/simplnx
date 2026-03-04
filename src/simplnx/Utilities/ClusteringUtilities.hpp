#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractArray.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/simplnx_export.hpp"

#include <algorithm>
#include <numeric>
#include <random>

namespace nx::core::ClusterUtilities
{
enum DistanceMetric
{
  Euclidean,
  SquaredEuclidean,
  Manhattan,
  Cosine,
  Pearson,
  SquaredPearson
};

template <typename T>
std::vector<T> CreateRandomizedIndex(usize numElements, T startingValue)
{
  auto generator = std::mt19937_64(std::mt19937_64::default_seed);
  std::vector<T> randomIds(numElements);
  std::iota(randomIds.begin(), randomIds.end(), startingValue);
  std::shuffle(randomIds.begin(), randomIds.end(), generator);
  return randomIds;
}

/**
 * @brief The DistanceTemplate class contains a templated function getDistance to find the distance, via a variety of
 * metrics, between two vectors of arbitrary dimensions. The developer should ensure that the pointers passed to
 * getDistance do indeed contain vectors of the same component dimensions and start at the desired tuples.
 */
template <typename leftDataType, typename rightDataType>
float64 GetDistance(const leftDataType& leftVector, usize leftOffset, const rightDataType& rightVector, usize rightOffset, usize compDims, DistanceMetric distMetric)
{
  float64 dist = 0.0;
  float64 lVal = 0.0;
  float64 rVal = 0.0;

  float64 epsilon = std::numeric_limits<float64>::min();

  switch(distMetric)
  {
  case Euclidean: {
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      dist += (lVal - rVal) * (lVal - rVal);
    }

    dist = std::sqrt(dist);
    break;
  }
  case SquaredEuclidean: {
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      dist += (lVal - rVal) * (lVal - rVal);
    }
    break;
  }
  case Manhattan: {
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      dist += std::abs(lVal - rVal);
    }
    break;
  }
  case Cosine: {
    float64 r = 0;
    float64 x = 0;
    float64 y = 0;
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      r += lVal * rVal;
      x += lVal * lVal;
      y += rVal * rVal;
    }
    dist = 1 - (r / (sqrt(x * y) + epsilon));
    break;
  }
  case Pearson: {
    float64 r = 0;
    float64 x = 0;
    float64 y = 0;
    float64 xAvg = 0;
    float64 yAvg = 0;
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      xAvg += lVal;
      yAvg += rVal;
    }
    xAvg /= static_cast<float64>(compDims);
    yAvg /= static_cast<float64>(compDims);
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      r += (lVal - xAvg) * (rVal - yAvg);
      x += (lVal - xAvg) * (lVal - xAvg);
      y += (rVal - yAvg) * (rVal - yAvg);
    }
    dist = 1 - (r / (sqrt(x * y) + epsilon));
    break;
  }
  case SquaredPearson: {
    float64 r = 0;
    float64 x = 0;
    float64 y = 0;
    float64 xAvg = 0;
    float64 yAvg = 0;
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      xAvg += lVal;
      yAvg += rVal;
    }
    xAvg /= static_cast<float64>(compDims);
    yAvg /= static_cast<float64>(compDims);
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      r += (lVal - xAvg) * (rVal - yAvg);
      x += (lVal - xAvg) * (lVal - xAvg);
      y += (rVal - yAvg) * (rVal - yAvg);
    }
    dist = 1 - ((r * r) / ((x * y) + epsilon));
    break;
  }
  }

  // Return the correct primitive type for distance
  return dist;
}

/**
 * @brief Randomize the provided Feature IDs.
 * @param featureIds the array that maps cell data to feature data via IDs
 * @param totalFeatures the total feature count in the feature ids array (equivalent to max value in feature ids + 1)
 * (Note: this can be derived implicitly, however,the calling functions should have already have this value so we are saving additional parsing)
 * @return void
 */
SIMPLNX_EXPORT void RandomizeFeatureIds(Int32AbstractDataStore& featureIds, usize totalFeatures);

/**
 * @brief Randomize the provided Feature IDs and update supplied feature AbstractArray data.
 * Assumption: Every array in `featureIArrays` are at least the length of totalFeatures
 * @param featureIds the array that maps cell data to feature data via IDs
 * @param totalFeatures the total feature count in the feature ids array (equivalent to max value in feature ids + 1)
 * (Note: this can be derived implicitly, however,the calling functions should have already have this value so we are saving additional parsing)
 * @param featureIArrays a vector of pointers to the IArrays in the Feature Attribute Matrix
 * (Note: These are not found from a datapath, so calling functions can threshold out feature arrays if necessary)
 * @return void
 */
SIMPLNX_EXPORT void RandomizeFeatureIds(Int32AbstractDataStore& featureIds, usize totalFeatures, std::vector<AbstractArray*>& featureIArrays);
} // namespace nx::core::ClusterUtilities
