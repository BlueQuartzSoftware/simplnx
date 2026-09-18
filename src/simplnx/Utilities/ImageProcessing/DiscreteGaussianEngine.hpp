#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp" // CreateDataStoreWithFormat (working store)
#include "simplnx/Utilities/ImageProcessing/GaussianTemporaryStore.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
struct DiscreteGaussianPassOptions
{
  static constexpr usize k_MaxStagingBytes = 16ULL * 1024ULL * 1024ULL;
  static constexpr usize k_Default2DResidentLimit = 64ULL * 1024ULL * 1024ULL;
  usize stagingByteBudget = k_MaxStagingBytes;
  usize residentLimit2D = k_Default2DResidentLimit;
  bool forceFusedPlanePass = false;
  bool forceBounded2D = false;
};

namespace detail
{
struct DiscreteGaussianStoragePlan
{
  bool useFusedPlanePass = false;
  bool useTemporaryRecordStore = false;
  std::string workingDataFormat;
};

struct DiscreteGaussian2DBufferPlan
{
  usize coreRows = 0;
  usize coreCols = 0;
  usize residentBytes = 0;
  bool valid = false;
  bool overflow = false;
};

inline bool DiscreteGaussianCheckedAdd(usize left, usize right, usize& sum) noexcept
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

inline bool DiscreteGaussianCheckedMultiply(usize left, usize right, usize& product) noexcept
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline DiscreteGaussian2DBufferPlan BuildDiscreteGaussian2DBufferPlan(usize dimX, usize dimY, usize radiusX, usize radiusY, usize valueBytes, usize residentLimit)
{
  DiscreteGaussian2DBufferPlan plan;
  if(dimX == 0 || dimY == 0 || valueBytes == 0 || residentLimit == 0)
  {
    return plan;
  }

  usize twiceRadiusY = 0;
  usize haloRowsValues = 0;
  usize valuesPerCoreRow = 0;
  usize fullWidthFixedValues = 0;
  if(!DiscreteGaussianCheckedMultiply(radiusY, 2, twiceRadiusY) || !DiscreteGaussianCheckedMultiply(twiceRadiusY, dimX, haloRowsValues) ||
     !DiscreteGaussianCheckedMultiply(dimX, 3, valuesPerCoreRow) || !DiscreteGaussianCheckedAdd(haloRowsValues, valuesPerCoreRow, fullWidthFixedValues))
  {
    plan.overflow = true;
    return plan;
  }
  const usize residentValues = residentLimit / valueBytes;
  if(fullWidthFixedValues <= residentValues)
  {
    const usize coreRows = std::min(dimY, (residentValues - haloRowsValues) / valuesPerCoreRow);
    usize sourceRows = 0;
    usize sourceValues = 0;
    usize coreValues = 0;
    usize totalValues = 0;
    if(coreRows == 0 || !DiscreteGaussianCheckedAdd(coreRows, twiceRadiusY, sourceRows) || !DiscreteGaussianCheckedMultiply(sourceRows, dimX, sourceValues) ||
       !DiscreteGaussianCheckedMultiply(coreRows, dimX, coreValues) || !DiscreteGaussianCheckedAdd(sourceValues, coreValues, totalValues) ||
       !DiscreteGaussianCheckedAdd(totalValues, coreValues, totalValues) || !DiscreteGaussianCheckedMultiply(totalValues, valueBytes, plan.residentBytes))
    {
      plan.overflow = coreRows != 0;
      return plan;
    }
    plan.coreRows = coreRows;
    plan.coreCols = dimX;
    plan.valid = plan.residentBytes <= residentLimit;
    return plan;
  }

  usize twiceRadiusX = 0;
  usize rowsPlusIntermediate = 0;
  usize fixedValues = 0;
  usize valuesPerCoreCol = 0;
  if(!DiscreteGaussianCheckedMultiply(radiusX, 2, twiceRadiusX) || !DiscreteGaussianCheckedAdd(twiceRadiusY, 2, rowsPlusIntermediate) ||
     !DiscreteGaussianCheckedMultiply(rowsPlusIntermediate, twiceRadiusX, fixedValues) || !DiscreteGaussianCheckedAdd(rowsPlusIntermediate, 1, valuesPerCoreCol))
  {
    plan.overflow = true;
    return plan;
  }
  if(fixedValues >= residentValues)
  {
    return plan;
  }
  const usize coreCols = std::min(dimX, (residentValues - fixedValues) / valuesPerCoreCol);
  usize inputCols = 0;
  usize sourceValues = 0;
  usize totalValues = 0;
  if(coreCols == 0 || !DiscreteGaussianCheckedAdd(coreCols, twiceRadiusX, inputCols) || !DiscreteGaussianCheckedMultiply(rowsPlusIntermediate, inputCols, sourceValues) ||
     !DiscreteGaussianCheckedAdd(sourceValues, coreCols, totalValues) || !DiscreteGaussianCheckedMultiply(totalValues, valueBytes, plan.residentBytes))
  {
    plan.overflow = coreCols != 0;
    return plan;
  }
  plan.coreRows = 1;
  plan.coreCols = coreCols;
  plan.valid = plan.residentBytes <= residentLimit;
  return plan;
}

inline DiscreteGaussianStoragePlan SelectDiscreteGaussianStoragePlan(IDataStore::StoreType inputStoreType, std::string inputDataFormat, IDataStore::StoreType outputStoreType,
                                                                     std::string outputDataFormat, bool forceFusedPlanePass = false)
{
  if(inputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return {true, true, std::move(inputDataFormat)};
  }
  if(outputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return {true, true, std::move(outputDataFormat)};
  }
  return {forceFusedPlanePass, false, std::move(inputDataFormat)};
}

// Verbatim port of itk::GaussianOperator's modified-Bessel functions (all double).
inline double ModifiedBesselI0(double y)
{
  const double d = std::abs(y);
  if(d < 3.75)
  {
    double m = y / 3.75;
    m *= m;
    return 1.0 + m * (3.5156229 + m * (3.0899424 + m * (1.2067492 + m * (0.2659732 + m * (0.360768e-1 + m * 0.45813e-2)))));
  }
  const double m = 3.75 / d;
  return (std::exp(d) / std::sqrt(d)) *
         (0.39894228 + m * (0.1328592e-1 + m * (0.225319e-2 + m * (-0.157565e-2 + m * (0.916281e-2 + m * (-0.2057706e-1 + m * (0.2635537e-1 + m * (-0.1647633e-1 + m * 0.392377e-2))))))));
}

inline double ModifiedBesselI1(double y)
{
  const double d = std::abs(y);
  double accumulator;
  if(d < 3.75)
  {
    double m = y / 3.75;
    m *= m;
    accumulator = d * (0.5 + m * (0.87890594 + m * (0.51498869 + m * (0.15084934 + m * (0.2658733e-1 + m * (0.301532e-2 + m * 0.32411e-3))))));
  }
  else
  {
    const double m = 3.75 / d;
    accumulator = 0.2282967e-1 + m * (-0.2895312e-1 + m * (0.1787654e-1 - m * 0.420059e-2));
    accumulator = 0.39894228 + m * (-0.3988024e-1 + m * (-0.362018e-2 + m * (0.163801e-2 + m * (-0.1031555e-1 + m * accumulator))));
    accumulator *= (std::exp(d) / std::sqrt(d));
  }
  return (y < 0.0) ? -accumulator : accumulator;
}

inline double ModifiedBesselI(int n, double y)
{
  constexpr double ACCURACY = 40.0;
  if(y == 0.0)
  {
    return 0.0;
  }
  const double toy = 2.0 / std::abs(y);
  double qip = 0.0;
  double accumulator = 0.0;
  double qi = 1.0;
  for(int j = 2 * (n + static_cast<int>(std::sqrt(ACCURACY * n))); j > 0; j--)
  {
    const double qim = qip + j * toy * qi;
    qip = qi;
    qi = qim;
    if(std::abs(qi) > 1.0e10)
    {
      accumulator *= 1.0e-10;
      qi *= 1.0e-10;
      qip *= 1.0e-10;
    }
    if(j == n)
    {
      accumulator = qip;
    }
  }
  accumulator *= ModifiedBesselI0(y) / qi;
  return (y < 0.0 && (n & 1)) ? -accumulator : accumulator;
}

// Verbatim port of GaussianOperator::GenerateCoefficients. `variance` is already spacing-adjusted by the caller.
// Returns the symmetric, sum-normalized kernel (odd length). All arithmetic in double.
inline std::vector<double> GaussianKernelCoefficients(double variance, double maximumError, uint32 maximumKernelWidth)
{
  std::vector<double> coeff;
  const double et = std::exp(-variance);
  const double cap = 1.0 - maximumError;
  double sum = 0.0;
  coeff.push_back(et * ModifiedBesselI0(variance));
  sum += coeff[0];
  coeff.push_back(et * ModifiedBesselI1(variance));
  sum += coeff[1] * 2.0;
  for(int i = 2; sum < cap; ++i)
  {
    coeff.push_back(et * ModifiedBesselI(i, variance));
    sum += coeff[static_cast<usize>(i)] * 2.0;
    if(coeff[static_cast<usize>(i)] <= 0.0)
    {
      break;
    }
    if(coeff.size() > maximumKernelWidth)
    {
      break;
    }
  }
  for(double& c : coeff)
  {
    c /= sum;
  }
  // Mirror the one-sided coefficients (index 0 = center) into a symmetric kernel of length 2*(size-1)+1.
  const int j = static_cast<int>(coeff.size()) - 1;
  coeff.insert(coeff.begin(), static_cast<usize>(j), 0.0);
  for(int i = 0; i < j; ++i)
  {
    coeff[static_cast<usize>(i)] = coeff[coeff.size() - 1 - static_cast<usize>(i)];
  }
  return coeff;
}

// Applies the Y pass into a T-typed intermediate plane and then the X pass into the reusable source plane. Keeping the
// intermediate plane in T preserves the per-axis cast/truncation performed by the ordinary store-backed cascade.
template <class T, class SrcStoreT, class DstStoreT>
Result<> DiscreteGaussianYXPlanePass(const SrcStoreT& src, DstStoreT& dst, const SizeVec3& dims, const std::vector<double>& yKernel, const std::vector<double>& xKernel,
                                     const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize planeValues = dimX * dimY;
  const int64 dimX64 = static_cast<int64>(dimX);
  const int64 dimY64 = static_cast<int64>(dimY);
  const int64 yRadius = static_cast<int64>(yKernel.size() / 2);
  const int64 xRadius = static_cast<int64>(xKernel.size() / 2);
  auto clampi = [](int64 value, int64 high) { return value < 0 ? int64{0} : (value > high ? high : value); };

  std::vector<T> sourcePlane(planeValues);
  std::vector<T> intermediatePlane(planeValues);
  for(usize z = 0; z < dimZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize planeOffset = z * planeValues;
    if(Result<> result = src.copyIntoBuffer(planeOffset, nonstd::span<T>(sourcePlane.data(), planeValues)); result.invalid())
    {
      return result;
    }

    auto convolveColumns = [&](const Range& columnRange) {
      std::vector<double> lineIn(dimY);
      for(usize x = columnRange.min(); x < columnRange.max(); ++x)
      {
        if(shouldCancel)
        {
          return;
        }
        for(usize y = 0; y < dimY; ++y)
        {
          lineIn[y] = static_cast<double>(sourcePlane[y * dimX + x]);
        }
        for(int64 y = 0; y < dimY64; ++y)
        {
          double accumulator = 0.0;
          for(int64 k = 0; k < static_cast<int64>(yKernel.size()); ++k)
          {
            accumulator += lineIn[static_cast<usize>(clampi(y + (k - yRadius), dimY64 - 1))] * yKernel[static_cast<usize>(k)];
          }
          intermediatePlane[static_cast<usize>(y) * dimX + x] = static_cast<T>(accumulator);
        }
      }
    };
    ParallelDataAlgorithm columnAlgorithm;
    columnAlgorithm.setRange(0, dimX);
    columnAlgorithm.execute(convolveColumns);

    if(shouldCancel)
    {
      return {};
    }

    auto convolveRows = [&](const Range& rowRange) {
      std::vector<double> lineIn(dimX);
      for(usize y = rowRange.min(); y < rowRange.max(); ++y)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize rowOffset = y * dimX;
        for(usize x = 0; x < dimX; ++x)
        {
          lineIn[x] = static_cast<double>(intermediatePlane[rowOffset + x]);
        }
        for(int64 x = 0; x < dimX64; ++x)
        {
          double accumulator = 0.0;
          for(int64 k = 0; k < static_cast<int64>(xKernel.size()); ++k)
          {
            accumulator += lineIn[static_cast<usize>(clampi(x + (k - xRadius), dimX64 - 1))] * xKernel[static_cast<usize>(k)];
          }
          sourcePlane[rowOffset + static_cast<usize>(x)] = static_cast<T>(accumulator);
        }
      }
    };
    ParallelDataAlgorithm rowAlgorithm;
    rowAlgorithm.setRange(0, dimY);
    rowAlgorithm.execute(convolveRows);

    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = dst.copyFromBuffer(planeOffset, nonstd::span<const T>(sourcePlane.data(), planeValues)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class T, class SrcStoreT, class DstStoreT>
Result<> DiscreteGaussian2DBounded(const SrcStoreT& src, DstStoreT& dst, const SizeVec3& dims, const std::vector<double>& yKernel, const std::vector<double>& xKernel,
                                   const std::atomic_bool& shouldCancel, usize residentLimit)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize radiusX = xKernel.size() / 2;
  const usize radiusY = yKernel.size() / 2;
  const DiscreteGaussian2DBufferPlan plan = BuildDiscreteGaussian2DBufferPlan(dimX, dimY, radiusX, radiusY, sizeof(T), residentLimit);
  if(plan.overflow)
  {
    return MakeErrorResult(-8364, fmt::format("Discrete Gaussian 2D buffer planning overflowed for dimensions {} x {}, radii {} x {}, and {}-byte values.", dimX, dimY, radiusX, radiusY, sizeof(T)));
  }
  if(!plan.valid)
  {
    return MakeErrorResult(
        -8365, fmt::format("Discrete Gaussian 2D execution cannot fit its minimum bounded tile within the {}-byte resident limit for dimensions {} x {}, radii {} x {}, and {}-byte values.",
                           residentLimit, dimX, dimY, radiusX, radiusY, sizeof(T)));
  }

  const int64 dimX64 = static_cast<int64>(dimX);
  const int64 dimY64 = static_cast<int64>(dimY);
  const int64 radiusX64 = static_cast<int64>(radiusX);
  const int64 radiusY64 = static_cast<int64>(radiusY);
  auto clampIndex = [](int64 value, int64 high) { return value < 0 ? int64{0} : (value > high ? high : value); };

  if(plan.coreCols == dimX)
  {
    usize coreRowsPerBlock = plan.coreRows;
    const std::optional<ShapeType> outputChunkShape = dst.getChunkShape();
    if(outputChunkShape.has_value() && outputChunkShape->size() >= 3 && (*outputChunkShape)[0] == 1 && (*outputChunkShape)[1] > 0 && (*outputChunkShape)[1] <= coreRowsPerBlock &&
       (*outputChunkShape)[2] == dimX)
    {
      coreRowsPerBlock = (coreRowsPerBlock / (*outputChunkShape)[1]) * (*outputChunkShape)[1];
    }
    std::vector<T> source;
    std::vector<T> intermediate;
    std::vector<T> output;
    for(usize yBegin = 0; yBegin < dimY; yBegin += coreRowsPerBlock)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize yEnd = std::min(dimY, yBegin + coreRowsPerBlock);
      const usize inputYBegin = yBegin > radiusY ? yBegin - radiusY : 0;
      const usize inputYEnd = std::min(dimY, yEnd + radiusY);
      const usize inputRows = inputYEnd - inputYBegin;
      const usize coreRows = yEnd - yBegin;
      source.resize(inputRows * dimX);
      intermediate.resize(coreRows * dimX);
      output.resize(coreRows * dimX);
      const usize topHaloRows = yBegin - inputYBegin;
      const usize bottomHaloRows = inputYEnd - yEnd;
      if(topHaloRows > 0)
      {
        if(Result<> result = src.copyIntoBuffer(inputYBegin * dimX, nonstd::span<T>(source.data(), topHaloRows * dimX)); result.invalid())
        {
          return result;
        }
      }
      if(Result<> result = src.copyIntoBuffer(yBegin * dimX, nonstd::span<T>(source.data() + topHaloRows * dimX, coreRows * dimX)); result.invalid())
      {
        return result;
      }
      if(bottomHaloRows > 0)
      {
        if(Result<> result = src.copyIntoBuffer(yEnd * dimX, nonstd::span<T>(source.data() + (topHaloRows + coreRows) * dimX, bottomHaloRows * dimX)); result.invalid())
        {
          return result;
        }
      }

      auto convolveY = [&](const Range& columnRange) {
        for(usize x = columnRange.min(); x < columnRange.max(); ++x)
        {
          for(usize localY = 0; localY < coreRows; ++localY)
          {
            double accumulator = 0.0;
            const int64 globalY = static_cast<int64>(yBegin + localY);
            for(int64 kernelIndex = 0; kernelIndex < static_cast<int64>(yKernel.size()); ++kernelIndex)
            {
              const usize sourceY = static_cast<usize>(clampIndex(globalY + kernelIndex - radiusY64, dimY64 - 1));
              accumulator += static_cast<double>(source[(sourceY - inputYBegin) * dimX + x]) * yKernel[static_cast<usize>(kernelIndex)];
            }
            intermediate[localY * dimX + x] = static_cast<T>(accumulator);
          }
        }
      };
      ParallelDataAlgorithm yAlgorithm;
      yAlgorithm.setRange(0, dimX);
      yAlgorithm.execute(convolveY);
      if(shouldCancel)
      {
        return {};
      }

      auto convolveX = [&](const Range& rowRange) {
        for(usize localY = rowRange.min(); localY < rowRange.max(); ++localY)
        {
          const usize rowOffset = localY * dimX;
          for(int64 x = 0; x < dimX64; ++x)
          {
            double accumulator = 0.0;
            for(int64 kernelIndex = 0; kernelIndex < static_cast<int64>(xKernel.size()); ++kernelIndex)
            {
              const usize sourceX = static_cast<usize>(clampIndex(x + kernelIndex - radiusX64, dimX64 - 1));
              accumulator += static_cast<double>(intermediate[rowOffset + sourceX]) * xKernel[static_cast<usize>(kernelIndex)];
            }
            output[rowOffset + static_cast<usize>(x)] = static_cast<T>(accumulator);
          }
        }
      };
      ParallelDataAlgorithm xAlgorithm;
      xAlgorithm.setRange(0, coreRows);
      xAlgorithm.execute(convolveX);
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = dst.copyFromBuffer(yBegin * dimX, nonstd::span<const T>(output.data(), output.size())); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  std::vector<T> source;
  std::vector<T> intermediate;
  std::vector<T> output;
  for(usize y = 0; y < dimY; ++y)
  {
    const usize inputYBegin = y > radiusY ? y - radiusY : 0;
    const usize inputYEnd = std::min(dimY, y + radiusY + 1);
    const usize inputRows = inputYEnd - inputYBegin;
    for(usize xBegin = 0; xBegin < dimX; xBegin += plan.coreCols)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize xEnd = std::min(dimX, xBegin + plan.coreCols);
      const usize inputXBegin = xBegin > radiusX ? xBegin - radiusX : 0;
      const usize inputXEnd = std::min(dimX, xEnd + radiusX);
      const usize inputCols = inputXEnd - inputXBegin;
      const usize coreCols = xEnd - xBegin;
      source.resize(inputRows * inputCols);
      intermediate.resize(inputCols);
      output.resize(coreCols);
      for(usize inputY = inputYBegin; inputY < inputYEnd; ++inputY)
      {
        if(Result<> result = src.copyIntoBuffer(inputY * dimX + inputXBegin, nonstd::span<T>(source.data() + (inputY - inputYBegin) * inputCols, inputCols)); result.invalid())
        {
          return result;
        }
      }

      auto convolveY = [&](const Range& columnRange) {
        for(usize localX = columnRange.min(); localX < columnRange.max(); ++localX)
        {
          double accumulator = 0.0;
          for(int64 kernelIndex = 0; kernelIndex < static_cast<int64>(yKernel.size()); ++kernelIndex)
          {
            const usize sourceY = static_cast<usize>(clampIndex(static_cast<int64>(y) + kernelIndex - radiusY64, dimY64 - 1));
            accumulator += static_cast<double>(source[(sourceY - inputYBegin) * inputCols + localX]) * yKernel[static_cast<usize>(kernelIndex)];
          }
          intermediate[localX] = static_cast<T>(accumulator);
        }
      };
      ParallelDataAlgorithm yAlgorithm;
      yAlgorithm.setRange(0, inputCols);
      yAlgorithm.execute(convolveY);

      auto convolveX = [&](const Range& columnRange) {
        for(usize localOutputX = columnRange.min(); localOutputX < columnRange.max(); ++localOutputX)
        {
          double accumulator = 0.0;
          const int64 globalX = static_cast<int64>(xBegin + localOutputX);
          for(int64 kernelIndex = 0; kernelIndex < static_cast<int64>(xKernel.size()); ++kernelIndex)
          {
            const usize sourceX = static_cast<usize>(clampIndex(globalX + kernelIndex - radiusX64, dimX64 - 1));
            accumulator += static_cast<double>(intermediate[sourceX - inputXBegin]) * xKernel[static_cast<usize>(kernelIndex)];
          }
          output[localOutputX] = static_cast<T>(accumulator);
        }
      };
      ParallelDataAlgorithm xAlgorithm;
      xAlgorithm.setRange(0, coreCols);
      xAlgorithm.execute(convolveX);
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = dst.copyFromBuffer(y * dimX + xBegin, nonstd::span<const T>(output.data(), output.size())); result.invalid())
      {
        return result;
      }
    }
  }
  return {};
}
} // namespace detail

