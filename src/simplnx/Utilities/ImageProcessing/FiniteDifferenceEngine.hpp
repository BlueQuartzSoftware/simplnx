#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp" // CreateDataStoreWithFormat (update buffer)
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp" // CalculateChange in-plane parallelization

#include <fmt/core.h>

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
inline std::string SelectFiniteDifferenceWorkingDataFormat(IDataStore::StoreType inputStoreType, std::string inputDataFormat, IDataStore::StoreType outputStoreType, std::string outputDataFormat)
{
  if(inputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return inputDataFormat;
  }
  if(outputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return outputDataFormat;
  }
  return inputDataFormat;
}

constexpr usize k_FiniteDifference2DResidentLimit = 64ULL * 1024ULL * 1024ULL;
constexpr usize k_FiniteDifference2DFixedStateBytes = 4096;

struct FiniteDifference2DBufferPlan
{
  usize coreRows = 0;
  usize coreCols = 0;
  usize residentBytes = 0;
  bool valid = false;
  bool overflow = false;
};

inline bool FiniteDifferenceCheckedAdd(usize left, usize right, usize& result)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  result = left + right;
  return true;
}

inline bool FiniteDifferenceCheckedMultiply(usize left, usize right, usize& result)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  result = left * right;
  return true;
}

inline bool FiniteDifferenceNeighborhoodIsInterior(usize x, usize y, usize z, const SizeVec3& dims, int64 radius, uint32 effectiveDimensions)
{
  if(radius < 0 || (effectiveDimensions != 2 && effectiveDimensions != 3))
  {
    return false;
  }
  const usize unsignedRadius = static_cast<usize>(radius);
  const auto axisIsInterior = [unsignedRadius](usize coordinate, usize dimension) { return coordinate < dimension && unsignedRadius <= coordinate && unsignedRadius < dimension - coordinate; };
  return axisIsInterior(x, dims[0]) && axisIsInterior(y, dims[1]) && (effectiveDimensions == 2 || axisIsInterior(z, dims[2]));
}

struct FiniteDifferenceInteriorRange
{
  usize begin = 0;
  usize end = 0;
};

inline FiniteDifferenceInteriorRange CreateFiniteDifferenceInteriorRange(usize dimension, int64 radius)
{
  if(radius < 0)
  {
    return {};
  }
  const usize unsignedRadius = static_cast<usize>(radius);
  if(unsignedRadius > dimension || unsignedRadius >= dimension - unsignedRadius)
  {
    return {};
  }
  return {unsignedRadius, dimension - unsignedRadius};
}

struct FiniteDifferenceResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

inline bool ShouldUseFiniteDifferenceResidentState(const SizeVec3& dims, uint32 numberOfIterations)
{
  return dims[2] > 1 && numberOfIterations > 0;
}

template <class T, class Real>
Result<usize> CalculateFiniteDifferenceResidentWorkingMemoryBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize inputOutputBytesPerValue = 0;
  usize workingBytesPerValue = 0;
  usize bytesPerVolumeValue = 0;
  usize volumeBytes = 0;
  usize planeBytesPerValue = 0;
  usize planeBytes = 0;
  usize requiredBytes = 0;
  if(!FiniteDifferenceCheckedMultiply(dims[0], dims[1], sliceValues) || !FiniteDifferenceCheckedMultiply(sliceValues, dims[2], volumeValues) ||
     !FiniteDifferenceCheckedMultiply(sizeof(T), usize{2}, inputOutputBytesPerValue) || !FiniteDifferenceCheckedMultiply(sizeof(Real), usize{2}, workingBytesPerValue) ||
     !FiniteDifferenceCheckedAdd(inputOutputBytesPerValue, workingBytesPerValue, bytesPerVolumeValue) || !FiniteDifferenceCheckedMultiply(volumeValues, bytesPerVolumeValue, volumeBytes) ||
     !FiniteDifferenceCheckedAdd(sizeof(T), sizeof(Real), planeBytesPerValue))
  {
    return MakeErrorResult<usize>(-8767, fmt::format("Finite-difference dimensions {} x {} x {}, {}-byte input values, and {}-byte working values overflow while sizing the resident state.", dims[0],
                                                     dims[1], dims[2], sizeof(T), sizeof(Real)));
  }
  usize conductancePlaneBytesPerValue = 0;
  if(!FiniteDifferenceCheckedMultiply(sizeof(Real), usize{3}, conductancePlaneBytesPerValue) ||
     !FiniteDifferenceCheckedMultiply(sliceValues, std::max(planeBytesPerValue, conductancePlaneBytesPerValue), planeBytes) || !FiniteDifferenceCheckedAdd(volumeBytes, planeBytes, requiredBytes))
  {
    return MakeErrorResult<usize>(-8767, fmt::format("Finite-difference dimensions {} x {} x {}, {}-byte input values, and {}-byte working values overflow while sizing the resident state.", dims[0],
                                                     dims[1], dims[2], sizeof(T), sizeof(Real)));
  }
  return {requiredBytes};
}

