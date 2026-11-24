#pragma once

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace nx::core::grid_fusion
{
inline constexpr usize k_InvalidIndex = std::numeric_limits<usize>::max();

// TwoSum represents the exact sum as (rounded value, residual). The pair orders exact sums lexicographically.
inline std::pair<float64, float64> SplitSum(float64 left, float64 right)
{
  const float64 sum = left + right;
  const float64 roundedRight = sum - left;
  return {sum, (left - (sum - roundedRight)) + (right - roundedRight)};
}

struct Axis
{
  std::pair<float64, float64> OriginOffset;
  float64 ReferenceSpacing;
  float64 SampleSpacing;
  usize SampleDim;
  bool IdenticalLattice;
};

inline Axis MakeAxis(const ImageGeom& reference, const ImageGeom& sample, usize axis)
{
  const float64 refOrigin = reference.getOrigin()[axis];
  const float64 sampleOrigin = sample.getOrigin()[axis];
  const float64 refSpacing = reference.getSpacing()[axis];
  const float64 sampleSpacing = sample.getSpacing()[axis];
  return {.OriginOffset = SplitSum(refOrigin, -sampleOrigin),
          .ReferenceSpacing = refSpacing,
          .SampleSpacing = sampleSpacing,
          .SampleDim = sample.getDimensions()[axis],
          .IdenticalLattice = refOrigin == sampleOrigin && refSpacing == sampleSpacing};
}

// Compare the physical center with the face before rounding their separation to a source index.
inline bool IsBelowFace(float64 refCenterIndex, const Axis& axis, usize face)
{
  const float64 refStep = refCenterIndex * axis.ReferenceSpacing;
  const auto [center, centerResidual] = SplitSum(axis.OriginOffset.first, refStep);
  const auto faceDistance = SplitSum(std::fma(-static_cast<float64>(face), axis.SampleSpacing, center), centerResidual);
  const auto correction = SplitSum(-axis.OriginOffset.second, -std::fma(refCenterIndex, axis.ReferenceSpacing, -refStep));
  return faceDistance < correction;
}

inline usize GetSourceIndex(const Axis& axis, usize refIndex)
{
  if(axis.IdenticalLattice)
  {
    return refIndex < axis.SampleDim ? refIndex : k_InvalidIndex;
  }
  const float64 refCenterIndex = static_cast<float64>(refIndex) + 0.5;
  const float64 center = std::fma(refCenterIndex, axis.ReferenceSpacing, axis.OriginOffset.first);
  const float64 position = (center + axis.OriginOffset.second) / axis.SampleSpacing;
  const float64 sampleDim = static_cast<float64>(axis.SampleDim);
  constexpr float64 roundoff = 2 * std::numeric_limits<float64>::epsilon();
  if(!std::isfinite(position) || position < 0 || position > sampleDim * (1.0 + roundoff))
  {
    return k_InvalidIndex;
  }
  const usize lower = position < sampleDim ? static_cast<usize>(position) : axis.SampleDim;
  const float64 fraction = position - static_cast<float64>(lower);
  const float64 tolerance = roundoff * position;
  // A rounded multiply-add and division can put a center on the adjacent face. Resolve only that narrow interval.
  if(lower < axis.SampleDim && fraction > tolerance && 1.0 - fraction > tolerance)
  {
    return lower;
  }
  const usize face = fraction >= 0.5 && lower < axis.SampleDim ? lower + 1 : lower;
  if(IsBelowFace(refCenterIndex, axis, face))
  {
    return face == 0 ? k_InvalidIndex : face - 1;
  }
  return face < axis.SampleDim ? face : k_InvalidIndex;
}
} // namespace nx::core::grid_fusion