// One separable discrete-Gaussian convolution pass along `axis`, reading `src` (type T), accumulating in double,
// storing static_cast<T> (matching ITK's Image<OutputPixelType> intermediates + static_cast on store), ZeroFluxNeumann
// (edge-clamp). `src` may alias `dst` (each line is fully gathered before write). `kernel` is the symmetric double
// kernel (odd length, radius = size/2).
template <class T, class SrcStoreT, class DstStoreT>
Result<> DiscreteGaussianAxisPass(const SrcStoreT& src, DstStoreT& dst, const SizeVec3& dims, uint32 axis, const std::vector<double>& kernel, const std::atomic_bool& shouldCancel,
                                  const IFilter::MessageHandler& /*messageHandler*/, const DiscreteGaussianPassOptions& options = {})
{
  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  const usize slice = static_cast<usize>(nX) * static_cast<usize>(nY);
  const int64 r = static_cast<int64>(kernel.size() / 2);
  const int64 ln = (axis == 0) ? nX : ((axis == 1) ? nY : nZ);
  auto clampi = [](int64 v, int64 hi) { return v < 0 ? int64{0} : (v > hi ? hi : v); };

  // Convolve one gathered line (length ln) of doubles `in` into `out` (length ln, static_cast<T>).
  auto convLine = [&](const std::vector<double>& in, std::vector<T>& out) {
    for(int64 n = 0; n < ln; ++n)
    {
      double acc = 0.0;
      for(int64 k = 0; k < static_cast<int64>(kernel.size()); ++k)
      {
        acc += in[static_cast<usize>(clampi(n + (k - r), ln - 1))] * kernel[static_cast<usize>(k)];
      }
      out[static_cast<usize>(n)] = static_cast<T>(acc);
    }
  };

  // NOTE: the per-line scratch (`lineIn`/`lineOut`) is NOT allocated here. Each axis pass stages one plane/slab into a
  // LOCAL buffer (serial, one resident at a time -> bounded memory) and then convolves its independent 1-D lines with a
  // ParallelDataAlgorithm; every worker allocates its OWN thread-local scratch so there is no shared mutable state. The
  // parallel region touches only the staged local buffers -- never the store -- so it is byte-exact and safe for BOTH
  // in-core and OOC with no gate (each output line depends only on its own input line + the read-only kernel, so
  // reordering lines changes nothing). `convLine` above reads only read-only captures (kernel/ln/r/clampi) and its
  // in/out parameters, so it is pure and may be called concurrently with per-worker buffers.

  if(axis == 0)
  {
    // X pass: stage a whole z-plane, convolve each contiguous nX-row in parallel, write the plane back. Reading the full
    // plane before writing it keeps in-place aliasing (src may alias dst) safe, and bounds memory at O(slice) like Y.
    std::vector<T> srcPlane(slice);
    std::vector<T> dstPlane(slice);
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize planeOff = static_cast<usize>(z) * slice;
      if(Result<> rr = src.copyIntoBuffer(planeOff, nonstd::span<T>(srcPlane.data(), slice)); rr.invalid())
      {
        return rr;
      }
      // Parallel over rows [0, nY): each worker convolves a disjoint set of contiguous nX-rows with its OWN thread-local
      // scratch, gathering from srcPlane and scattering into dstPlane at disjoint indices.
      auto convRows = [&](const Range& rowRange) {
        std::vector<double> lineIn(static_cast<usize>(nX));
        std::vector<T> lineOut(static_cast<usize>(nX));
        for(usize y = rowRange.min(); y < rowRange.max(); ++y)
        {
          const usize rowOff = y * static_cast<usize>(nX);
          for(int64 i = 0; i < nX; ++i)
          {
            lineIn[static_cast<usize>(i)] = static_cast<double>(srcPlane[rowOff + static_cast<usize>(i)]);
          }
          convLine(lineIn, lineOut);
          for(int64 i = 0; i < nX; ++i)
          {
            dstPlane[rowOff + static_cast<usize>(i)] = lineOut[static_cast<usize>(i)];
          }
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, static_cast<usize>(nY));
      parallelAlgorithm.execute(convRows);
      if(Result<> rr = dst.copyFromBuffer(planeOff, nonstd::span<const T>(dstPlane.data(), slice)); rr.invalid())
      {
        return rr;
      }
    }
    return {};
  }
  if(axis == 1)
  {
    // Y pass: read a whole z-plane, convolve each column (stride nX, count nY) in parallel, write the plane back.
    std::vector<T> srcPlane(slice);
    std::vector<T> dstPlane(slice);
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize planeOff = static_cast<usize>(z) * slice;
      if(Result<> rr = src.copyIntoBuffer(planeOff, nonstd::span<T>(srcPlane.data(), slice)); rr.invalid())
      {
        return rr;
      }
      // Parallel over columns [0, nX): each worker convolves a disjoint set of columns (stride nX, count nY) with its OWN
      // thread-local scratch, gathering from srcPlane and scattering into dstPlane at disjoint indices.
      auto convColumns = [&](const Range& colRange) {
        std::vector<double> lineIn(static_cast<usize>(nY));
        std::vector<T> lineOut(static_cast<usize>(nY));
        for(usize x = colRange.min(); x < colRange.max(); ++x)
        {
          for(int64 j = 0; j < nY; ++j)
          {
            lineIn[static_cast<usize>(j)] = static_cast<double>(srcPlane[static_cast<usize>(j) * static_cast<usize>(nX) + x]);
          }
          convLine(lineIn, lineOut);
          for(int64 j = 0; j < nY; ++j)
          {
            dstPlane[static_cast<usize>(j) * static_cast<usize>(nX) + x] = lineOut[static_cast<usize>(j)];
          }
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, static_cast<usize>(nX));
      parallelAlgorithm.execute(convColumns);
      if(Result<> rr = dst.copyFromBuffer(planeOff, nonstd::span<const T>(dstPlane.data(), slice)); rr.invalid())
      {
        return rr;
      }
    }
    return {};
  }
  // axis == 2 (Z): stage consecutive Y rows across every Z plane. This converts many small per-row HDF5 operations
  // into one contiguous read and write per Z plane while retaining a fixed memory budget. The one-row minimum is an
  // indivisible XZ staging unit and may exceed the requested budget for exceptionally wide/tall data.
  const usize stagingByteBudget = std::min(options.stagingByteBudget, DiscreteGaussianPassOptions::k_MaxStagingBytes);
  const usize maxStagedValues = std::max<usize>(1, stagingByteBudget / (2 * sizeof(T)));
  const usize maxYRows = (dims[0] == 0 || dims[2] == 0) ? 1 : std::max<usize>(1, std::min<usize>(dims[1], maxStagedValues / dims[0] / dims[2]));
  std::vector<T> srcSlab;
  std::vector<T> dstSlab;
  for(usize yBegin = 0; yBegin < dims[1]; yBegin += maxYRows)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize yCount = std::min(maxYRows, dims[1] - yBegin);
    const usize valuesPerPlaneBlock = yCount * dims[0];
    const usize batchValues = valuesPerPlaneBlock * dims[2];
    srcSlab.resize(batchValues);
    dstSlab.resize(batchValues);
    for(usize z = 0; z < dims[2]; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize blockOffset = (z * dims[1] + yBegin) * dims[0];
      if(Result<> rr = src.copyIntoBuffer(blockOffset, nonstd::span<T>(srcSlab.data() + z * valuesPerPlaneBlock, valuesPerPlaneBlock)); rr.invalid())
      {
        return rr;
      }
    }
    // Parallel over the staged X/Y lines. Workers gather and scatter only disjoint local-buffer lines; all store
    // access remains outside the parallel region.
    const usize lineCount = valuesPerPlaneBlock;
    auto convZLines = [&](const Range& lineRange) {
      std::vector<double> lineIn(static_cast<usize>(nZ));
      std::vector<T> lineOut(static_cast<usize>(nZ));
      for(usize line = lineRange.min(); line < lineRange.max(); ++line)
      {
        if(shouldCancel)
        {
          return;
        }
        for(usize z = 0; z < dims[2]; ++z)
        {
          lineIn[z] = static_cast<double>(srcSlab[z * valuesPerPlaneBlock + line]);
        }
        convLine(lineIn, lineOut);
        for(usize z = 0; z < dims[2]; ++z)
        {
          dstSlab[z * valuesPerPlaneBlock + line] = lineOut[z];
        }
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, lineCount);
    parallelAlgorithm.execute(convZLines);
    if(shouldCancel)
    {
      return {};
    }
    for(usize z = 0; z < dims[2]; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize blockOffset = (z * dims[1] + yBegin) * dims[0];
      if(Result<> rr = dst.copyFromBuffer(blockOffset, nonstd::span<const T>(dstSlab.data() + z * valuesPerPlaneBlock, valuesPerPlaneBlock)); rr.invalid())
      {
        return rr;
      }
    }
  }
  return {};
}

