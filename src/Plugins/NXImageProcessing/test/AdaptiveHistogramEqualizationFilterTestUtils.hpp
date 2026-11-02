#pragma once

#include <catch2/catch.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/AdaptiveHistogramEqualizationEngine.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace ahe_test
{
using namespace nx::core;

//------------------------------------------------------------------------------
// simplnx flat index for voxel (x,y,z): X fastest-moving, then Y, then Z.
inline usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return z * (dimY * dimX) + y * dimX + x;
}

/**
 * @brief INDEPENDENT reference oracle for Adaptive Histogram Equalization, computed in the SAME float/double
 *        split as the engine (u/v/cf in float; sum/scale in double) via a plain triple-loop that shares no
 *        code with the engine (its own min/max scan, its own boundary skip). It reproduces the pinned §1.3
 *        arithmetic; used for the engine-vs-oracle grid and the analytic checks. A constant image (max==min)
 *        returns the input unchanged (the engine's documented guard).
 *
 *        The engine takes a difference-keyed LUT fast path for a bounded-range INTEGER T (see
 *        AdaptiveHistogramEqualizationEngine.hpp): there its per-neighbor |2(u-v)| is formed from the EXACT
 *        integer difference d = Vc - Vn (as d/iscale, double) rather than the twice-float-rounded (u-v). Since
 *        that (u-v) rounding depends on the individual values, not on d alone, no d-keyed table can reproduce
 *        it, so the LUT is a documented tolerant deviation. This oracle reproduces that SAME pinned deviation
 *        INDEPENDENTLY (its own loop, indexing, and gate) so the engine-vs-oracle check stays near-exact on the
 *        LUT path while still catching real boundary/indexing bugs. Float T and out-of-LUT-range integer spans
 *        keep the twice-float-rounded (u-v) path in BOTH engine and oracle.
 */
template <class T>
inline std::vector<T> AdaptiveHistogramEqualizationOracle(const std::vector<T>& in, usize dimX, usize dimY, usize dimZ, const std::array<int32, 3>& radius, float32 alpha, float32 beta)
{
  const int64 dimXi = static_cast<int64>(dimX);
  const int64 dimYi = static_cast<int64>(dimY);
  const int64 dimZi = static_cast<int64>(dimZ);

  float64 minValue = static_cast<float64>(in[0]);
  float64 maxValue = static_cast<float64>(in[0]);
  for(const T value : in)
  {
    const float64 dv = static_cast<float64>(value);
    minValue = std::min(minValue, dv);
    maxValue = std::max(maxValue, dv);
  }
  const float64 iscale = maxValue - minValue;
  if(iscale == 0.0)
  {
    return in; // constant-image guard (matches the engine)
  }

  // Gate mirrors the engine's LUT gate: integral T AND the integer gray span within the table-size cap. For the
  // integer types exercised here (uint8/int16/int32) iscale equals the exact integer span, so this float64
  // comparison decides the path identically to the engine's integer arithmetic.
  constexpr usize kMaxAheLutEntries = usize{1} << 22;                             // must match the engine's cap
  constexpr float64 kMaxSpan = static_cast<float64>((kMaxAheLutEntries - 1) / 2); // 2*span + 1 <= kMaxAheLutEntries
  const bool useLut = std::is_integral_v<T> && iscale <= kMaxSpan;

  std::vector<T> out(in.size());
  for(int64 zi = 0; zi < dimZi; ++zi)
  {
    for(int64 yi = 0; yi < dimYi; ++yi)
    {
      for(int64 xi = 0; xi < dimXi; ++xi)
      {
        const usize cIdx = FlatIndex(static_cast<usize>(xi), static_cast<usize>(yi), static_cast<usize>(zi), dimX, dimY);
        const float u = static_cast<float>((static_cast<float64>(in[cIdx]) - minValue) / iscale - 0.5);
        float64 localSum = 0.0;
        usize inBounds = 0;
        for(int64 dz = -radius[2]; dz <= radius[2]; ++dz)
        {
          const int64 nz = zi + dz;
          if(nz < 0 || nz >= dimZi)
          {
            continue;
          }
          for(int64 dy = -radius[1]; dy <= radius[1]; ++dy)
          {
            const int64 ny = yi + dy;
            if(ny < 0 || ny >= dimYi)
            {
              continue;
            }
            for(int64 dx = -radius[0]; dx <= radius[0]; ++dx)
            {
              const int64 nx = xi + dx;
              if(nx < 0 || nx >= dimXi)
              {
                continue;
              }
              const usize nIdx = FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY);
              // Normalized-difference source for sgn/|2(u-v)|: the exact integer difference on the LUT path
              // (mirrors the engine's difference-keyed table), else the twice-float-rounded (u - v).
              int s = 0;
              float ad = 0.0f;
              if(useLut)
              {
                const int64 d = static_cast<int64>(in[cIdx]) - static_cast<int64>(in[nIdx]); // exact integer diff
                const double t = static_cast<double>(d) / iscale;
                const float tf = static_cast<float>(t);
                s = (tf != 0.0f) ? ((tf > 0.0f) ? 1 : -1) : 0;
                ad = static_cast<float>(std::fabs(2.0 * t));
              }
              else
              {
                const float v = static_cast<float>((static_cast<float64>(in[nIdx]) - minValue) / iscale - 0.5);
                s = (u - v != 0.0f) ? ((u - v > 0.0f) ? 1 : -1) : 0;
                ad = static_cast<float>(std::fabs(2.0 * static_cast<double>(u - v)));
              }
              const float cf = 0.5f * static_cast<float>(s) * std::pow(ad, alpha) - beta * 0.5f * static_cast<float>(s) * ad + beta * u;
              localSum += static_cast<float64>(cf);
              ++inBounds;
            }
          }
        }
        const float64 result = iscale * (localSum / static_cast<float64>(inBounds) + 0.5) + minValue;
        out[FlatIndex(static_cast<usize>(xi), static_cast<usize>(yi), static_cast<usize>(zi), dimX, dimY)] = static_cast<T>(result);
      }
    }
  }
  return out;
}