template <class T, class Real>
Result<FiniteDifferenceResidentMemoryAllocation> ReserveFiniteDifferenceResidentWorkingMemory(const SizeVec3& dims)
{
  auto requiredResult = CalculateFiniteDifferenceResidentWorkingMemoryBytes<T, Real>(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<FiniteDifferenceResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {FiniteDifferenceResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

inline bool FiniteDifference2DFullWidthPeak(usize columns, usize rows, usize radius, usize valueBytes, usize& peak)
{
  usize halo = 0;
  usize inputRows = 0;
  usize inputCells = 0;
  usize outputCells = 0;
  usize totalCells = 0;
  usize valuesBytes = 0;
  return FiniteDifferenceCheckedMultiply(radius, 2, halo) && FiniteDifferenceCheckedAdd(rows, halo, inputRows) && FiniteDifferenceCheckedMultiply(columns, inputRows, inputCells) &&
         FiniteDifferenceCheckedMultiply(columns, rows, outputCells) && FiniteDifferenceCheckedAdd(inputCells, outputCells, totalCells) &&
         FiniteDifferenceCheckedMultiply(totalCells, valueBytes, valuesBytes) && FiniteDifferenceCheckedAdd(valuesBytes, k_FiniteDifference2DFixedStateBytes, peak);
}

inline bool FiniteDifference2DTiledPeak(usize columns, usize radius, usize valueBytes, usize& peak)
{
  usize halo = 0;
  usize inputRows = 0;
  usize inputCols = 0;
  usize inputCells = 0;
  usize totalCells = 0;
  usize valuesBytes = 0;
  return FiniteDifferenceCheckedMultiply(radius, 2, halo) && FiniteDifferenceCheckedAdd(1, halo, inputRows) && FiniteDifferenceCheckedAdd(columns, halo, inputCols) &&
         FiniteDifferenceCheckedMultiply(inputRows, inputCols, inputCells) && FiniteDifferenceCheckedAdd(inputCells, columns, totalCells) &&
         FiniteDifferenceCheckedMultiply(totalCells, valueBytes, valuesBytes) && FiniteDifferenceCheckedAdd(valuesBytes, k_FiniteDifference2DFixedStateBytes, peak);
}

inline FiniteDifference2DBufferPlan BuildFiniteDifference2DBufferPlan(usize nx, usize ny, usize radius, usize valueBytes, usize residentLimit = k_FiniteDifference2DResidentLimit)
{
  FiniteDifference2DBufferPlan plan;
  usize cellCount = 0;
  usize halo = 0;
  if(nx == 0 || ny == 0 || valueBytes == 0 || residentLimit == 0 || nx > static_cast<usize>(std::numeric_limits<int64>::max()) || ny > static_cast<usize>(std::numeric_limits<int64>::max()) ||
     !FiniteDifferenceCheckedMultiply(nx, ny, cellCount) || !FiniteDifferenceCheckedMultiply(radius, 2, halo))
  {
    plan.overflow = true;
    return plan;
  }

  usize minimumPeak = 0;
  if(!FiniteDifference2DTiledPeak(1, radius, valueBytes, minimumPeak))
  {
    plan.overflow = true;
    return plan;
  }
  if(minimumPeak > residentLimit)
  {
    return plan;
  }

  auto largestFitting = [residentLimit](usize high, const auto& peakFunction) {
    usize low = 1;
    while(low < high)
    {
      const usize middle = low + (high - low + 1) / 2;
      usize candidatePeak = 0;
      if(peakFunction(middle, candidatePeak) && candidatePeak <= residentLimit)
      {
        low = middle;
      }
      else
      {
        high = middle - 1;
      }
    }
    return low;
  };

  usize oneRowPeak = 0;
  if(FiniteDifference2DFullWidthPeak(nx, 1, radius, valueBytes, oneRowPeak) && oneRowPeak <= residentLimit)
  {
    plan.coreCols = nx;
    plan.coreRows = largestFitting(ny, [nx, radius, valueBytes](usize rows, usize& peak) { return FiniteDifference2DFullWidthPeak(nx, rows, radius, valueBytes, peak); });
    if(!FiniteDifference2DFullWidthPeak(nx, plan.coreRows, radius, valueBytes, plan.residentBytes))
    {
      plan.overflow = true;
      return plan;
    }
  }
  else
  {
    plan.coreCols = largestFitting(nx, [radius, valueBytes](usize columns, usize& peak) { return FiniteDifference2DTiledPeak(columns, radius, valueBytes, peak); });
    plan.coreRows = 1;
    if(!FiniteDifference2DTiledPeak(plan.coreCols, radius, valueBytes, plan.residentBytes))
    {
      plan.overflow = true;
      return plan;
    }
  }
  plan.valid = plan.residentBytes <= residentLimit;
  return plan;
}

// Curvature-flow update functor (radius 1, no global data). See the Parity model (itkCurvatureFlowFunction).
struct CurvatureFlowFn
{
  int64 radius() const
  {
    return 1;
  }
  static constexpr bool k_NeedsGlobalGradient = false;
  // The legacy ITK bridge functor declares IntermediateType=float64 for this filter, so ApplyFiniteDifference's
  // Real stays float64 regardless of T (see CurvatureAnisoFn below, the one functor where this is true).
  static constexpr bool k_NativePrecision = false;

  // sc = per-axis scale (1/spacing). get(dx,dy,dz) reads the clamped neighbor as double. effDim in {2,3}.
  template <class GetFn>
  double computeUpdate(const GetFn& get, const std::array<double, 3>& sc, uint32 effDim) const
  {
    double first[3] = {0.0, 0.0, 0.0};
    double sec[3] = {0.0, 0.0, 0.0};
    double cross[3][3] = {};
    static constexpr int64 e[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const double f0 = get(0, 0, 0); // invariant across i -- read once, not effDim times per voxel
    double magSqr = 0.0;
    for(uint32 i = 0; i < effDim; ++i)
    {
      const double fp = get(e[i][0], e[i][1], e[i][2]);
      const double fm = get(-e[i][0], -e[i][1], -e[i][2]);
      first[i] = 0.5 * (fp - fm) * sc[i];
      sec[i] = (fp - 2.0 * f0 + fm) * sc[i] * sc[i];
      for(uint32 j = i + 1; j < effDim; ++j)
      {
        const double a = get(-e[i][0] - e[j][0], -e[i][1] - e[j][1], -e[i][2] - e[j][2]);
        const double b = get(-e[i][0] + e[j][0], -e[i][1] + e[j][1], -e[i][2] + e[j][2]);
        const double c = get(e[i][0] - e[j][0], e[i][1] - e[j][1], e[i][2] - e[j][2]);
        const double d = get(e[i][0] + e[j][0], e[i][1] + e[j][1], e[i][2] + e[j][2]);
        cross[i][j] = 0.25 * (a - b - c + d) * sc[i] * sc[j];
      }
      magSqr += first[i] * first[i];
    }
    if(magSqr < 1e-9)
    {
      return 0.0;
    }
    double update = 0.0;
    for(uint32 i = 0; i < effDim; ++i)
    {
      double temp = 0.0;
      for(uint32 j = 0; j < effDim; ++j)
      {
        if(j != i)
        {
          temp += sec[j];
        }
      }
      update += temp * first[i] * first[i];
    }
    for(uint32 i = 0; i < effDim; ++i)
    {
      for(uint32 j = i + 1; j < effDim; ++j)
      {
        update -= 2.0 * first[i] * first[j] * cross[i][j];
      }
    }
    return update / magSqr;
  }
};

// itk::Math::Round<TReturn>(x) (== RoundHalfIntegerUp) for the double inputs MinMaxComputeThreshold needs. ITK's own
// two implementations (itkMathDetail.h: the SSE-ish RoundHalfIntegerToEven(2x+0.5)>>1 fast path AND the generic
// "_base" fallback `r = (TReturn)(x+0.5); IsNonnegative(x+0.5) ? r : (exact ? r : r-1)`) both realize the documented
// contract RoundHalfIntegerUp(1.5)==2, (-1.5)==-1, (2.5)==3 -- i.e. floor(x+0.5) -- regardless of which one a given
// ITK build actually took, so that closed form is what is ported here.
inline int64 MinMaxRoundHalfUp(double x)
{
  return static_cast<int64>(std::floor(x + 0.5));
}

// itkMinMaxCurvatureFlowFunction.hxx::ComputeThreshold(Dispatch<2>&, it) / (Dispatch<3>&, it), ported onto the
// driver's get(dxo,dyo,dzo) RELATIVE-offset accessor. ITK samples 2 (2D) / 4 (3D) neighborhood positions along the
// (rotated) gradient direction at radius `stencilRadius` and averages them; its `position[j]` is an ABSOLUTE local
// neighborhood coordinate (stencilRadius + a signed offset), but because stencilRadius is an integer and
// round(n + y) == n + round(y) for integer n, `position[j] - stencilRadius == round(the signed offset)` exactly --
// so every position below is expressed directly as the offset get() already expects, with no stride/index math.
// gradient[] and the trig locals are `double` throughout, matching PixelType==double for this filter (the
// IntermediateType=float64 bridge -- see the Parity model note in ExecuteFiniteDifferenceImageFilter's caller).
template <class GetFn>
double MinMaxComputeThreshold(const GetFn& get, const std::array<double, 3>& sc, uint32 effDim, int64 stencilRadius)
{
  const double r = static_cast<double>(stencilRadius);
  if(effDim == 2)
  {
    double gradient[2] = {0.0, 0.0};
    gradient[0] = 0.5 * (get(1, 0, 0) - get(-1, 0, 0)) * sc[0];
    gradient[1] = 0.5 * (get(0, 1, 0) - get(0, -1, 0)) * sc[1];
    double gradMagnitude = gradient[0] * gradient[0] + gradient[1] * gradient[1];
    if(gradMagnitude == 0.0)
    {
      return 0.0;
    }
    gradMagnitude = std::sqrt(gradMagnitude) / r;
    gradient[0] /= gradMagnitude;
    gradient[1] /= gradMagnitude;

    // Point 1 (angle 90): local offset (-gradient[1], +gradient[0]). Point 2 (angle 270): the negation.
    double threshold = get(MinMaxRoundHalfUp(-gradient[1]), MinMaxRoundHalfUp(gradient[0]), 0);
    threshold += get(MinMaxRoundHalfUp(gradient[1]), MinMaxRoundHalfUp(-gradient[0]), 0);
    threshold *= 0.5;
    return threshold;
  }

  // effDim == 3
  double gradient[3] = {0.0, 0.0, 0.0};
  gradient[0] = 0.5 * (get(1, 0, 0) - get(-1, 0, 0)) * sc[0];
  gradient[1] = 0.5 * (get(0, 1, 0) - get(0, -1, 0)) * sc[1];
  gradient[2] = 0.5 * (get(0, 0, 1) - get(0, 0, -1)) * sc[2];
  double gradMagnitude = gradient[0] * gradient[0] + gradient[1] * gradient[1] + gradient[2] * gradient[2];
  if(gradMagnitude == 0.0)
  {
    return 0.0;
  }
  gradMagnitude = std::sqrt(gradMagnitude) / r;
  gradient[0] /= gradMagnitude;
  gradient[1] /= gradMagnitude;
  gradient[2] /= gradMagnitude;

  double g2 = gradient[2];
  g2 = std::min(1.0, std::max(-1.0, g2));
  const double theta = std::acos(g2);
  // itk::Math::AlmostEquals(gradient[0], PixelType{}) with PixelType==double: FloatAlmostEqual's default
  // maxAbsoluteDifference is 0.1*epsilon<double>() (~2.2e-17) and gradient[0] is O(1), so only the
  // absolute-difference branch can ever fire -- the ULP branch is unreachable at this magnitude.
  constexpr double k_Pi = 3.14159265358979323846;
  const double phi = (std::abs(gradient[0]) <= 0.1 * std::numeric_limits<double>::epsilon()) ? (k_Pi * 0.5) : std::atan(gradient[1] / gradient[0]);

  const double cosTheta = std::cos(theta);
  const double sinTheta = std::sin(theta);
  const double cosPhi = std::cos(phi);
  const double sinPhi = std::sin(phi);

  const double rSinTheta = r * sinTheta;
  const double rCosThetaCosPhi = r * cosTheta * cosPhi;
  const double rCosThetaSinPhi = r * cosTheta * sinPhi;
  const double rSinPhi = r * sinPhi;
  const double rCosPhi = r * cosPhi;

  // Points at angle 0/90/180/270 around the gradient axis (ITK's Point 1..4); Points 2 & 4 have an EXACT (unrounded)
  // zero z-offset (position[2] = m_StencilRadius verbatim, not a rounded expression).
  double threshold = get(MinMaxRoundHalfUp(rCosThetaCosPhi), MinMaxRoundHalfUp(rCosThetaSinPhi), MinMaxRoundHalfUp(-rSinTheta));
  threshold += get(MinMaxRoundHalfUp(-rSinPhi), MinMaxRoundHalfUp(rCosPhi), 0);
  threshold += get(MinMaxRoundHalfUp(-rCosThetaCosPhi), MinMaxRoundHalfUp(-rCosThetaSinPhi), MinMaxRoundHalfUp(rSinTheta));
  threshold += get(MinMaxRoundHalfUp(rSinPhi), MinMaxRoundHalfUp(-rCosPhi), 0);
  threshold *= 0.25;
  return threshold;
}

// itkMinMaxCurvatureFlowFunction.hxx::InitializeStencilOperator builds this N-D sphere once per filter execution.
// Preserve ITK's dz-outer/dy-middle/dx-inner order so the later multiply-then-add average keeps identical rounding.
inline std::vector<std::array<int64, 3>> BuildMinMaxStencilOffsets(uint32 effDim, int64 stencilRadius)
{
  std::vector<std::array<int64, 3>> offsets;
  const int64 sqrRadius = stencilRadius * stencilRadius;
  for(int64 dz = (effDim == 3) ? -stencilRadius : 0; dz <= ((effDim == 3) ? stencilRadius : 0); ++dz)
  {
    for(int64 dy = -stencilRadius; dy <= stencilRadius; ++dy)
    {
      for(int64 dx = -stencilRadius; dx <= stencilRadius; ++dx)
      {
        if(dx * dx + dy * dy + dz * dz <= sqrRadius)
        {
          offsets.push_back({dx, dy, dz});
        }
      }
    }
  }
  return offsets;
}

inline constexpr int64 k_MaxPrecomputedMinMaxStencilRadius = 16;

template <class GetFn>
double MinMaxStencilAverageDynamic(const GetFn& get, uint32 effDim, int64 stencilRadius)
{
  const int64 squaredRadius = stencilRadius * stencilRadius;
  int64 pixelCount = 0;
  for(int64 dz = effDim == 3 ? -stencilRadius : 0; dz <= (effDim == 3 ? stencilRadius : 0); ++dz)
  {
    for(int64 dy = -stencilRadius; dy <= stencilRadius; ++dy)
    {
      for(int64 dx = -stencilRadius; dx <= stencilRadius; ++dx)
      {
        if(dx * dx + dy * dy + dz * dz <= squaredRadius)
        {
          ++pixelCount;
        }
      }
    }
  }
  if(pixelCount == 0)
  {
    return 0.0;
  }

  const double inversePixelCount = 1.0 / static_cast<double>(pixelCount);
  double sum = 0.0;
  for(int64 dz = effDim == 3 ? -stencilRadius : 0; dz <= (effDim == 3 ? stencilRadius : 0); ++dz)
  {
    for(int64 dy = -stencilRadius; dy <= stencilRadius; ++dy)
    {
      for(int64 dx = -stencilRadius; dx <= stencilRadius; ++dx)
      {
        if(dx * dx + dy * dy + dz * dz <= squaredRadius)
        {
          sum += get(dx, dy, dz) * inversePixelCount;
        }
      }
    }
  }
  return sum;
}

// NeighborhoodInnerProduct::Compute over the prebuilt sphere. Each per-term `pixel * invN` multiply-then-add
// mirrors ITK's `sum += op[i] * pixel[i]` exactly rather than summing raw pixels and dividing once.
template <class GetFn>
double MinMaxStencilAverage(const GetFn& get, const std::vector<std::array<int64, 3>>& offsets, double inversePixelCount)
{
  double sum = 0.0;
  for(const auto& offset : offsets)
  {
    sum += get(offset[0], offset[1], offset[2]) * inversePixelCount;
  }
  return sum;
}

// Min/max curvature-flow update functor (itkMinMaxCurvatureFlowFunction.hxx::ComputeUpdate): the base curvature-flow
// update (CurvatureFlowFn), gated by whether the local stencil average is above/below a gradient-direction threshold
// (see MinMaxComputeThreshold/MinMaxStencilAverage above). Unlike CurvatureFlowFn's fixed radius 1, this functor's
// radius is a RUNTIME member (the user's Stencil Radius parameter), which is why every functor exposes radius() as a
// method rather than a `static constexpr` -- the driver widens its rolling z-window to 2*radius()+1 planes for it.
struct MinMaxCurvatureFlowFn
{
  int64 stencilRadius = 2;

  explicit MinMaxCurvatureFlowFn(int64 radius = 2)
  : stencilRadius(radius)
  , m_StencilOffsets2D(radius >= 0 && radius <= k_MaxPrecomputedMinMaxStencilRadius ? BuildMinMaxStencilOffsets(2, radius) : std::vector<std::array<int64, 3>>{})
  , m_StencilOffsets3D(radius >= 0 && radius <= k_MaxPrecomputedMinMaxStencilRadius ? BuildMinMaxStencilOffsets(3, radius) : std::vector<std::array<int64, 3>>{})
  , m_InverseStencilPixelCount2D(m_StencilOffsets2D.empty() ? 0.0 : 1.0 / static_cast<double>(m_StencilOffsets2D.size()))
  , m_InverseStencilPixelCount3D(m_StencilOffsets3D.empty() ? 0.0 : 1.0 / static_cast<double>(m_StencilOffsets3D.size()))
  {
  }

  int64 radius() const
  {
    return stencilRadius;
  }

  const std::vector<std::array<int64, 3>>& stencilOffsets(uint32 effDim) const
  {
    return effDim == 3 ? m_StencilOffsets3D : m_StencilOffsets2D;
  }

  double inverseStencilPixelCount(uint32 effDim) const
  {
    return effDim == 3 ? m_InverseStencilPixelCount3D : m_InverseStencilPixelCount2D;
  }

  static constexpr bool k_NeedsGlobalGradient = false;
  // Same IntermediateType=float64 bridge as CurvatureFlowFn (MinMaxCurvatureFlowFunction inherits
  // CurvatureFlowFunction, and the legacy wrapper declares IntermediateType=float64) -- Real stays float64.
  static constexpr bool k_NativePrecision = false;

  template <class GetFn>
  double computeUpdate(const GetFn& get, const std::array<double, 3>& sc, uint32 effDim) const
  {
    // itkFiniteDifferenceFunction.hxx::ComputeNeighborhoodScales(): neighborhoodScales[i] = m_ScaleCoefficients[i] /
    // m_Radius[i]. CurvatureFlowFunction's OWN constructor sets its radius to 1 (so its neighborhoodScales ==
    // m_ScaleCoefficients == sc[] unchanged, matching plain CurvatureFlowFn). But MinMaxCurvatureFlowFunction
    // INHERITS CurvatureFlowFunction and calls SetStencilRadius -> SetRadius(stencilRadius) in ITS OWN constructor,
    // which overwrites `this->m_Radius` on the SAME object -- so when MinMaxCurvatureFlowFunction::ComputeUpdate
    // calls `Superclass::ComputeUpdate` (base CurvatureFlowFunction::ComputeUpdate), `this->ComputeNeighborhoodScales()`
    // divides by the OVERRIDDEN stencilRadius, not by 1. The base curvature derivatives must therefore be computed
    // with sc[i]/stencilRadius here, NOT raw sc[i] (verified: without this, the returned update was exactly
    // stencilRadius^2 too large, live-ITK parity mismatch root-caused via a per-voxel update dump at stencilRadius=2).
    // ComputeThreshold is UNAFFECTED -- it multiplies by `this->m_ScaleCoefficients[k]` directly (not through
    // ComputeNeighborhoodScales), so MinMaxComputeThreshold below keeps using raw sc[] unchanged. Division (not
    // multiplication by a precomputed reciprocal) matches ITK's `ScaleCoefficients[i] / m_Radius[i]` bit-for-bit for
    // every stencilRadius, not merely the powers of two exercised by the current parity tests (1, 2).
    const double radiusD = static_cast<double>(stencilRadius);
    const std::array<double, 3> baseSc = {sc[0] / radiusD, sc[1] / radiusD, sc[2] / radiusD};
    const double update = CurvatureFlowFn{}.computeUpdate(get, baseSc, effDim);
    if(update == 0.0)
    {
      return 0.0;
    }
    const double threshold = MinMaxComputeThreshold(get, sc, effDim, stencilRadius);
    const auto& offsets = stencilOffsets(effDim);
    const double avgValue = offsets.empty() ? MinMaxStencilAverageDynamic(get, effDim, stencilRadius) : MinMaxStencilAverage(get, offsets, inverseStencilPixelCount(effDim));
    return (avgValue < threshold) ? std::max(update, 0.0) : std::min(update, 0.0);
  }

private:
  std::vector<std::array<int64, 3>> m_StencilOffsets2D;
  std::vector<std::array<int64, 3>> m_StencilOffsets3D;
  double m_InverseStencilPixelCount2D = 0.0;
  double m_InverseStencilPixelCount3D = 0.0;
};

// Gradient (Perona-Malik) anisotropic-diffusion update functor (itkGradientNDAnisotropicDiffusionFunction.hxx::
// ComputeUpdate, radius 1 -- unlike MinMaxCurvatureFlowFn there is NO stencil-radius division here; sc[] is used
// raw). The conductance term C(x) = exp(-(|grad(I)|/K)^2) needs a GLOBAL reduction (the average squared gradient
// magnitude over the WHOLE image, recalibrating K every `interval` iterations) -- see the specialized
// detail::UpdateGlobalConductance below, which the driver invokes once per iteration (before CalculateChange)
// purely because k_NeedsGlobalGradient is true.
struct GradientAnisoFn
{
  // Modified global average-gradient-magnitude term (itkGradientNDAnisotropicDiffusionFunction.h: m_K), recomputed
  // every iteration by detail::UpdateGlobalConductance via setGlobalK() -- mutable per-iteration STATE, unlike
  // conductance/interval below which are constant parameters fixed by the caller for the whole run.
  double m_K = 0.0;
  double conductance = 1.0;
  uint32 interval = 1u;
  // itkAnisotropicDiffusionImageFilter's GradientMagnitudeIsFixed / FixedAverageGradientMagnitude escape hatch:
  // when set, the global reduction is skipped every iteration and setGlobalK(fixedAvgGradMagSq) is called directly
  // instead -- fixedAvgGradMagSq is stored ALREADY SQUARED (matching setGlobalK's avgGradMagSq parameter), unlike
  // ITK's own m_FixedAverageGradientMagnitude (a magnitude, squared internally before SetAverageGradientMagnitudeSquared).
  // The legacy simplnx wrapper never exposes this parameter (so both fields default off/0.0 below and stay
  // effectively dead code), but they are kept for ITK fidelity.
  bool gradientMagnitudeFixed = false;
  double fixedAvgGradMagSq = 0.0;

  int64 radius() const
  {
    return 1;
  }
  static constexpr bool k_NeedsGlobalGradient = true;
  // The legacy ITK bridge functor declares IntermediateType=float64 for this filter (verified), so Real stays
  // float64 regardless of T -- unlike CurvatureAnisoFn below, whose legacy bridge has NO IntermediateType.
  static constexpr bool k_NativePrecision = false;

  // itkGradientNDAnisotropicDiffusionFunction.h InitializeIteration:
  //   m_K = static_cast<PixelType>(avg * conductance * conductance * -2.0f)
  // KReal is the driver's Real -- float64 for this float64-bridged functor (k_NativePrecision==false), so the
  // static_cast<KReal> narrowing is a no-op and m_K keeps full double precision, matching ITK's own float64 PixelType
  // m_K here. The multiplication ORDER is ITK's (avg * c * c * -2.0), not the previous -2.0 * avg * c * c: the whole
  // product is formed in double (all operands promote; ITK's -2.0f promotes to -2.0 exactly) and only THEN narrowed,
  // exactly as ITK does -- matching ITK's factor order keeps this m_K byte-identical to ITK's, and matters for the
  // float32 rounding of the native-precision sibling (CurvatureAnisoFn) that shares this formula.
  template <class KReal>
  void setGlobalK(double avgGradMagSq)
  {
    m_K = static_cast<double>(static_cast<KReal>(avgGradMagSq * conductance * conductance * -2.0));
  }

  template <class GetFn>
  double computeUpdate(const GetFn& get, const std::array<double, 3>& sc, uint32 effDim) const
  {
    static constexpr int64 e[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const double f0 = get(0, 0, 0); // invariant across i -- read once, not effDim times per voxel
    // Centralized derivative per axis (computed once, reused below as the "dx[j]" term for j != i -- matching
    // ITK's own single pre-pass over ImageDimension before the per-i conductance loop).
    double facePlus[3] = {0.0, 0.0, 0.0};
    double faceMinus[3] = {0.0, 0.0, 0.0};
    double dx[3] = {0.0, 0.0, 0.0};
    for(uint32 i = 0; i < effDim; ++i)
    {
      facePlus[i] = get(e[i][0], e[i][1], e[i][2]);
      faceMinus[i] = get(-e[i][0], -e[i][1], -e[i][2]);
      dx[i] = 0.5 * (facePlus[i] - faceMinus[i]) * sc[i];
    }

    double dxAug[3][3] = {};
    double dxDim[3][3] = {};
    for(uint32 i = 0; i < effDim; ++i)
    {
      for(uint32 j = i + 1; j < effDim; ++j)
      {
        const double plusPlus = get(e[i][0] + e[j][0], e[i][1] + e[j][1], e[i][2] + e[j][2]);
        const double plusMinus = get(e[i][0] - e[j][0], e[i][1] - e[j][1], e[i][2] - e[j][2]);
        const double minusPlus = get(-e[i][0] + e[j][0], -e[i][1] + e[j][1], -e[i][2] + e[j][2]);
        const double minusMinus = get(-e[i][0] - e[j][0], -e[i][1] - e[j][1], -e[i][2] - e[j][2]);
        dxAug[i][j] = 0.5 * (plusPlus - plusMinus) * sc[j];
        dxDim[i][j] = 0.5 * (minusPlus - minusMinus) * sc[j];
        dxAug[j][i] = 0.5 * (plusPlus - minusPlus) * sc[i];
        dxDim[j][i] = 0.5 * (plusMinus - minusMinus) * sc[i];
      }
    }
    double delta = 0.0;
    for(uint32 i = 0; i < effDim; ++i)
    {
      double dxForward = (facePlus[i] - f0) * sc[i];
      double dxBackward = (f0 - faceMinus[i]) * sc[i];

      // Conductance terms: the gradient-magnitude approximation augmented/diminished along each OTHER axis j.
      double accum = 0.0;
      double accumD = 0.0;
      for(uint32 j = 0; j < effDim; ++j)
      {
        if(j == i)
        {
          continue;
        }
        accum += 0.25 * (dx[j] + dxAug[i][j]) * (dx[j] + dxAug[i][j]);
        accumD += 0.25 * (dx[j] + dxDim[i][j]) * (dx[j] + dxDim[i][j]);
      }

      double Cx = 0.0;
      double Cxd = 0.0;
      if(m_K != 0.0)
      {
        Cx = std::exp((dxForward * dxForward + accum) / m_K);
        Cxd = std::exp((dxBackward * dxBackward + accumD) / m_K);
      }
      delta += dxForward * Cx - dxBackward * Cxd;
    }
    return delta;
  }
};

// Curvature (modified-curvature diffusion equation / MCDE) anisotropic-diffusion update functor
// (itkCurvatureNDAnisotropicDiffusionFunction.hxx::ComputeUpdate, radius 1). Shares GradientAnisoFn's conductance
// state/m_K formula/global reduction machinery verbatim (detail::UpdateGlobalConductance below reads
// fn.interval/fn.setGlobalK generically -- it needs no changes for this functor), but the update itself is the
// "curvature" form: each axis's forward/backward difference is normalized by ITS OWN local gradient-magnitude
// approximation (grad_mag/grad_mag_d, regularized against an exact-zero gradient by m_MinNorm) before being
// conductance-weighted, and the summed per-axis "speed" term is further modulated by an upwind ("propagation")
// gradient-magnitude term -- unlike GradientAnisoFn, which conductance-weights the raw forward/backward
// differences directly with no normalization or upwind step.
struct CurvatureAnisoFn
{
  // Same conductance state + m_K formula as GradientAnisoFn -- see that struct's field comments (this is
  // deliberately a byte-for-byte duplicate of that layout, not a shared base, so each functor stays a simple
  // aggregate the caller can brace-initialize in member-declaration order).
  double m_K = 0.0;
  double conductance = 1.0;
  uint32 interval = 1u;
  bool gradientMagnitudeFixed = false;
  double fixedAvgGradMagSq = 0.0;

  int64 radius() const
  {
    return 1;
  }
  static constexpr bool k_NeedsGlobalGradient = true;
  // Unlike CurvatureFlowFn/MinMaxCurvatureFlowFn/GradientAnisoFn, ITKCurvatureAnisotropicDiffusionImageFilter's
  // legacy bridge functor declares NO IntermediateType (verified by grep) -- so live ITK runs it NATIVELY in the
  // input precision (float32 input stays float32 for the whole iteration, never upcast to float64).
  //
  // Three precision-model details had to match ITK before float32 was BIT-EXACT (verified 0-ULP vs live ITK at
  // every iteration count 1..40 on a 12x12x12 ramp at conductance=3.0/interval=1):
  //  1) ApplyFiniteDifference's Real must alias T (k_NativePrecision=true below), not the hardcoded float64 of the
  //     other three functors: float32 stays float32 for the whole iteration, matching ITK's native PixelType.
  //     With Real==float64 the whole iteration ran one precision level higher than ITK -- a deterministic
  //     precision-MODEL mismatch (~1 ULP at essentially every voxel), not thread-order FP noise.
  //  2) The driver's neighborhood accessor returns the RAW Real sample (not eagerly widened to double), and
  //     computeUpdate subtracts two neighbor reads in Real (float32) FIRST, widening only the finished difference --
  //     reproducing ITK's `dx_forward[i] = it.GetPixel(+) - it.GetPixel(center)` (a float32 subtraction) before the
  //     result is scaled in double. Eagerly promoting each operand before subtracting yields a strictly more precise
  //     (DIFFERENT) result whenever the true difference is not exactly float32-representable.
  //  3) m_K carries ITK's per-iteration float32 rounding: ITK's InitializeIteration recomputes
  //     `m_K = static_cast<PixelType>(avg * c * c * -2.0f)` EVERY iteration, so for a float32 image m_K is a
  //     float32. An earlier port kept m_K a full double, making the `grad_mag_sq / m_K` conductance divisor ~6e-8
  //     (relative) too precise. That single missing narrowing was the ENTIRE remaining float32 gap: with fixes (1)
  //     and (2) in place but m_K still double, the float32 diffCount/maxUlp GREW with iteration count -- 0/0, 18/1,
  //     14/1, 21/1, 53/2, 137/7, 271/163 at iterations 1,2,3,5,10,20,40 (measured directly against live ITK) --
  //     which had been MIS-attributed to reduction-order FP noise. It is not noise: setGlobalK below now narrows m_K
  //     to the native precision (KReal==T), and grad_mag_sq stays a double in computeUpdate exactly as in ITK (so
  //     the double-precision division is byte-identical to ITK's float32-m_K-promoted-to-double division), and
  //     float32 becomes bit-exact at ALL of those iteration counts (0/0 through 40 iterations).
  //  For a FLOAT64 image ITK's PixelType is already double, so m_K is double in both engines and (3) is a pure
  //  no-op; the float64 case retains a genuine reduction-summation-order divergence from ITK's own (possibly
  //  multi-threaded) CalculateAverageGradientMagnitudeSquared traversal (see detail::UpdateGlobalConductance), so
  //  the filter's live-ITK parity test asserts float32 BIT-EXACT (rt::RequireExact) but keeps float64 tolerant.
  static constexpr bool k_NativePrecision = true;

  // itkCurvatureNDAnisotropicDiffusionFunction.h InitializeIteration:
  //   m_K = static_cast<PixelType>(avg * conductance * conductance * -2.0f)
  // This functor runs NATIVELY (k_NativePrecision), so ITK's PixelType is the input type T. For float32 input ITK
  // ROUNDS m_K to float32 EVERY iteration; the divisor is then promoted back to double in ComputeUpdate's
  // `grad_mag_sq / m_K` (grad_mag_sq is itself a `double` in ITK, so the division runs in double -- ONLY the divisor
  // m_K carries the float32 rounding). KReal is the driver's Real (== T for this native functor), so
  // static_cast<KReal> reproduces that per-iteration narrowing exactly: float32 for float32 input, a no-op for
  // float64 input (whose PixelType is already double). m_K is stored as a `double` member holding the float32-
  // representable value, which makes the `gradMagSq / m_K` division in computeUpdate byte-identical to ITK's
  // float32-m_K-promoted-to-double division. Multiplication order matches ITK (avg * c * c * -2.0), which affects
  // that float32 rounding.
  template <class KReal>
  void setGlobalK(double avgGradMagSq)
  {
    m_K = static_cast<double>(static_cast<KReal>(avgGradMagSq * conductance * conductance * -2.0));
  }

  template <class GetFn>
  double computeUpdate(const GetFn& get, const std::array<double, 3>& sc, uint32 effDim) const
  {
    // itkCurvatureNDAnisotropicDiffusionFunction.hxx: static double m_MIN_NORM = 1.0e-10 -- regularizes
    // grad_mag/grad_mag_d against an exact-zero local gradient so the (dxForward[i]/gradMag) division below never
    // divides by zero.
    constexpr double k_MinNorm = 1.0e-10;
    static constexpr int64 e[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const auto f0 = get(0, 0, 0); // GetFn's own return type (== the engine's Real) -- invariant across i, read once

    // First pass (matches ITK's own single pre-pass over ImageDimension before the per-i conductance loop):
    // half (forward/backward) and centralized derivatives for every axis, all reused below. Each `fp`/`fm` pair is
    // SUBTRACTED before being widened to double (`static_cast<double>(fp - f0)`, not
    // `static_cast<double>(fp) - static_cast<double>(f0)`) -- this reproduces ITK's own PixelType-precision
    // subtraction (`dx_forward[i] = it.GetPixel(+) - it.GetPixel(center)`, then *only that finished difference* is
    // scaled in double) bit-for-bit. Live-ITK parity testing caught the eager-promote-then-subtract form as a real
    // (iteration-over-iteration GROWING ULP) divergence for float32 input, not bounded FP noise -- see
    // k_NativePrecision's comment above.
    using SampleType = std::decay_t<decltype(f0)>;
    SampleType facePlus[3] = {};
    SampleType faceMinus[3] = {};
    double dxForward[3] = {0.0, 0.0, 0.0};
    double dxBackward[3] = {0.0, 0.0, 0.0};
    double dx[3] = {0.0, 0.0, 0.0};
    for(uint32 i = 0; i < effDim; ++i)
    {
      facePlus[i] = get(e[i][0], e[i][1], e[i][2]);
      faceMinus[i] = get(-e[i][0], -e[i][1], -e[i][2]);
      dxForward[i] = static_cast<double>(facePlus[i] - f0) * sc[i];
      dxBackward[i] = static_cast<double>(f0 - faceMinus[i]) * sc[i];
      dx[i] = 0.5 * static_cast<double>(facePlus[i] - faceMinus[i]) * sc[i];
    }

    double dxAug[3][3] = {};
    double dxDim[3][3] = {};
    for(uint32 i = 0; i < effDim; ++i)
    {
      for(uint32 j = i + 1; j < effDim; ++j)
      {
        const SampleType plusPlus = get(e[i][0] + e[j][0], e[i][1] + e[j][1], e[i][2] + e[j][2]);
        const SampleType plusMinus = get(e[i][0] - e[j][0], e[i][1] - e[j][1], e[i][2] - e[j][2]);
        const SampleType minusPlus = get(-e[i][0] + e[j][0], -e[i][1] + e[j][1], -e[i][2] + e[j][2]);
        const SampleType minusMinus = get(-e[i][0] - e[j][0], -e[i][1] - e[j][1], -e[i][2] - e[j][2]);
        dxAug[i][j] = 0.5 * static_cast<double>(plusPlus - plusMinus) * sc[j];
        dxDim[i][j] = 0.5 * static_cast<double>(minusPlus - minusMinus) * sc[j];
        dxAug[j][i] = 0.5 * static_cast<double>(plusPlus - minusPlus) * sc[i];
        dxDim[j][i] = 0.5 * static_cast<double>(plusMinus - minusMinus) * sc[i];
      }
    }

    double speed = 0.0;
    for(uint32 i = 0; i < effDim; ++i)
    {
      // Gradient magnitude approximations (forward/backward), augmented along each OTHER axis j exactly like
      // GradientAnisoFn's accum/accumD, but here they feed a NORMALIZING gradient magnitude rather than the
      // conductance exponent's argument directly.
      double gradMagSq = dxForward[i] * dxForward[i];
      double gradMagSqD = dxBackward[i] * dxBackward[i];
      for(uint32 j = 0; j < effDim; ++j)
      {
        if(j == i)
        {
          continue;
        }
        gradMagSq += 0.25 * (dx[j] + dxAug[i][j]) * (dx[j] + dxAug[i][j]);
        gradMagSqD += 0.25 * (dx[j] + dxDim[i][j]) * (dx[j] + dxDim[i][j]);
      }
      const double gradMag = std::sqrt(k_MinNorm + gradMagSq);
      const double gradMagD = std::sqrt(k_MinNorm + gradMagSqD);

      double Cx = 0.0;
      double Cxd = 0.0;
      if(m_K != 0.0)
      {
        Cx = std::exp(gradMagSq / m_K);
        Cxd = std::exp(gradMagSqD / m_K);
      }
      // First-order normalized finite-difference conductance products, then the second-order
      // conductance-modified curvature accumulated into `speed`.
      const double dxForwardCn = (dxForward[i] / gradMag) * Cx;
      const double dxBackwardCn = (dxBackward[i] / gradMagD) * Cxd;
      speed += dxForwardCn - dxBackwardCn;
    }

    // "Upwind" gradient-magnitude term, gated by the sign of speed (itk::Math::sqr(min/max(dx_backward/forward,0))).
    double propagationGradient = 0.0;
    if(speed > 0.0)
    {
      for(uint32 i = 0; i < effDim; ++i)
      {
        const double a = std::min(dxBackward[i], 0.0);
        const double b = std::max(dxForward[i], 0.0);
        propagationGradient += a * a + b * b;
      }
    }
    else
    {
      for(uint32 i = 0; i < effDim; ++i)
      {
        const double a = std::max(dxBackward[i], 0.0);
        const double b = std::min(dxForward[i], 0.0);
        propagationGradient += a * a + b * b;
      }
    }
    return std::sqrt(propagationGradient) * speed;
  }
};

// Global-conductance hook (previously a no-op placeholder; Task 4 gives it a real body). The driver only calls
// this `if constexpr(F::k_NeedsGlobalGradient)`; CurvatureFlowFn/MinMaxCurvatureFlowFn::k_NeedsGlobalGradient are
// both false, so for those functors this template is simply never instantiated (the `if constexpr` discards the
// call entirely) and this body's use of `fn.interval`/`fn.setGlobalK` is never checked against them.
//
// Body (itkAnisotropicDiffusionImageFilter.hxx::InitializeIteration + itkScalarAnisotropicDiffusionFunction.hxx::
// CalculateAverageGradientMagnitudeSquared): every `fn.interval` iterations (elapsed % interval == 0, which is
// always true at iter==0 for any nonzero interval -- ITK's own cadence) recompute avgGradMagSq as the mean, over
// EVERY voxel, of sum_i (0.5*(accum[+i]-accum[-i])*sc[i])^2 (ZeroFluxNeumann boundary), then call
// fn.setGlobalK(avgGradMagSq). The accumulation is done as ONE deterministic single-threaded ordered pass (raster
// order, streamed via a 3-plane rolling window exactly like ApplyFiniteDifference's own CalculateChange pass
// below) so in-core and out-of-core runs are byte-identical -- this is intentionally NOT a port of ITK's own
// multi-threaded face-list traversal order (whose partial-sum order varies with thread count even within ITK
// itself; see the live-ITK parity note on the aniso test grid for why that is an acceptable, provably-FP-noise
// divergence rather than a structural one). `fn.interval == 0` is guarded (never divide/modulo by zero) by treating
// it as "always update" -- ITK's own unsigned-integer modulo would be undefined behavior for that input.
//
// Templated on `Real` (== ApplyFiniteDifference's accum/update element type: float64 for every functor except
// CurvatureAnisoFn's native-precision case, where Real==T) rather than hardcoded float64: ITK's own
// CalculateAverageGradientMagnitudeSquared reads two PixelType neighbors and SUBTRACTS them in PixelType
// arithmetic (`val = GetPixel(+) - GetPixel(-)`) BEFORE widening to its (always-double) PixelRealType accumulator
// -- for the native-precision (float32) case that subtraction step must likewise happen in Real (float32), not
// after an eager promotion to double, to match ITK bit-for-bit wherever the neighbor difference itself is not
// exactly double-representable from two floats. Each `diff` local below is exactly that Real-precision subtraction;
// the `static_cast<double>` widening happens only afterward, on the finished difference -- never on the operands.
template <class F, class Real, class StoreT>
Result<> UpdateGlobalConductance(F& fn, const StoreT& accum, const SizeVec3& dims, const std::array<double, 3>& sc, uint32 effDim, uint32 iter, const std::atomic_bool& shouldCancel)
{
  if(fn.gradientMagnitudeFixed)
  {
    fn.template setGlobalK<Real>(fn.fixedAvgGradMagSq);
    return {};
  }
  if(fn.interval != 0 && (iter % fn.interval) != 0)
  {
    return {};
  }

  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  const usize slice = static_cast<usize>(nX) * static_cast<usize>(nY);
  auto clampi = [](int64 v, int64 hi) { return v < 0 ? int64{0} : (v > hi ? hi : v); };

  // 3-plane rolling window (radius 1, fixed -- CalculateAverageGradientMagnitudeSquared's derivative operator is
  // ALWAYS radius 1, independent of the calling functor's own radius()).
  std::array<std::vector<Real>, 3> win = {std::vector<Real>(slice), std::vector<Real>(slice), std::vector<Real>(slice)};
  std::array<int64, 3> windowZ = {std::numeric_limits<int64>::min(), std::numeric_limits<int64>::min(), std::numeric_limits<int64>::min()};
  auto loadPlane = [&](usize slot, int64 zWanted) -> Result<> {
    const int64 zc = clampi(zWanted, nZ - 1);
    for(usize sourceSlot = 0; sourceSlot < windowZ.size(); ++sourceSlot)
    {
      if(sourceSlot != slot && windowZ[sourceSlot] == zc)
      {
        win[slot] = win[sourceSlot];
        windowZ[slot] = zc;
        return {};
      }
    }
    if(Result<> result = accum.copyIntoBuffer(static_cast<usize>(zc) * slice, nonstd::span<Real>(win[slot].data(), slice)); result.invalid())
    {
      return result;
    }
    windowZ[slot] = zc;
    return {};
  };
  for(int64 k = 0; k < 3; ++k)
  {
    if(Result<> rr = loadPlane(static_cast<usize>(k), k - 1); rr.invalid())
    {
      return rr;
    }
  }

  double sum = 0.0;
  usize counter = 0;
  for(int64 z = 0; z < nZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const std::vector<Real>& planeMinus = win[0];
    const std::vector<Real>& planeCenter = win[1];
    const std::vector<Real>& planePlus = win[2];
    for(int64 y = 0; y < nY; ++y)
    {
      const int64 yp = clampi(y + 1, nY - 1);
      const int64 ym = clampi(y - 1, nY - 1);
      for(int64 x = 0; x < nX; ++x)
      {
        const int64 xp = clampi(x + 1, nX - 1);
        const int64 xm = clampi(x - 1, nX - 1);
        const usize idx = static_cast<usize>(y * nX + x);
        double voxelSum = 0.0;
        {
          const Real diff = planeCenter[static_cast<usize>(y * nX + xp)] - planeCenter[static_cast<usize>(y * nX + xm)];
          const double val = 0.5 * static_cast<double>(diff) * sc[0];
          voxelSum += val * val;
        }
        {
          const Real diff = planeCenter[static_cast<usize>(yp * nX + x)] - planeCenter[static_cast<usize>(ym * nX + x)];
          const double val = 0.5 * static_cast<double>(diff) * sc[1];
          voxelSum += val * val;
        }
        if(effDim == 3)
        {
          const Real diff = planePlus[idx] - planeMinus[idx];
          const double val = 0.5 * static_cast<double>(diff) * sc[2];
          voxelSum += val * val;
        }
        sum += voxelSum;
        ++counter;
      }
    }
    if(z + 1 < nZ)
    {
      std::swap(win[0], win[1]);
      std::swap(win[1], win[2]);
      std::swap(windowZ[0], windowZ[1]);
      std::swap(windowZ[1], windowZ[2]);
      if(Result<> rr = loadPlane(2, z + 2); rr.invalid())
      {
        return rr;
      }
    }
  }

  const double avgGradMagSq = (counter > 0) ? (sum / static_cast<double>(counter)) : 0.0;
  fn.template setGlobalK<Real>(avgGradMagSq);
  return {};
}

template <class T, class F, class Real, class InputStoreT, class WorkingStoreT>
Result<> InitializeFiniteDifferenceWorkingStoreAndConductance(F& fn, const InputStoreT& inStore, WorkingStoreT& firstStore, const SizeVec3& dims, const std::array<double, 3>& sc,
                                                              const std::atomic_bool& shouldCancel)
{
  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  const usize slice = static_cast<usize>(nX) * static_cast<usize>(nY);
  auto clampi = [](int64 value, int64 high) { return value < 0 ? int64{0} : (value > high ? high : value); };

  std::array<std::vector<Real>, 3> win = {std::vector<Real>(slice), std::vector<Real>(slice), std::vector<Real>(slice)};
  std::array<int64, 3> windowZ = {std::numeric_limits<int64>::min(), std::numeric_limits<int64>::min(), std::numeric_limits<int64>::min()};
  std::vector<T> inputPlane(slice);
  auto loadPlane = [&](usize slot, int64 zWanted) -> Result<> {
    const int64 zc = clampi(zWanted, nZ - 1);
    for(usize sourceSlot = 0; sourceSlot < windowZ.size(); ++sourceSlot)
    {
      if(sourceSlot != slot && windowZ[sourceSlot] == zc)
      {
        win[slot] = win[sourceSlot];
        windowZ[slot] = zc;
        return {};
      }
    }
    if(Result<> result = inStore.copyIntoBuffer(static_cast<usize>(zc) * slice, nonstd::span<T>(inputPlane.data(), slice)); result.invalid())
    {
      return result;
    }
    for(usize index = 0; index < slice; ++index)
    {
      win[slot][index] = static_cast<Real>(inputPlane[index]);
    }
    if(Result<> result = firstStore.copyFromBuffer(static_cast<usize>(zc) * slice, nonstd::span<const Real>(win[slot].data(), slice)); result.invalid())
    {
      return result;
    }
    windowZ[slot] = zc;
    return {};
  };
  for(int64 slot = 0; slot < 3; ++slot)
  {
    if(Result<> result = loadPlane(static_cast<usize>(slot), slot - 1); result.invalid())
    {
      return result;
    }
  }

  double sum = 0.0;
  usize counter = 0;
  for(int64 z = 0; z < nZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const std::vector<Real>& planeMinus = win[0];
    const std::vector<Real>& planeCenter = win[1];
    const std::vector<Real>& planePlus = win[2];
    for(int64 y = 0; y < nY; ++y)
    {
      const int64 yp = clampi(y + 1, nY - 1);
      const int64 ym = clampi(y - 1, nY - 1);
      for(int64 x = 0; x < nX; ++x)
      {
        const int64 xp = clampi(x + 1, nX - 1);
        const int64 xm = clampi(x - 1, nX - 1);
        const usize idx = static_cast<usize>(y * nX + x);
        double voxelSum = 0.0;
        const Real xDiff = planeCenter[static_cast<usize>(y * nX + xp)] - planeCenter[static_cast<usize>(y * nX + xm)];
        const double xValue = 0.5 * static_cast<double>(xDiff) * sc[0];
        voxelSum += xValue * xValue;
        const Real yDiff = planeCenter[static_cast<usize>(yp * nX + x)] - planeCenter[static_cast<usize>(ym * nX + x)];
        const double yValue = 0.5 * static_cast<double>(yDiff) * sc[1];
        voxelSum += yValue * yValue;
        const Real zDiff = planePlus[idx] - planeMinus[idx];
        const double zValue = 0.5 * static_cast<double>(zDiff) * sc[2];
        voxelSum += zValue * zValue;
        sum += voxelSum;
        ++counter;
      }
    }
    if(z + 1 < nZ)
    {
      std::swap(win[0], win[1]);
      std::swap(win[1], win[2]);
      std::swap(windowZ[0], windowZ[1]);
      std::swap(windowZ[1], windowZ[2]);
      if(Result<> result = loadPlane(2, z + 2); result.invalid())
      {
        return result;
      }
    }
  }
  const double avgGradMagSq = counter > 0 ? sum / static_cast<double>(counter) : 0.0;
  fn.template setGlobalK<Real>(avgGradMagSq);
  return {};
}
} // namespace detail

// Decile progress over the outer iteration loop, mirroring RecursiveGaussianEngine's throttled messageHandler
// pattern: emit only when a new ~10% boundary is crossed (never per-iteration, which would spam thousands of
// messages on a long run). Progress is message emission ONLY -- it never touches store contents, so numerical
// output is unaffected. Stateless (derived purely from iter/numberOfIterations) so repeated engine invocations in
// the same process never observe stale throttle state from a previous call.
inline void throttleProgress(const IFilter::MessageHandler& messageHandler, uint32 iter, uint32 numberOfIterations)
{
  if(numberOfIterations == 0)
  {
    return;
  }
  const uint32 tenth = (iter * 10) / numberOfIterations;
  if(iter == 0 || tenth != ((iter - 1) * 10) / numberOfIterations)
  {
    messageHandler(fmt::format("Finite Difference: iteration {}/{} ({}%)", iter + 1, numberOfIterations, tenth * 10));
  }
}

template <class ResultT>
std::string DescribeFiniteDifferenceResultError(const Result<ResultT>& result)
{
  if(result.errors().empty())
  {
    return "provider returned an unspecified error";
  }
  const Error& error = result.errors().front();
  return fmt::format("{} (provider code {})", error.message, error.code);
}

inline Result<std::unique_ptr<ITemporaryRecordStore>> CreateFiniteDifferenceScratchStore(const TemporaryRecordStoreConfig& config)
{
  try
  {
    auto result = DataStoreUtilities::CreateTemporaryRecordStore(config);
    if(result.invalid())
    {
      return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(
          -8637, fmt::format("Finite-difference failed to create a fixed-record OOC working store: {}", DescribeFiniteDifferenceResultError(result)));
    }
    std::unique_ptr<ITemporaryRecordStore> store = std::move(result.value());
    if(store == nullptr)
    {
      return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-8637, "Finite-difference failed to create a fixed-record OOC working store: provider returned a null store");
    }
    return {std::move(store)};
  } catch(const std::bad_alloc& exception)
  {
    return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-8637, fmt::format("Finite-difference failed to create a fixed-record OOC working store: {}", exception.what()));
  } catch(const std::exception& exception)
  {
    return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-8637, fmt::format("Finite-difference failed to create a fixed-record OOC working store: {}", exception.what()));
  }
}

template <class Real>
class TemporaryFiniteDifferenceStore
{
public:
  TemporaryFiniteDifferenceStore(ITemporaryRecordStore& store, const std::atomic_bool& shouldCancel)
  : m_Store(store)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize valueOffset, nonstd::span<Real> values) const
  {
    try
    {
      nonstd::span<std::byte> bytes(reinterpret_cast<std::byte*>(values.data()), values.size() * sizeof(Real));
      auto result = m_Store.read(static_cast<uint64>(valueOffset), static_cast<uint64>(values.size()), bytes, m_ShouldCancel);
      if(result.invalid())
      {
        return MakeErrorResult(-8638, fmt::format("Finite-difference fixed-record OOC read failed: {}", DescribeFiniteDifferenceResultError(result)));
      }
      if(result.value() != values.size())
      {
        return MakeErrorResult(-8638, fmt::format("Finite-difference fixed-record OOC read returned {} of {} values.", result.value(), values.size()));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8638, fmt::format("Finite-difference fixed-record OOC read failed: {}", exception.what()));
    }
    return {};
  }

  Result<> copyFromBuffer(usize valueOffset, nonstd::span<const Real> values)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    try
    {
      nonstd::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(values.data()), values.size() * sizeof(Real));
      Result<> result = m_Store.write(static_cast<uint64>(valueOffset), static_cast<uint64>(values.size()), bytes, m_ShouldCancel);
      if(result.invalid())
      {
        return MakeErrorResult(-8638, fmt::format("Finite-difference fixed-record OOC write failed: {}", DescribeFiniteDifferenceResultError(result)));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8638, fmt::format("Finite-difference fixed-record OOC write failed: {}", exception.what()));
    }
    return {};
  }