// Full separable discrete-Gaussian convolution in DESCENDING axis order [effDim-1, ..., 0], matching ITK's reversed
// direction. Resident endpoints use the direct store-backed cascade. If either endpoint is OOC, the 3D Z pass writes
// raw fixed-record scratch and the plane-local Y/X passes are fused; 2D fuses Y/X directly without a working store.
// `variance`/`maximumError` are per-axis (3-long). This is the engine entry point the facade and tests call.
template <class T>
Result<> ApplyDiscreteGaussian(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const std::vector<float64>& variance, uint32 maximumKernelWidth,
                               const std::vector<float64>& maximumError, bool useImageSpacing, FloatVec3 spacing, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler,
                               const DiscreteGaussianPassOptions& options = {})
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {};
  }
  auto checkedMultiply = [](usize left, usize right, usize& product) {
    if(left != 0 && right > std::numeric_limits<usize>::max() / left)
    {
      return false;
    }
    product = left * right;
    return true;
  };
  usize planeValues = 0;
  usize volumeValues = 0;
  if(!checkedMultiply(dims[0], dims[1], planeValues) || !checkedMultiply(planeValues, dims[2], volumeValues))
  {
    return MakeErrorResult(-8360, fmt::format("Discrete Gaussian image dimensions overflow the addressable value count. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  if(dims[0] > static_cast<usize>(std::numeric_limits<int64>::max()) || dims[1] > static_cast<usize>(std::numeric_limits<int64>::max()) ||
     dims[2] > static_cast<usize>(std::numeric_limits<int64>::max()))
  {
    return MakeErrorResult(-8361, fmt::format("Discrete Gaussian image dimensions exceed the supported signed index range. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  if(in.getSize() != volumeValues)
  {
    return MakeErrorResult(-8362, fmt::format("Discrete Gaussian input store size ({}) does not match the expected image volume ({}) for dimensions {} x {} x {}.", in.getSize(), volumeValues, dims[0],
                                              dims[1], dims[2]));
  }
  if(out.getSize() != volumeValues)
  {
    return MakeErrorResult(-8363, fmt::format("Discrete Gaussian output store size ({}) does not match the expected image volume ({}) for dimensions {} x {} x {}.", out.getSize(), volumeValues,
                                              dims[0], dims[1], dims[2]));
  }

  const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
  const detail::DiscreteGaussianStoragePlan storagePlan =
      detail::SelectDiscreteGaussianStoragePlan(in.getStoreType(), in.getDataFormat(), out.getStoreType(), out.getDataFormat(), options.forceFusedPlanePass);

  if(storagePlan.useFusedPlanePass)
  {
    double yVariance = variance[1];
    double xVariance = variance[0];
    if(useImageSpacing)
    {
      const double ySpacing = static_cast<double>(spacing[1]);
      const double xSpacing = static_cast<double>(spacing[0]);
      yVariance /= ySpacing * ySpacing;
      xVariance /= xSpacing * xSpacing;
    }
    const std::vector<double> yKernel = detail::GaussianKernelCoefficients(yVariance, maximumError[1], maximumKernelWidth);
    const std::vector<double> xKernel = detail::GaussianKernelCoefficients(xVariance, maximumError[0], maximumKernelWidth);
    if(effDim == 2)
    {
      if(storagePlan.useTemporaryRecordStore || options.forceBounded2D)
      {
        return detail::DiscreteGaussian2DBounded<T>(in, out, dims, yKernel, xKernel, shouldCancel, options.residentLimit2D);
      }
      return detail::DiscreteGaussianYXPlanePass<T>(in, out, dims, yKernel, xKernel, shouldCancel);
    }

    if(shouldCancel)
    {
      return {};
    }
    double zVariance = variance[2];
    if(useImageSpacing)
    {
      const double zSpacing = static_cast<double>(spacing[2]);
      zVariance /= zSpacing * zSpacing;
    }
    const std::vector<double> zKernel = detail::GaussianKernelCoefficients(zVariance, maximumError[2], maximumKernelWidth);
    if(storagePlan.useTemporaryRecordStore)
    {
      auto temporaryResult = detail::CreateGaussianTemporaryStore<T>(volumeValues, planeValues, shouldCancel);
      if(temporaryResult.invalid())
      {
        return ConvertResult(std::move(temporaryResult));
      }
      std::unique_ptr<detail::GaussianTemporaryStore<T>> work = std::move(temporaryResult.value());
      if(Result<> result = DiscreteGaussianAxisPass<T>(in, *work, dims, 2, zKernel, shouldCancel, messageHandler, options); result.invalid())
      {
        return result;
      }
      return detail::DiscreteGaussianYXPlanePass<T>(*work, out, dims, yKernel, xKernel, shouldCancel);
    }

    auto work = DataStoreUtilities::CreateDataStoreWithFormat<T>(storagePlan.workingDataFormat, std::vector<usize>{dims[2], dims[1], dims[0]}, std::vector<usize>{1});
    if(Result<> result = DiscreteGaussianAxisPass<T>(in, *work, dims, 2, zKernel, shouldCancel, messageHandler, options); result.invalid())
    {
      return result;
    }
    return detail::DiscreteGaussianYXPlanePass<T>(*work, out, dims, yKernel, xKernel, shouldCancel);
  }

  auto workPtr = DataStoreUtilities::CreateDataStoreWithFormat<T>(in.getDataFormat(), std::vector<usize>{static_cast<usize>(dims[2]), static_cast<usize>(dims[1]), static_cast<usize>(dims[0])},
                                                                  std::vector<usize>{1});
  AbstractDataStore<T>& work = *workPtr;

  for(uint32 pass = 0; pass < effDim; ++pass)
  {
    if(shouldCancel)
    {
      return {};
    }
    const uint32 axis = effDim - 1 - pass; // descending: highest dimension first
    double var = variance[axis];
    if(useImageSpacing)
    {
      const double s = static_cast<double>(spacing[axis]);
      var = var / (s * s);
    }
    const std::vector<double> kernel = detail::GaussianKernelCoefficients(var, maximumError[axis], maximumKernelWidth);
    const bool firstPass = (pass == 0);
    const bool lastPass = (pass == effDim - 1);
    const AbstractDataStore<T>& src = firstPass ? in : work;
    AbstractDataStore<T>& dst = lastPass ? out : work;
    if(Result<> r = DiscreteGaussianAxisPass<T>(src, dst, dims, axis, kernel, shouldCancel, messageHandler, options); r.invalid())
    {
      return r;
    }
  }
  return {};
}
} // namespace nx::core::ImageProcessing