/**
 * @brief Tolerant comparison of an AHE result against an expected result. Bit-for-bit parity is impossible
 *        (ITK sums over an unordered_map in hash order), so: for an INTEGER T every voxel must be within
 *        |a-b| <= 1 (truncation-boundary flips) AND the FRACTION of differing voxels must be <= @p
 *        maxDiffFraction (a systematic error moves far more than that); for a FLOAT T every voxel must satisfy
 *        |a-b| <= atol + rtol*|b|. Records the first violation for diagnostics.
 */
template <class T>
inline void RequireAheClose(const std::vector<T>& computed, const std::vector<T>& expected, double atol, double rtol, double maxDiffFraction)
{
  REQUIRE(computed.size() == expected.size());
  bool hardFail = false;
  usize firstBad = 0;
  double gotV = 0.0;
  double expV = 0.0;
  usize nDiff = 0;
  for(usize i = 0; i < expected.size(); ++i)
  {
    if constexpr(std::is_integral_v<T>)
    {
      const int64 diff = std::llabs(static_cast<int64>(computed[i]) - static_cast<int64>(expected[i]));
      if(diff != 0)
      {
        ++nDiff;
      }
      if(diff > 1 && !hardFail)
      {
        hardFail = true;
        firstBad = i;
        gotV = static_cast<double>(computed[i]);
        expV = static_cast<double>(expected[i]);
      }
    }
    else
    {
      const double a = static_cast<double>(computed[i]);
      const double b = static_cast<double>(expected[i]);
      if(std::fabs(a - b) > atol + rtol * std::fabs(b) && !hardFail)
      {
        hardFail = true;
        firstBad = i;
        gotV = a;
        expV = b;
      }
    }
  }
  if(hardFail)
  {
    UNSCOPED_INFO("AHE out-of-tolerance at index=" << firstBad << " got=" << gotV << " expected=" << expV);
  }
  REQUIRE_FALSE(hardFail);
  if constexpr(std::is_integral_v<T>)
  {
    const double frac = expected.empty() ? 0.0 : static_cast<double>(nDiff) / static_cast<double>(expected.size());
    UNSCOPED_INFO("AHE integer differing-voxel fraction=" << frac << " (allowed <= " << maxDiffFraction << ")");
    REQUIRE(frac <= maxDiffFraction);
  }
}

/**
 * @brief Deterministic NON-CONSTANT pattern in [0, 100] (so iscale > 0 for every type, and there are ~100
 *        distinct values to exercise the cumulative-function accumulation). Float types get a fractional
 *        offset. Fits every scalar type's range (int8's [-128,127] included).
 */
template <class T>
inline std::vector<T> MakeAhePattern(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const usize h = (x * 3 + y * 5 + z * 7 + ((x + 1) * (y + 1) * (z + 1)) % 13) % 101; // 0..100, non-constant
        if constexpr(std::is_floating_point_v<T>)
        {
          v[FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>(h) + static_cast<T>(0.25);
        }
        else
        {
          v[FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>(h);
        }
      }
    }
  }
  return v;
}

/**
 * @brief Builds an ImageGeom "Image Geometry" (dimX x dimY x dimZ) with cell AttributeMatrix "CellData" and a
 *        scalar cell array "Input" of type T, filled with @ref MakeAhePattern. Returns the input path.
 */
template <class T>
inline DataPath BuildAheImage(DataStructure& ds, usize dimX, usize dimY, usize dimZ)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<T>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<T>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();

  const std::vector<T> pattern = MakeAhePattern<T>(dimX, dimY, dimZ);
  for(usize i = 0; i < pattern.size(); ++i)
  {
    ref.setValue(i, pattern[i]);
  }
  return inputPath;
}

/**
 * @brief Runs the public @ref nx::core::ImageProcessing::ApplyAdaptiveHistogramEqualization entry on an
 *        in-memory DataStore and returns the output as a flat vector.
 */
template <class T>
inline std::vector<T> RunApplyAhe(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const std::array<usize, 3>& radius, float32 alpha, float32 beta)
{
  DataStore<T> inStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < input.size(); ++i)
  {
    inStore.setValue(i, input[i]);
  }
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = ImageProcessing::ApplyAdaptiveHistogramEqualization<T>(inStore, outStore, SizeVec3{dimX, dimY, dimZ}, radius, alpha, beta, shouldCancel, messageHandler);
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

/**
 * @brief Sets the standard geom/input/output keys + Radius/Alpha/Beta on a shared Arguments, runs preflight +
 *        execute, and requires that both succeed. The function reads the key constants from NewFilterT.
 */
template <class NewFilterT>
inline void RunAheFilter(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float32 alpha, float32 beta, const std::vector<uint32>& radius)
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(NewFilterT::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
  args.insertOrAssign(NewFilterT::k_Alpha_Key, std::make_any<float32>(alpha));
  args.insertOrAssign(NewFilterT::k_Beta_Key, std::make_any<float32>(beta));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace ahe_test
