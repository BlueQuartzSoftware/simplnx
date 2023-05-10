#pragma once

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

#include <cmath>

namespace complex::KUtilities
{
enum COMPLEX_EXPORT DistanceMetric
{
  Euclidean,
  SquaredEuclidean,
  Manhattan,
  Cosine,
  Pearson,
  SquaredPearson
};

/**
 * @brief The DistanceTemplate class contains a templated function getDistance to find the distance, via a variety of
 * metrics, between two vectors of arbitrary dimensions. The developer should ensure that the pointers passed to
 * getDistance do indeed contain vectors of the same component dimensions and start at the desired tuples.
 */
template <typename leftDataType, typename rightDataType>
auto GetDistance(const leftDataType& leftVector, usize leftOffset, const rightDataType& rightVector, usize rightOffset, usize compDims, DistanceMetric distMetric)
{
  float64 dist = 0.0;
  float64 lVal = 0.0;
  float64 rVal = 0.0;

  float64 epsilon = std::numeric_limits<float64>::min();

  switch(distMetric)
  {
  case Euclidean:
    {
      for(usize i = 0; i < compDims; i++)
      {
        lVal = static_cast<float64>(leftVector[i + leftOffset]);
        rVal = static_cast<float64>(rightVector[i + rightOffset]);
        dist += (lVal - rVal) * (lVal - rVal);
      }

      dist = std::sqrt(dist);
    }
    case SquaredEuclidean:
    {
      for(usize i = 0; i < compDims; i++)
      {
        lVal = static_cast<float64>(leftVector[i + leftOffset]);
        rVal = static_cast<float64>(rightVector[i + rightOffset]);
        dist += (lVal - rVal) * (lVal - rVal);
      }
    }
    case Manhattan:
    {
      for(usize i = 0; i < compDims; i++)
      {
        lVal = static_cast<float64>(leftVector[i + leftOffset]);
        rVal = static_cast<float64>(rightVector[i + rightOffset]);
        dist += std::abs(lVal - rVal);
      }
    }
    case Cosine:
    {
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
    }
    case Pearson:
    {
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
    }
    case SquaredPearson:
    {
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
    }
  }

  // Return the correct primitive type for distance
  return dist;
}
} // namespace complex::KUtilities
