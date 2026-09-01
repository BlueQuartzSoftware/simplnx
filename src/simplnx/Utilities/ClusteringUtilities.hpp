#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/simplnx_export.hpp"

#include <algorithm>
#include <numeric>
#include <random>

namespace nx::core::ClusterUtilities
{
/**
 * @enum DistanceMetric
 * @brief Selects the distance calculation for two numeric vectors.
 */
enum DistanceMetric
{
  Euclidean,        ///< Calculates the square root of the squared differences.
  SquaredEuclidean, ///< Calculates the sum of squared differences.
  Manhattan,        ///< Calculates the sum of absolute differences.
  Cosine,           ///< Calculates one minus cosine similarity.
  Pearson,          ///< Calculates one minus Pearson correlation.
  SquaredPearson    ///< Calculates one minus squared Pearson correlation.
};

/**
 * @brief Creates a deterministic shuffled index sequence.
 * @tparam T Specifies the index value type.
 * @param numElements Specifies the sequence length.
 * @param startingValue Specifies the first value before shuffling.
 * @return Shuffled values from startingValue through the sequence end.
 * @pre The complete sequence is representable by T.
 *
 * The fixed default seed makes the sequence reproducible.
 */
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
 * @brief Calculates the selected distance between two numeric vector ranges.
 * @tparam leftDataType Specifies the left random-access container type.
 * @tparam rightDataType Specifies the right random-access container type.
 * @param leftVector Supplies left vector values.
 * @param leftOffset Identifies the first left component.
 * @param rightVector Supplies right vector values.
 * @param rightOffset Identifies the first right component.
 * @param compDims Specifies the common component count.
 * @param distMetric Selects the distance calculation.
 * @return Calculated distance.
 * @pre compDims is nonzero. Both offset ranges contain compDims values.
 * @pre distMetric is a declared DistanceMetric value.
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

  return dist;
}

/**
 * @brief Applies a deterministic random permutation to cell feature IDs.
 * @param featureIds Cell-to-feature ID store to update.
 * @param totalFeatures Total feature count, including feature zero.
 * @pre totalFeatures is nonzero and totalFeatures - 1 fits int32.
 * @pre Each feature ID is in the range [0, totalFeatures). Bulk store operations succeed.
 *
 * Feature zero remains fixed. The supplied count avoids an additional full-store scan.
 */
SIMPLNX_EXPORT void RandomizeFeatureIds(Int32AbstractDataStore& featureIds, usize totalFeatures);

/**
 * @brief Permutes cell feature IDs and applies matching pairwise feature-tuple swaps.
 * @param featureIds Cell-to-feature ID store to update.
 * @param totalFeatures Total feature count, including feature zero.
 * @param featureIArrays Non-owning feature arrays to permute.
 * @pre totalFeatures is nonzero and totalFeatures - 1 fits int32.
 * @pre Each feature ID is in the range [0, totalFeatures). Bulk store operations succeed.
 * @pre Each array pointer is non-null and contains at least totalFeatures tuples.
 *
 * The caller supplies the feature arrays so it can omit arrays that must not move.
 * Feature zero remains fixed. The implementation derives a deterministic swap
 * sequence from the same feature-ID mapping.
 */
SIMPLNX_EXPORT void RandomizeFeatureIds(Int32AbstractDataStore& featureIds, usize totalFeatures, std::vector<IArray*>& featureIArrays);
} // namespace nx::core::ClusterUtilities