private:
  ITemporaryRecordStore& m_Store;
  const std::atomic_bool& m_ShouldCancel;
};

template <class StoreT, class ValueT>
Result<> ReadFiniteDifference2DRegion(const StoreT& store, usize nx, usize xBegin, usize xEnd, usize yBegin, usize yEnd, nonstd::span<ValueT> buffer)
{
  const usize width = xEnd - xBegin;
  const usize rows = yEnd - yBegin;
  const usize valueCount = width * rows;
  if(width == nx)
  {
    return store.copyIntoBuffer(yBegin * nx, buffer.subspan(0, valueCount));
  }
  for(usize row = 0; row < rows; ++row)
  {
    if(Result<> result = store.copyIntoBuffer((yBegin + row) * nx + xBegin, buffer.subspan(row * width, width)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class StoreT, class ValueT>
Result<> WriteFiniteDifference2DRegion(StoreT& store, usize nx, usize xBegin, usize xEnd, usize yBegin, usize yEnd, nonstd::span<const ValueT> buffer)
{
  const usize width = xEnd - xBegin;
  const usize rows = yEnd - yBegin;
  const usize valueCount = width * rows;
  if(width == nx)
  {
    return store.copyFromBuffer(yBegin * nx, buffer.subspan(0, valueCount));
  }
  for(usize row = 0; row < rows; ++row)
  {
    if(Result<> result = store.copyFromBuffer((yBegin + row) * nx + xBegin, buffer.subspan(row * width, width)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class F, class Real, class StoreT>
Result<> UpdateGlobalConductance2DBounded(F& fn, const StoreT& currentStore, usize nx, usize ny, const std::array<double, 3>& sc, const detail::FiniteDifference2DBufferPlan& plan,
                                          nonstd::span<Real> inputBuffer, uint32 iter, const std::atomic_bool& shouldCancel)
{
  if(fn.gradientMagnitudeFixed)
  {
    fn.template setGlobalK<Real>(fn.fixedAvgGradMagSq);
    return {};
  }
  if(fn.interval != 0 && (iter % fn.interval) != 0)
  {
    return {};
  }

  double sum = 0.0;
  usize counter = 0;
  const auto accumulateTile = [&](usize xBegin, usize xEnd, usize y, usize loadXBegin, usize loadXEnd, usize loadYBegin) {
    const usize loadedWidth = loadXEnd - loadXBegin;
    auto get = [&](usize x, usize sampleY) -> Real { return inputBuffer[(sampleY - loadYBegin) * loadedWidth + (x - loadXBegin)]; };
    const usize yMinus = y == 0 ? 0 : y - 1;
    const usize yPlus = std::min(y + 1, ny - 1);
    for(usize x = xBegin; x < xEnd; ++x)
    {
      const usize xMinus = x == 0 ? 0 : x - 1;
      const usize xPlus = std::min(x + 1, nx - 1);
      const Real diffX = get(xPlus, y) - get(xMinus, y);
      const Real diffY = get(x, yPlus) - get(x, yMinus);
      const double valueX = 0.5 * static_cast<double>(diffX) * sc[0];
      const double valueY = 0.5 * static_cast<double>(diffY) * sc[1];
      sum += valueX * valueX + valueY * valueY;
      ++counter;
    }
  };

  if(plan.coreCols == nx)
  {
    for(usize yBegin = 0; yBegin < ny; yBegin += plan.coreRows)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize yEnd = yBegin + std::min(plan.coreRows, ny - yBegin);
      const usize loadYBegin = yBegin == 0 ? 0 : yBegin - 1;
      const usize loadYEnd = yEnd + std::min(usize{1}, ny - yEnd);
      if(Result<> result = ReadFiniteDifference2DRegion(currentStore, nx, 0, nx, loadYBegin, loadYEnd, inputBuffer); result.invalid())
      {
        return result;
      }
      for(usize y = yBegin; y < yEnd; ++y)
      {
        accumulateTile(0, nx, y, 0, nx, loadYBegin);
      }
    }
  }
  else
  {
    for(usize y = 0; y < ny; ++y)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize loadYBegin = y == 0 ? 0 : y - 1;
      const usize loadYEnd = std::min(y + 2, ny);
      for(usize xBegin = 0; xBegin < nx; xBegin += plan.coreCols)
      {
        const usize xEnd = xBegin + std::min(plan.coreCols, nx - xBegin);
        const usize loadXBegin = xBegin == 0 ? 0 : xBegin - 1;
        const usize loadXEnd = xEnd + std::min(usize{1}, nx - xEnd);
        if(Result<> result = ReadFiniteDifference2DRegion(currentStore, nx, loadXBegin, loadXEnd, loadYBegin, loadYEnd, inputBuffer); result.invalid())
        {
          return result;
        }
        accumulateTile(xBegin, xEnd, y, loadXBegin, loadXEnd, loadYBegin);
      }
    }
  }
  const double average = counter > 0 ? sum / static_cast<double>(counter) : 0.0;
  fn.template setGlobalK<Real>(average);
  return {};
}

template <class T, class F, class Real, class WorkingStoreT>
Result<> ApplyFiniteDifference2DBounded(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, WorkingStoreT& firstStore, WorkingStoreT& secondStore, usize nx, usize ny,
                                        const std::array<double, 3>& sc, F fn, float64 timeStep, uint32 numberOfIterations, const std::atomic_bool& shouldCancel,
                                        const IFilter::MessageHandler& messageHandler, usize residentLimit)
{
  const usize radius = static_cast<usize>(fn.radius());
  const usize planRadius = std::max(radius, static_cast<usize>(F::k_NeedsGlobalGradient));
  const detail::FiniteDifference2DBufferPlan plan = detail::BuildFiniteDifference2DBufferPlan(nx, ny, planRadius, sizeof(Real), residentLimit);
  if(plan.overflow)
  {
    return MakeErrorResult(-8635, fmt::format("Finite-difference 2D buffer plan cannot represent dimensions {} x {}, radius {}, and {}-byte working values due to arithmetic overflow.", nx, ny,
                                              planRadius, sizeof(Real)));
  }
  if(!plan.valid)
  {
    return MakeErrorResult(-8636, fmt::format("Finite-difference 2D buffer plan exceeds the {}-byte resident limit for dimensions {} x {}, radius {}, and {}-byte working values.", residentLimit, nx,
                                              ny, planRadius, sizeof(Real)));
  }

  usize inputRows = 0;
  usize inputCols = 0;
  usize inputCapacity = 0;
  usize outputCapacity = 0;
  if(plan.coreCols == nx)
  {
    inputRows = plan.coreRows + 2 * planRadius;
    inputCols = nx;
  }
  else
  {
    inputRows = 1 + 2 * planRadius;
    inputCols = plan.coreCols + 2 * planRadius;
  }
  if(!detail::FiniteDifferenceCheckedMultiply(inputRows, inputCols, inputCapacity) || !detail::FiniteDifferenceCheckedMultiply(plan.coreRows, plan.coreCols, outputCapacity))
  {
    return MakeErrorResult(-8635, fmt::format("Finite-difference 2D working-buffer layout overflows for dimensions {} x {} and radius {}.", nx, ny, planRadius));
  }

  {
    std::vector<T> inputBlock(outputCapacity);
    std::vector<Real> castBlock(outputCapacity);
    for(usize yBegin = 0; yBegin < ny; yBegin += plan.coreRows)
    {
      const usize yEnd = yBegin + std::min(plan.coreRows, ny - yBegin);
      for(usize xBegin = 0; xBegin < nx; xBegin += plan.coreCols)
      {
        if(shouldCancel)
        {
          return {};
        }
        const usize xEnd = xBegin + std::min(plan.coreCols, nx - xBegin);
        const usize valueCount = (yEnd - yBegin) * (xEnd - xBegin);
        if(Result<> result = ReadFiniteDifference2DRegion(inStore, nx, xBegin, xEnd, yBegin, yEnd, nonstd::span<T>(inputBlock.data(), inputBlock.size())); result.invalid())
        {
          return result;
        }
        for(usize index = 0; index < valueCount; ++index)
        {
          castBlock[index] = static_cast<Real>(inputBlock[index]);
        }
        if(Result<> result = WriteFiniteDifference2DRegion(firstStore, nx, xBegin, xEnd, yBegin, yEnd, nonstd::span<const Real>(castBlock.data(), valueCount)); result.invalid())
        {
          return result;
        }
      }
    }
  }

  WorkingStoreT* currentStore = &firstStore;
  WorkingStoreT* nextStore = &secondStore;
  {
    std::vector<Real> inputBlock(inputCapacity);
    std::vector<Real> outputBlock(outputCapacity);
    for(uint32 iter = 0; iter < numberOfIterations; ++iter)
    {
      if(shouldCancel)
      {
        return {};
      }
      if constexpr(F::k_NeedsGlobalGradient)
      {
        if(Result<> result = UpdateGlobalConductance2DBounded(fn, *currentStore, nx, ny, sc, plan, nonstd::span<Real>(inputBlock.data(), inputBlock.size()), iter, shouldCancel); result.invalid())
        {
          return result;
        }
      }

      for(usize yBegin = 0; yBegin < ny; yBegin += plan.coreRows)
      {
        const usize yEnd = yBegin + std::min(plan.coreRows, ny - yBegin);
        const usize loadYBegin = yBegin > radius ? yBegin - radius : 0;
        const usize loadYEnd = yEnd + std::min(radius, ny - yEnd);
        for(usize xBegin = 0; xBegin < nx; xBegin += plan.coreCols)
        {
          if(shouldCancel)
          {
            return {};
          }
          const usize xEnd = xBegin + std::min(plan.coreCols, nx - xBegin);
          const usize loadXBegin = xBegin > radius ? xBegin - radius : 0;
          const usize loadXEnd = xEnd + std::min(radius, nx - xEnd);
          const usize loadedWidth = loadXEnd - loadXBegin;
          const usize coreWidth = xEnd - xBegin;
          const usize coreRows = yEnd - yBegin;
          if(Result<> result = ReadFiniteDifference2DRegion(*currentStore, nx, loadXBegin, loadXEnd, loadYBegin, loadYEnd, nonstd::span<Real>(inputBlock.data(), inputBlock.size())); result.invalid())
          {
            return result;
          }

          auto calculateRows = [&](const Range& rowRange) {
            for(usize localY = rowRange.min(); localY < rowRange.max(); ++localY)
            {
              const usize y = yBegin + localY;
              for(usize localX = 0; localX < coreWidth; ++localX)
              {
                const usize x = xBegin + localX;
                auto get = [&](int64 dx, int64 dy, int64) -> Real {
                  const int64 sampleX = std::clamp(static_cast<int64>(x) + dx, int64{0}, static_cast<int64>(nx - 1));
                  const int64 sampleY = std::clamp(static_cast<int64>(y) + dy, int64{0}, static_cast<int64>(ny - 1));
                  return inputBlock[(static_cast<usize>(sampleY) - loadYBegin) * loadedWidth + (static_cast<usize>(sampleX) - loadXBegin)];
                };
                const Real update = static_cast<Real>(fn.computeUpdate(get, sc, 2));
                const Real delta = static_cast<Real>(static_cast<double>(update) * timeStep);
                outputBlock[localY * coreWidth + localX] = static_cast<Real>(get(0, 0, 0) + delta);
              }
            }
          };
          if(plan.coreCols == nx && coreRows > 1)
          {
            ParallelDataAlgorithm algorithm;
            algorithm.setRange(0, coreRows);
            algorithm.execute(calculateRows);
          }
          else
          {
            calculateRows(Range{0, coreRows});
          }
          const usize valueCount = coreRows * coreWidth;
          if(Result<> result = WriteFiniteDifference2DRegion(*nextStore, nx, xBegin, xEnd, yBegin, yEnd, nonstd::span<const Real>(outputBlock.data(), valueCount)); result.invalid())
          {
            return result;
          }
        }
      }
      std::swap(currentStore, nextStore);
      throttleProgress(messageHandler, iter, numberOfIterations);
    }
  }

  {
    std::vector<Real> workingBlock(outputCapacity);
    std::vector<T> outputValues(outputCapacity);
    for(usize yBegin = 0; yBegin < ny; yBegin += plan.coreRows)
    {
      const usize yEnd = yBegin + std::min(plan.coreRows, ny - yBegin);
      for(usize xBegin = 0; xBegin < nx; xBegin += plan.coreCols)
      {
        if(shouldCancel)
        {
          return {};
        }
        const usize xEnd = xBegin + std::min(plan.coreCols, nx - xBegin);
        const usize valueCount = (yEnd - yBegin) * (xEnd - xBegin);
        if(Result<> result = ReadFiniteDifference2DRegion(*currentStore, nx, xBegin, xEnd, yBegin, yEnd, nonstd::span<Real>(workingBlock.data(), workingBlock.size())); result.invalid())
        {
          return result;
        }
        for(usize index = 0; index < valueCount; ++index)
        {
          outputValues[index] = static_cast<T>(workingBlock[index]);
        }
        if(Result<> result = WriteFiniteDifference2DRegion(outStore, nx, xBegin, xEnd, yBegin, yEnd, nonstd::span<const T>(outputValues.data(), valueCount)); result.invalid())
        {
          return result;
        }
      }
    }
  }
  return {};
}

template <class T, class F, class Real, class WorkingStoreT>
Result<> ApplyFiniteDifference3DPingPong(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, WorkingStoreT& firstStore, WorkingStoreT& secondStore, const SizeVec3& dims,
                                         const std::array<double, 3>& sc, F fn, float64 timeStep, uint32 numberOfIterations, const std::atomic_bool& shouldCancel,
                                         const IFilter::MessageHandler& messageHandler)
{
  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  const usize slice = dims[0] * dims[1];
  const int64 radius = fn.radius();
  auto clampi = [](int64 value, int64 high) { return value < 0 ? int64{0} : (value > high ? high : value); };

  if(numberOfIterations == 0)
  {
    std::vector<T> inputPlane(slice);
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = inStore.copyIntoBuffer(static_cast<usize>(z) * slice, nonstd::span<T>(inputPlane.data(), slice)); result.invalid())
      {
        return result;
      }
      if(Result<> result = outStore.copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const T>(inputPlane.data(), slice)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  if constexpr(F::k_NeedsGlobalGradient)
  {
    if(!fn.gradientMagnitudeFixed)
    {
      if(Result<> result = detail::InitializeFiniteDifferenceWorkingStoreAndConductance<T, F, Real>(fn, inStore, firstStore, dims, sc, shouldCancel); result.invalid())
      {
        return result;
      }
    }
    else
    {
      std::vector<T> inputPlane(slice);
      std::vector<Real> workingPlane(slice);
      for(int64 z = 0; z < nZ; ++z)
      {
        if(shouldCancel)
        {
          return {};
        }
        if(Result<> result = inStore.copyIntoBuffer(static_cast<usize>(z) * slice, nonstd::span<T>(inputPlane.data(), slice)); result.invalid())
        {
          return result;
        }
        for(usize index = 0; index < slice; ++index)
        {
          workingPlane[index] = static_cast<Real>(inputPlane[index]);
        }
        if(Result<> result = firstStore.copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const Real>(workingPlane.data(), slice)); result.invalid())
        {
          return result;
        }
      }
    }
  }

  const usize windowPlanes = static_cast<usize>(2 * radius + 1);
  std::vector<std::vector<Real>> window(windowPlanes, std::vector<Real>(slice));
  std::vector<Real> nextPlane(slice);
  std::vector<T> finalOutputPlane(slice);
  std::vector<T> inputCastPlane(slice);
  WorkingStoreT* currentStore = &firstStore;
  WorkingStoreT* nextStore = &secondStore;

  auto processIteration = [&](auto&& loadPlane, WorkingStoreT* destinationStore, bool finalIteration) -> Result<> {
    std::vector<int64> windowZ(windowPlanes, std::numeric_limits<int64>::min());
    auto loadWindowPlane = [&](usize slot, int64 zWanted) -> Result<> {
      const int64 z = clampi(zWanted, nZ - 1);
      for(usize sourceSlot = 0; sourceSlot < windowZ.size(); ++sourceSlot)
      {
        if(sourceSlot != slot && windowZ[sourceSlot] == z)
        {
          window[slot] = window[sourceSlot];
          windowZ[slot] = z;
          return {};
        }
      }
      if(Result<> result = loadPlane(window[slot], z); result.invalid())
      {
        return result;
      }
      windowZ[slot] = z;
      return {};
    };
    for(int64 slot = 0; slot < static_cast<int64>(windowPlanes); ++slot)
    {
      if(Result<> result = loadWindowPlane(static_cast<usize>(slot), slot - radius); result.invalid())
      {
        return result;
      }
    }
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      auto calculateRows = [&](const Range& rowRange) {
        const auto interiorX = detail::CreateFiniteDifferenceInteriorRange(static_cast<usize>(nX), radius);
        for(usize row = rowRange.min(); row < rowRange.max(); ++row)
        {
          const int64 y = static_cast<int64>(row);
          const bool rowIsInterior = static_cast<usize>(radius) <= row && static_cast<usize>(radius) < static_cast<usize>(nY - y);
          auto calculateValue = [&](int64 x, bool useDirectNeighbors) {
            const usize index = static_cast<usize>(y) * static_cast<usize>(nX) + static_cast<usize>(x);
            if(useDirectNeighbors)
            {
              auto get = [&](int64 dx, int64 dy, int64 dz) -> Real {
                const std::vector<Real>& plane = window[static_cast<usize>(dz + radius)];
                return plane[static_cast<usize>((y + dy) * nX + x + dx)];
              };
              const Real updateValue = static_cast<Real>(fn.computeUpdate(get, sc, 3));
              const Real delta = static_cast<Real>(static_cast<double>(updateValue) * timeStep);
              nextPlane[index] = static_cast<Real>(window[static_cast<usize>(radius)][index] + delta);
              return;
            }
            {
              auto get = [&](int64 dx, int64 dy, int64 dz) -> Real {
                const std::vector<Real>& plane = window[static_cast<usize>(dz + radius)];
                const int64 sampleY = clampi(y + dy, nY - 1);
                const int64 sampleX = clampi(x + dx, nX - 1);
                return plane[static_cast<usize>(sampleY * nX + sampleX)];
              };
              const Real updateValue = static_cast<Real>(fn.computeUpdate(get, sc, 3));
              const Real delta = static_cast<Real>(static_cast<double>(updateValue) * timeStep);
              nextPlane[index] = static_cast<Real>(window[static_cast<usize>(radius)][index] + delta);
            }
          };
          if(!rowIsInterior || interiorX.begin == interiorX.end)
          {
            for(int64 x = 0; x < nX; ++x)
            {
              calculateValue(x, false);
            }
            continue;
          }
          for(usize x = 0; x < interiorX.begin; ++x)
          {
            calculateValue(static_cast<int64>(x), false);
          }
          for(usize x = interiorX.begin; x < interiorX.end; ++x)
          {
            calculateValue(static_cast<int64>(x), true);
          }
          for(usize x = interiorX.end; x < static_cast<usize>(nX); ++x)
          {
            calculateValue(static_cast<int64>(x), false);
          }
        }
      };
      ParallelDataAlgorithm algorithm;
      algorithm.setRange(0, static_cast<usize>(nY));
      algorithm.execute(calculateRows);
      if(finalIteration)
      {
        for(usize index = 0; index < slice; ++index)
        {
          finalOutputPlane[index] = static_cast<T>(nextPlane[index]);
        }
        if(Result<> result = outStore.copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const T>(finalOutputPlane.data(), slice)); result.invalid())
        {
          return result;
        }
      }
      else if(Result<> result = destinationStore->copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const Real>(nextPlane.data(), slice)); result.invalid())
      {
        return result;
      }
      if(z + 1 < nZ)
      {
        for(usize slot = 0; slot + 1 < windowPlanes; ++slot)
        {
          std::swap(window[slot], window[slot + 1]);
          std::swap(windowZ[slot], windowZ[slot + 1]);
        }
        if(Result<> result = loadWindowPlane(windowPlanes - 1, z + 1 + radius); result.invalid())
        {
          return result;
        }
      }
    }
    return {};
  };

  for(uint32 iter = 0; iter < numberOfIterations; ++iter)
  {
    if(shouldCancel)
    {
      return {};
    }
    if constexpr(F::k_NeedsGlobalGradient)
    {
      const bool initializedConductance = iter == 0 && !fn.gradientMagnitudeFixed;
      if(!initializedConductance)
      {
        if(Result<> result = detail::UpdateGlobalConductance<F, Real>(fn, *currentStore, dims, sc, 3, iter, shouldCancel); result.invalid())
        {
          return result;
        }
      }
    }

    const bool finalIteration = iter + 1 == numberOfIterations;
    if constexpr(!F::k_NeedsGlobalGradient)
    {
      if(iter == 0)
      {
        std::optional<int64> loadedInputZ;
        auto loadInputPlane = [&](std::vector<Real>& destination, int64 wantedZ) -> Result<> {
          const int64 z = clampi(wantedZ, nZ - 1);
          if(!loadedInputZ.has_value() || *loadedInputZ != z)
          {
            if(Result<> result = inStore.copyIntoBuffer(static_cast<usize>(z) * slice, nonstd::span<T>(inputCastPlane.data(), slice)); result.invalid())
            {
              return result;
            }
            loadedInputZ = z;
          }
          for(usize index = 0; index < slice; ++index)
          {
            destination[index] = static_cast<Real>(inputCastPlane[index]);
          }
          return {};
        };
        if(Result<> result = processIteration(loadInputPlane, &firstStore, finalIteration); result.invalid())
        {
          return result;
        }
        if(finalIteration)
        {
          throttleProgress(messageHandler, iter, numberOfIterations);
          return {};
        }
        currentStore = &firstStore;
        nextStore = &secondStore;
        throttleProgress(messageHandler, iter, numberOfIterations);
        continue;
      }
    }

    auto loadWorkingPlane = [&](std::vector<Real>& destination, int64 wantedZ) -> Result<> {
      const int64 z = clampi(wantedZ, nZ - 1);
      return currentStore->copyIntoBuffer(static_cast<usize>(z) * slice, nonstd::span<Real>(destination.data(), slice));
    };
    if(Result<> result = processIteration(loadWorkingPlane, nextStore, finalIteration); result.invalid())
    {
      return result;
    }
    if(finalIteration)
    {
      throttleProgress(messageHandler, iter, numberOfIterations);
      return {};
    }
    std::swap(currentStore, nextStore);
    throttleProgress(messageHandler, iter, numberOfIterations);
  }
  return {};
}

template <class T, class F, class Real>
Result<> ApplyFiniteDifferenceOocRaw(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, const std::array<double, 3>& sc, F fn, float64 timeStep,
                                     uint32 numberOfIterations, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize residentLimit2D)
{
  const usize nx = dims[0];
  const usize ny = dims[1];
  const usize nz = dims[2];
  const usize slice = nx * ny;
  const usize volume = slice * nz;
  usize maxRecordsPerBatch = slice;
  if(nz == 1)
  {
    const usize radius = static_cast<usize>(fn.radius());
    const usize planRadius = std::max(radius, static_cast<usize>(F::k_NeedsGlobalGradient));
    const detail::FiniteDifference2DBufferPlan plan = detail::BuildFiniteDifference2DBufferPlan(nx, ny, planRadius, sizeof(Real), residentLimit2D);
    if(plan.overflow)
    {
      return MakeErrorResult(-8635, fmt::format("Finite-difference 2D buffer plan cannot represent dimensions {} x {}, radius {}, and {}-byte working values due to arithmetic overflow.", nx, ny,
                                                planRadius, sizeof(Real)));
    }
    if(!plan.valid)
    {
      return MakeErrorResult(-8636, fmt::format("Finite-difference 2D buffer plan exceeds the {}-byte resident limit for dimensions {} x {}, radius {}, and {}-byte working values.", residentLimit2D,
                                                nx, ny, planRadius, sizeof(Real)));
    }
    usize inputRows = 0;
    usize inputCols = 0;
    usize inputCapacity = 0;
    usize outputCapacity = 0;
    if(plan.coreCols == nx)
    {
      inputRows = plan.coreRows + 2 * planRadius;
      inputCols = nx;
    }
    else
    {
      inputRows = 1 + 2 * planRadius;
      inputCols = plan.coreCols + 2 * planRadius;
    }
    if(!detail::FiniteDifferenceCheckedMultiply(inputRows, inputCols, inputCapacity) || !detail::FiniteDifferenceCheckedMultiply(plan.coreRows, plan.coreCols, outputCapacity))
    {
      return MakeErrorResult(-8635, fmt::format("Finite-difference 2D working-buffer layout overflows for dimensions {} x {} and radius {}.", nx, ny, planRadius));
    }
    maxRecordsPerBatch = std::max(inputCapacity, outputCapacity);
  }

  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(Real);
  config.maxRecordsPerBatch = static_cast<uint64>(maxRecordsPerBatch);
  config.initialRecordCount = static_cast<uint64>(volume);
  auto firstResult = CreateFiniteDifferenceScratchStore(config);
  if(firstResult.invalid())
  {
    return ConvertResult(std::move(firstResult));
  }
  auto secondResult = CreateFiniteDifferenceScratchStore(config);
  if(secondResult.invalid())
  {
    return ConvertResult(std::move(secondResult));
  }
  std::unique_ptr<ITemporaryRecordStore> firstRecords = std::move(firstResult.value());
  std::unique_ptr<ITemporaryRecordStore> secondRecords = std::move(secondResult.value());
  TemporaryFiniteDifferenceStore<Real> firstStore(*firstRecords, shouldCancel);
  TemporaryFiniteDifferenceStore<Real> secondStore(*secondRecords, shouldCancel);
  if(nz == 1)
  {
    return ApplyFiniteDifference2DBounded<T, F, Real>(inStore, outStore, firstStore, secondStore, nx, ny, sc, fn, timeStep, numberOfIterations, shouldCancel, messageHandler, residentLimit2D);
  }
  return ApplyFiniteDifference3DPingPong<T, F, Real>(inStore, outStore, firstStore, secondStore, dims, sc, fn, timeStep, numberOfIterations, shouldCancel, messageHandler);
}

// Full ITK-free FiniteDifference driver. Resident DataStore execution retains the original full-volume
// CalculateChange update buffer plus ApplyUpdate loop. Any OOC endpoint selects two RAII fixed-record raw working
// stores and fuses CalculateChange+ApplyUpdate into a frozen-current/next-store Jacobi pass. True 2-D uses a checked
// <=64 MiB row/tile plan; 3-D retains only O(plane) windows. Both routes preserve the same Real rounding, ordered
// global-gradient reduction, ZeroFluxNeumann boundary, time step, spacing scales, cancellation, and final T cast.
//
// Compute precision: `Real = F::k_NativePrecision ? T : float64`. For every functor except CurvatureAnisoFn, the
// legacy ITK bridge functor (e.g. ITKCurvatureFlowImageFilter.cpp's filter-creation functor) declares
// `using IntermediateType = float64;`, which makes ITK::Execute (ITKArrayHelper.hpp) itk::CastImageFilter the INPUT
// up to an itk::Image<float64,...> BEFORE constructing the itk filter with BOTH its InputImageType and
// OutputImageType bound to that float64 image, and cast the FINAL output back down to T ONCE, after every iteration
// has run. So even for an integer T, live ITK's own PixelType (and its UpdateBuffer) is float64 for the entire
// iteration -- NOT T -- and Real=float64 reproduces that. An earlier version of this driver truncated
// `update`/`out` to T on every iteration for ALL functors (matching a naive reading of
// FiniteDifferenceFunction::ComputeUpdate's `PixelType` return type in isolation); that discards fractional
// per-iteration progress that live ITK actually keeps for the IntermediateType=float64 functors, and was verified
// (via a live-ITK parity mismatch, root-caused against the running legacy filter) to diverge from ITK for every
// integer type. ITKCurvatureAnisotropicDiffusionImageFilter's legacy bridge functor has NO IntermediateType,
// though, so live ITK runs it NATIVELY in the input precision (PixelType==T for the whole iteration, never upcast);
// CurvatureAnisoFn sets k_NativePrecision=true so Real==T reproduces THAT model instead (verified: with Real
// hardcoded to float64, the float32-input live-ITK grid diverged from ITK by ~1 ULP at essentially every voxel --
// a deterministic precision-MODEL mismatch, not thread-order FP noise -- and resolved once Real became T here).
template <class T, class F>
Result<> ApplyFiniteDifference(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, FloatVec3 spacing, F fn, float64 timeStep, uint32 numberOfIterations,
                               const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize residentLimit2D = detail::k_FiniteDifference2DResidentLimit)
{
  using Real = std::conditional_t<F::k_NativePrecision, T, float64>;
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {};
  }
  if(dims[0] > static_cast<usize>(std::numeric_limits<int64>::max()) || dims[1] > static_cast<usize>(std::numeric_limits<int64>::max()) ||
     dims[2] > static_cast<usize>(std::numeric_limits<int64>::max()))
  {
    return MakeErrorResult(-8630, fmt::format("Finite-difference image dimensions exceed the supported signed coordinate range. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  auto checkedMultiply = [](usize left, usize right, usize& product) {
    if(left != 0 && right > std::numeric_limits<usize>::max() / left)
    {
      return false;
    }
    product = left * right;
    return true;
  };
  usize slice = 0;
  usize volume = 0;
  if(!checkedMultiply(dims[0], dims[1], slice) || !checkedMultiply(slice, dims[2], volume))
  {
    return MakeErrorResult(-8631, fmt::format("Finite-difference image dimensions overflow the addressable value count. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  if(inStore.getSize() != volume)
  {
    return MakeErrorResult(-8632, fmt::format("Finite-difference input store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                              inStore.getSize(), volume, dims[0], dims[1], dims[2]));
  }
  if(outStore.getSize() != volume)
  {
    return MakeErrorResult(-8633, fmt::format("Finite-difference output store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                              outStore.getSize(), volume, dims[0], dims[1], dims[2]));
  }
  if(shouldCancel)
  {
    return {};
  }

  const bool hasOutOfCoreEndpoint = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(hasOutOfCoreEndpoint && detail::ShouldUseFiniteDifferenceResidentState(dims, numberOfIterations))
  {
    auto allocationResult = detail::ReserveFiniteDifferenceResidentWorkingMemory<T, Real>(dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    if(allocation.holdsCompleteState())
    {
      try
      {
        DataStore<T> residentInput(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
        DataStore<T> residentOutput(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
        if(Result<> result = inStore.copyIntoBuffer(0, residentInput.createSpan()); result.invalid())
        {
          return result;
        }
        if(Result<> result = ApplyFiniteDifference(residentInput, residentOutput, dims, spacing, fn, timeStep, numberOfIterations, shouldCancel, messageHandler, residentLimit2D); result.invalid())
        {
          return result;
        }
        if(shouldCancel)
        {
          return {};
        }
        const auto outputSpan = residentOutput.createSpan();
        return outStore.copyFromBuffer(0, nonstd::span<const T>(outputSpan.data(), outputSpan.size()));
      } catch(const std::bad_alloc&)
      {
        // Release the complete-state reservation before entering the raw-record fallback.
      }
    }
  }
  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  const uint32 effDim = (nZ > 1) ? 3u : 2u;
  const int64 r = fn.radius();
  if(r < 0 || r > (std::numeric_limits<int64>::max() - 1) / 2)
  {
    return MakeErrorResult(-8634, fmt::format("Finite-difference neighborhood radius must be nonnegative and addressable. Radius: {}.", r));
  }
  // sc[i] = 1/spacing[i] (ITK InitializeFunctionCoefficients, UseImageSpacing on). A zero/near-zero spacing yields inf
  // here -- this MATCHES ITK's own unguarded 1.0/spacing[i], so (per the "ITK-faithful guard only" decision) no spacing
  // guard is added; the degenerate input is the caller's responsibility, as in ITK.
  const std::array<double, 3> sc = {1.0 / static_cast<double>(spacing[0]), 1.0 / static_cast<double>(spacing[1]), 1.0 / static_cast<double>(spacing[2])};
  auto clampi = [](int64 v, int64 hi) { return v < 0 ? int64{0} : (v > hi ? hi : v); };
  const std::vector<usize> shape{static_cast<usize>(nZ), static_cast<usize>(nY), static_cast<usize>(nX)};

  if(hasOutOfCoreEndpoint)
  {
    return ApplyFiniteDifferenceOocRaw<T, F, Real>(inStore, outStore, dims, sc, fn, timeStep, numberOfIterations, shouldCancel, messageHandler, residentLimit2D);
  }

  // Resident/fallback stores: accum (Real) = cast<Real>(in), streamed plane by plane -- ITK's IntermediateType
  // cast-IN (a no-op whenever Real==T, i.e. F::k_NativePrecision).
  const auto workingDataFormat = detail::SelectFiniteDifferenceWorkingDataFormat(inStore.getStoreType(), inStore.getDataFormat(), outStore.getStoreType(), outStore.getDataFormat());
  auto accumPtr = DataStoreUtilities::CreateDataStoreWithFormat<Real>(workingDataFormat, shape, std::vector<usize>{1});
  AbstractDataStore<Real>& accum = *accumPtr;

  // Resident/fallback update buffer: a second full-volume Real store (ITK's UpdateBufferType == the IntermediateType
  // image, or T itself for the native-precision functor).
  auto updatePtr = DataStoreUtilities::CreateDataStoreWithFormat<Real>(workingDataFormat, shape, std::vector<usize>{1});
  AbstractDataStore<Real>& updateStore = *updatePtr;

  {
    std::vector<T> planeIn(slice);
    std::vector<Real> planeOut(slice);
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> rr = inStore.copyIntoBuffer(static_cast<usize>(z) * slice, nonstd::span<T>(planeIn.data(), slice)); rr.invalid())
      {
        return rr;
      }
      for(usize i = 0; i < slice; ++i)
      {
        planeOut[i] = static_cast<Real>(planeIn[i]);
      }
      if(Result<> rr = accum.copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const Real>(planeOut.data(), slice)); rr.invalid())
      {
        return rr;
      }
    }
  }

  auto* inMemoryAccumStore = dynamic_cast<DataStore<Real>*>(&accum);
  auto* inMemoryUpdateStore = dynamic_cast<DataStore<Real>*>(&updateStore);
  auto* inMemoryOutputStore = dynamic_cast<DataStore<T>*>(&outStore);
  if(inMemoryAccumStore != nullptr && inMemoryUpdateStore != nullptr && inMemoryOutputStore != nullptr)
  {
    nonstd::span<Real> accumValues = inMemoryAccumStore->createSpan();
    nonstd::span<Real> updateValues = inMemoryUpdateStore->createSpan();
    nonstd::span<T> outputValues = inMemoryOutputStore->createSpan();

    for(uint32 iter = 0; iter < numberOfIterations; ++iter)
    {
      if(shouldCancel)
      {
        return {};
      }
      if constexpr(F::k_NeedsGlobalGradient)
      {
        if(Result<> result = detail::UpdateGlobalConductance<F, Real>(fn, accum, dims, sc, effDim, iter, shouldCancel); result.invalid())
        {
          return result;
        }
      }

      auto calculateChange = [&](const Range& range) {
        for(usize tuple = range.min(); tuple < range.max(); ++tuple)
        {
          if(((tuple - range.min()) & 4095ULL) == 0 && shouldCancel)
          {
            return;
          }
          const int64 z = static_cast<int64>(tuple / slice);
          const usize planeIndex = tuple - static_cast<usize>(z) * slice;
          const int64 y = static_cast<int64>(planeIndex / static_cast<usize>(nX));
          const int64 x = static_cast<int64>(planeIndex - static_cast<usize>(y) * static_cast<usize>(nX));
          if(volume <= static_cast<usize>(std::numeric_limits<int64>::max()) &&
             detail::FiniteDifferenceNeighborhoodIsInterior(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dims, r, effDim))
          {
            const int64 centerIndex = static_cast<int64>(tuple);
            const int64 signedSlice = static_cast<int64>(slice);
            auto get = [&](int64 dx, int64 dy, int64 dz) -> Real {
              const int64 neighborIndex = centerIndex + dz * signedSlice + dy * nX + dx;
              return accumValues[static_cast<usize>(neighborIndex)];
            };
            updateValues[tuple] = static_cast<Real>(fn.computeUpdate(get, sc, effDim));
          }
          else
          {
            auto get = [&](int64 dx, int64 dy, int64 dz) -> Real {
              const int64 neighborX = clampi(x + dx, nX - 1);
              const int64 neighborY = clampi(y + dy, nY - 1);
              const int64 neighborZ = clampi(z + dz, nZ - 1);
              const usize neighborIndex = (static_cast<usize>(neighborZ) * dims[1] + static_cast<usize>(neighborY)) * dims[0] + static_cast<usize>(neighborX);
              return accumValues[neighborIndex];
            };
            updateValues[tuple] = static_cast<Real>(fn.computeUpdate(get, sc, effDim));
          }
        }
      };
      ParallelDataAlgorithm calculateAlgorithm;
      calculateAlgorithm.setRange(0, volume);
      calculateAlgorithm.execute(calculateChange);
      if(shouldCancel)
      {
        return {};
      }

      auto applyUpdate = [&](const Range& range) {
        for(usize tuple = range.min(); tuple < range.max(); ++tuple)
        {
          if(((tuple - range.min()) & 4095ULL) == 0 && shouldCancel)
          {
            return;
          }
          const Real delta = static_cast<Real>(static_cast<double>(updateValues[tuple]) * timeStep);
          accumValues[tuple] = static_cast<Real>(accumValues[tuple] + delta);
        }
      };
      ParallelDataAlgorithm applyAlgorithm;
      applyAlgorithm.setRange(0, volume);
      applyAlgorithm.execute(applyUpdate);
      if(shouldCancel)
      {
        return {};
      }
      throttleProgress(messageHandler, iter, numberOfIterations);
    }

    auto castOutput = [&](const Range& range) {
      for(usize tuple = range.min(); tuple < range.max(); ++tuple)
      {
        if(((tuple - range.min()) & 4095ULL) == 0 && shouldCancel)
        {
          return;
        }
        outputValues[tuple] = static_cast<T>(accumValues[tuple]);
      }
    };
    ParallelDataAlgorithm castAlgorithm;
    castAlgorithm.setRange(0, volume);
    castAlgorithm.execute(castOutput);
    return {};
  }

  // Non-DataStore resident fallback: two Real stores are ping-ponged. CalculateChange and ApplyUpdate are fused per
  // output plane: every value reads only the frozen current store and writes only the next store, preserving Jacobi
  // semantics while eliminating the materialized-update write/read and the second current read/write pass.
  const usize windowPlanes = static_cast<usize>(2 * r + 1);
  std::vector<std::vector<Real>> win(windowPlanes, std::vector<Real>(slice));
  std::vector<Real> nextPlane(slice);
  AbstractDataStore<Real>* currentStore = &accum;
  AbstractDataStore<Real>* nextStore = &updateStore;

  auto loadPlane = [&](std::vector<Real>& dst, int64 zWanted) -> Result<> {
    const int64 zc = clampi(zWanted, nZ - 1);
    return currentStore->copyIntoBuffer(static_cast<usize>(zc) * slice, nonstd::span<Real>(dst.data(), slice));
  };

  for(uint32 iter = 0; iter < numberOfIterations; ++iter)
  {
    if(shouldCancel)
    {
      return {};
    }

    // (aniso) global reduction hook -- no-op unless F::k_NeedsGlobalGradient (Task 4 specializes this).
    if constexpr(F::k_NeedsGlobalGradient)
    {
      if(Result<> rr = detail::UpdateGlobalConductance<F, Real>(fn, *currentStore, dims, sc, effDim, iter, shouldCancel); rr.invalid())
      {
        return rr;
      }
    }

    // ---- Fused CalculateChange + ApplyUpdate: fill nextStore from the frozen currentStore neighborhood ----
    // seed the window at z = -r .. +r
    for(int64 k = 0; k < static_cast<int64>(windowPlanes); ++k)
    {
      if(Result<> rr = loadPlane(win[static_cast<usize>(k)], (k - r)); rr.invalid())
      {
        return rr;
      }
    }
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      // In-plane voxel loop, parallelized over the row range [0, nY). Each `nextPlane[y*nX+x]` depends ONLY on the
      // frozen (read-only) rolling window `win`, the shared const functor `fn`, and `sc` -- there is no shared mutable
      // state, and every worker writes a DISJOINT `updatePlane` index (distinct (x,y)), so the result is independent of
      // worker order and BYTE-EXACT with the former serial loop. `win` is loaded/advanced SERIALLY outside this region
      // (see below), so no worker ever mutates it. `fn` is shared by const reference (NOT copied per worker): its
      // global-gradient state was set by the serial UpdateGlobalConductance before CalculateChange and is only READ
      // here, and computeUpdate is const on every functor -- concurrent const calls on one shared `fn` are safe and
      // required. This mirrors the RecursiveGaussianEngine / MorphologyEngine in-plane parallelization pattern.
      auto calculateChangeRows = [&](const Range& rowRange) {
        for(usize yu = rowRange.min(); yu < rowRange.max(); ++yu)
        {
          const int64 y = static_cast<int64>(yu);
          for(int64 x = 0; x < nX; ++x)
          {
            // Clamped neighborhood accessor. Invariant: win[k] holds plane clamp(z - r + k) (z-clamping done at load), so
            // the slot for a wanted z-offset dzo (always in [-r, r]) is exactly (dzo + r); x/y are clamped in-plane.
            // Returns the RAW Real sample -- NOT widened to double here -- matching ITK's own it.GetPixel(), which
            // returns PixelType. This matters whenever Real is narrower than double (CurvatureAnisoFn's
            // native-precision float32 case): ITK's ComputeUpdate SUBTRACTS two PixelType neighbor reads in PixelType
            // (float32) arithmetic first (e.g. `dx_forward[i] = it.GetPixel(+) - it.GetPixel(center)`), and only THEN
            // widens the finished difference into a `double` local. Eagerly promoting each individual sample to
            // double before subtracting (as an earlier version of this accessor did) computes a DIFFERENT, more
            // precise difference than ITK's own float32 subtraction whenever that difference is not exactly
            // representable in float32 -- live-ITK parity testing caught this as a small but iteration-over-iteration
            // GROWING ULP divergence (not bounded reduction-order noise), so functors that read Real-precision
            // neighbors and need bit-exact parity (CurvatureAnisoFn) must perform their own raw-difference-then-widen
            // in computeUpdate; functors whose Real is always float64 (every other functor) see no behavior change
            // (Real==double makes this accessor identical to the old always-double version).
            auto get = [&](int64 dxo, int64 dyo, int64 dzo) -> Real {
              const std::vector<Real>& pl = win[static_cast<usize>(dzo + r)];
              const int64 yy = clampi(y + dyo, nY - 1);
              const int64 xx = clampi(x + dxo, nX - 1);
              return pl[static_cast<usize>(yy * nX + xx)];
            };
            const usize index = static_cast<usize>(y) * static_cast<usize>(nX) + static_cast<usize>(x);
            // Preserve the former two-phase rounding exactly: narrow computeUpdate to Real, narrow update*timeStep
            // to Real, then add center+delta in Real before assigning the next-store value.
            const Real updateValue = static_cast<Real>(fn.computeUpdate(get, sc, effDim));
            const Real delta = static_cast<Real>(static_cast<double>(updateValue) * timeStep);
            nextPlane[index] = static_cast<Real>(get(0, 0, 0) + delta);
          }
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, static_cast<usize>(nY));
      parallelAlgorithm.execute(calculateChangeRows);
      if(Result<> rr = nextStore->copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const Real>(nextPlane.data(), slice)); rr.invalid())
      {
        return rr;
      }
      // advance the rolling window by one z (drop the oldest plane, load clamp(z+1+r))
      if(z + 1 < nZ)
      {
        for(usize k = 0; k + 1 < windowPlanes; ++k)
        {
          std::swap(win[k], win[k + 1]);
        }
        if(Result<> rr = loadPlane(win[windowPlanes - 1], z + 1 + r); rr.invalid())
        {
          return rr;
        }
      }
    }
    std::swap(currentStore, nextStore);
    throttleProgress(messageHandler, iter, numberOfIterations); // decile progress like RecursiveGaussianEngine
  }

  // 3) outStore = cast<T>(currentStore), streamed -- the ONE-TIME final narrowing (a no-op cast when Real==T).
  {
    std::vector<Real> planeIn(slice);
    std::vector<T> planeOut(slice);
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> rr = currentStore->copyIntoBuffer(static_cast<usize>(z) * slice, nonstd::span<Real>(planeIn.data(), slice)); rr.invalid())
      {
        return rr;
      }
      for(usize i = 0; i < slice; ++i)
      {
        planeOut[i] = static_cast<T>(planeIn[i]);
      }
      if(Result<> rr = outStore.copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const T>(planeOut.data(), slice)); rr.invalid())
      {
        return rr;
      }
    }
  }
  return {};
}
} // namespace nx::core::ImageProcessing
