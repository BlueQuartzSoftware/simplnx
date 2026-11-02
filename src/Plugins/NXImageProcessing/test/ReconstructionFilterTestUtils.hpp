#pragma once

#include <catch2/catch.hpp>

#include "ItkGoldenTestUtils.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

// Shared test utilities for the three grayscale morphological-reconstruction filters (GrayscaleFillhole,
// HMaxima, HMinima). All three derive a marker image from a single scalar input and reconstruct it under the
// input mask via the shared reconstruction engine, so they share image builders, golden runners, and a
// parameter-setter abstraction (each filter exposes some subset of {FullyConnected, Height}).
namespace recon_test
{
using namespace nx::core;

//------------------------------------------------------------------------------
// simplnx flat index for voxel (x,y,z): X fastest-moving, then Y, then Z.
inline usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return z * (dimY * dimX) + y * dimX + x;
}

/**
 * @brief Deterministic HOLE-BEARING pattern for fillhole-style tests: a uniform bright @p field with several
 *        compact dark (@p hole) blobs in the interior that do NOT touch the image border (genuine holes -> get
 *        filled), PLUS one compact dark region anchored at the (0,0,0) corner that DOES touch the border (not a
 *        hole -> left unchanged). Works for any dims incl. a genuinely 2D image (dimZ == 1), where the interior
 *        blobs are single-plane and the corner region touches the 2D boundary. The blobs are small so the
 *        reconstruction converges quickly even on a large (200^3) volume.
 */
template <class T>
inline std::vector<T> MakeHolePattern(usize dimX, usize dimY, usize dimZ, T field, T hole)
{
  std::vector<T> v(dimX * dimY * dimZ, field);
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);

  // Paint the inclusive, clamped box [x0,x1] x [y0,y1] x [z0,z1] with value.
  auto paintBox = [&](int64 x0, int64 x1, int64 y0, int64 y1, int64 z0, int64 z1, T value) {
    x0 = std::max<int64>(0, x0);
    y0 = std::max<int64>(0, y0);
    z0 = std::max<int64>(0, z0);
    x1 = std::min<int64>(nX - 1, x1);
    y1 = std::min<int64>(nY - 1, y1);
    z1 = std::min<int64>(nZ - 1, z1);
    for(int64 z = z0; z <= z1; ++z)
    {
      for(int64 y = y0; y <= y1; ++y)
      {
        for(int64 x = x0; x <= x1; ++x)
        {
          v[FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dimX, dimY)] = value;
        }
      }
    }
  };

  // Interior dark blobs (half-extent 1 -> 3-voxel cubes), placed at fractional positions and only where they fit
  // STRICTLY inside every active border (>= 1 voxel of bright field on every side).
  const std::array<std::array<double, 3>, 3> centers = {{{0.25, 0.25, 0.5}, {0.5, 0.5, 0.5}, {0.72, 0.60, 0.35}}};
  const int64 rz = (dimZ == 1) ? 0 : 1;
  for(const auto& c : centers)
  {
    const int64 cx = static_cast<int64>(c[0] * static_cast<double>(dimX));
    const int64 cy = static_cast<int64>(c[1] * static_cast<double>(dimY));
    const int64 cz = (dimZ == 1) ? 0 : static_cast<int64>(c[2] * static_cast<double>(dimZ));
    if(cx - 1 < 1 || cx + 1 > nX - 2 || cy - 1 < 1 || cy + 1 > nY - 2)
    {
      continue;
    }
    if(dimZ > 1 && (cz - 1 < 1 || cz + 1 > nZ - 2))
    {
      continue;
    }
    paintBox(cx - 1, cx + 1, cy - 1, cy + 1, cz - rz, cz + rz, hole);
  }

  // One dark region anchored at the (0,0,0) corner so it touches the border (never filled).
  const int64 bx = std::min<int64>(nX, 3) - 1;
  const int64 by = std::min<int64>(nY, 3) - 1;
  const int64 bz = (dimZ == 1) ? 0 : (std::min<int64>(nZ, 3) - 1);
  paintBox(0, bx, 0, by, 0, bz, hole);

  return v;
}

/**
 * @brief Deterministic generic gradient + coarse-block + pseudo-noise pattern (values in [0, 199]) suitable for
 *        the HMaxima / HMinima tests (which suppress shallow maxima/minima rather than fill holes). Non-constant
 *        for every scalar type and fits in [0, 255], so uint8 is safe. Works for any dims incl. a 2D image.
 */
template <class T>
inline std::vector<T> MakeGradientNoisePattern(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const usize gradient = x + 2 * y + 3 * z;
        const usize block = (((x / 2) + (y / 2) + (z / 2)) % 2 == 0) ? 0 : 47;
        const usize noise = (x * 131 + y * 57 + z * 29) % 37;
        v[FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>((gradient + block + noise) % 200);
      }
    }
  }
  return v;
}

/**
 * @brief Deterministic MULTI-PLATEAU pattern for the regional-extrema tests: coarse flat blocks at several distinct
 *        levels plus a short ramp modulation, values in [0, 90]. Guarantees multiple isolated flat maxima/minima,
 *        ramps (chains of non-extrema), and plateaus touching the border, so both connectivities and both ops are
 *        exercised. Works for any dims incl. a 2D image (dimZ == 1).
 */
template <class T>
inline std::vector<T> MakePlateauPattern(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const usize block = ((x / 2) % 3) * 30 + ((y / 2) % 2) * 15; // coarse flat blocks: {0,15,30,45,60,75}
        const usize ramp = (x + y + z) % 7;                          // a short ramp modulation
        v[FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>((block + ramp) % 91);
      }
    }
  }
  return v;
}

/**
 * @brief Builds an ImageGeom "Image Geometry" (dimX x dimY x dimZ) with cell AttributeMatrix "CellData" and a
 *        scalar cell array "Input" of element type T, bulk-filled from @p pattern (so it is efficient even on a
 *        200^3 volume). The store honors the caller's active PreferencesSentinel (in-core vs out-of-core).
 *        Returns the input array path.
 */
template <class T>
inline DataPath BuildImageFromPattern(DataStructure& ds, usize dimX, usize dimY, usize dimZ, const std::vector<T>& pattern)
{
  REQUIRE(pattern.size() == dimX * dimY * dimZ);
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

  constexpr usize k_ChunkValues = 65536;
  for(usize start = 0; start < pattern.size(); start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, pattern.size() - start);
    Result<> writeResult = ref.copyFromBuffer(start, nonstd::span<const T>(pattern.data() + start, count));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
  return inputPath;
}

/**
 * @brief Convenience: build a hole-bearing image (see @ref MakeHolePattern) under the geometry. Returns the
 *        input array path.
 */
template <class T>
inline DataPath BuildHoleImage(DataStructure& ds, usize dimX, usize dimY, usize dimZ, T field, T hole)
{
  return BuildImageFromPattern<T>(ds, dimX, dimY, dimZ, MakeHolePattern<T>(dimX, dimY, dimZ, field, hole));
}

/**
 * @brief Deterministic PEAK-BEARING pattern for grind-peak tests: a uniform dark @p field with several compact
 *        BRIGHT (@p peak) blobs in the interior that do NOT touch the border (get ground down), PLUS one compact
 *        bright region at the (0,0,0) corner that DOES touch the border (preserved). Dual of @ref MakeHolePattern:
 *        that builder places @p peak-valued blobs on a @p field background with no assumption that peak<field, so
 *        passing a low @p field and a high @p peak yields a peak-bearing image with no new placement logic. Works
 *        for any dims incl. a 2D image (dimZ == 1); the blobs are small so reconstruction converges quickly.
 */
template <class T>
inline std::vector<T> MakePeakPattern(usize dimX, usize dimY, usize dimZ, T field, T peak)
{
  return MakeHolePattern<T>(dimX, dimY, dimZ, field, peak);
}

/**
 * @brief Convenience: build a peak-bearing image (see @ref MakePeakPattern) under the geometry. @p field is the
 *        dark background, @p peak the bright blobs. Returns the input array path.
 */
template <class T>
inline DataPath BuildPeakImage(DataStructure& ds, usize dimX, usize dimY, usize dimZ, T field, T peak)
{
  return BuildImageFromPattern<T>(ds, dimX, dimY, dimZ, MakePeakPattern<T>(dimX, dimY, dimZ, field, peak));
}

//------------------------------------------------------------------------------
/**
 * @brief Sets a reconstruction filter's op-specific parameter(s) on a shared Arguments. Every reconstruction
 *        filter exposes some subset of {FullyConnected, Height}: fillhole -> FullyConnected only; HMaxima ->
 *        Height only; HMinima -> both. The injected setter lets one runner drive all three filters.
 */
using ReconParamSetter = std::function<void(Arguments&, bool fullyConnected, float64 height)>;

/// Parameter setter for a filter with a FullyConnected flag only (GrayscaleFillhole). Ignores height.
template <class NewFilterT>
inline ReconParamSetter FullyConnectedParamSetter()
{
  return [](Arguments& args, bool fullyConnected, float64 /*height*/) { args.insertOrAssign(NewFilterT::k_FullyConnected_Key, std::make_any<bool>(fullyConnected)); };
}

/// Parameter setter for a filter with a Height value only (HMaxima). Ignores fullyConnected.
template <class NewFilterT>
inline ReconParamSetter HeightParamSetter()
{
  return [](Arguments& args, bool /*fullyConnected*/, float64 height) { args.insertOrAssign(NewFilterT::k_Height_Key, std::make_any<float64>(height)); };
}

/// Parameter setter for a filter with both a Height value and a FullyConnected flag (HMinima).
template <class NewFilterT>
inline ReconParamSetter HeightAndFullyConnectedParamSetter()
{
  return [](Arguments& args, bool fullyConnected, float64 height) {
    args.insertOrAssign(NewFilterT::k_Height_Key, std::make_any<float64>(height));
    args.insertOrAssign(NewFilterT::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  };
}

/// Parameter setter for the SE-based reconstruction filters (Opening/ClosingByReconstruction): captures the kernel
/// type + radius + preserveIntensities, and takes FullyConnected from the grid config (height is unused). The
/// stored any types match exactly what the filter reads (ChoicesParameter::ValueType, std::vector<uint32>, bool).
template <class NewFilterT>
inline ReconParamSetter SEReconParamSetter(ChoicesParameter::ValueType kernelType, std::vector<uint32> kernelRadius, bool preserveIntensities)
{
  return [kernelType, kernelRadius, preserveIntensities](Arguments& args, bool fullyConnected, float64 /*height*/) {
    args.insertOrAssign(NewFilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(kernelType));
    args.insertOrAssign(NewFilterT::k_KernelRadius_Key, std::make_any<std::vector<uint32>>(kernelRadius));
    args.insertOrAssign(NewFilterT::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
    args.insertOrAssign(NewFilterT::k_PreserveIntensities_Key, std::make_any<bool>(preserveIntensities));
  };
}

/// Parameter setter for the binary reconstruction filters (BinaryOpeningByReconstruction): captures the kernel type
/// + radius + foreground + background, and takes FullyConnected from the grid config (height unused).
template <class NewFilterT>
inline ReconParamSetter BinaryReconParamSetter(ChoicesParameter::ValueType kernelType, std::vector<uint32> kernelRadius, float64 foreground, float64 background)
{
  return [kernelType, kernelRadius, foreground, background](Arguments& args, bool fullyConnected, float64 /*height*/) {
    args.insertOrAssign(NewFilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(kernelType));
    args.insertOrAssign(NewFilterT::k_KernelRadius_Key, std::make_any<std::vector<uint32>>(kernelRadius));
    args.insertOrAssign(NewFilterT::k_ForegroundValue_Key, std::make_any<float64>(foreground));
    args.insertOrAssign(NewFilterT::k_BackgroundValue_Key, std::make_any<float64>(background));
    args.insertOrAssign(NewFilterT::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  };
}

/**
 * @brief Sets the standard geom/input/output keys + the injected op-specific params on a shared Arguments, runs
 *        preflight + execute, and requires that both succeed. It reads the key constants from NewFilterT.
 *        @p outputName is the created output array's name.
 */
template <class NewFilterT>
inline void RunReconstructionFilter(IFilter& filter, DataStructure& ds, const DataPath& inputPath, const ReconParamSetter& setParams, bool fullyConnected, float64 height,
                                    const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  setParams(args, fullyConnected, height);
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

/**
 * @brief Exact element-wise float32 comparison (NO tolerance, unlike UnitTest::CompareDataArrays which accepts a 1e-4
 *        absolute slack on any mismatch). Matched NaNs are treated as equal. Reports the first mismatch. The D3
 *        (OOC-vs-in-core) byte-match gates use it, because the tolerant comparator can hide a real ULP-level
 *        divergence.
 */
inline void RequireExactFloat32(const IDataArray& left, const IDataArray& right)
{
  const auto& a = left.getIDataStoreRefAs<AbstractDataStore<float32>>();
  const auto& b = right.getIDataStoreRefAs<AbstractDataStore<float32>>();
  REQUIRE(a.getSize() == b.getSize());
  const usize total = a.getSize();
  constexpr usize k_ChunkSize = 40000;
  auto bufA = std::make_unique<float32[]>(k_ChunkSize);
  auto bufB = std::make_unique<float32[]>(k_ChunkSize);
  bool failed = false;
  usize failIndex = 0;
  float32 failA = 0.0f;
  float32 failB = 0.0f;
  for(usize offset = 0; offset < total && !failed; offset += k_ChunkSize)
  {
    const usize count = std::min(k_ChunkSize, total - offset);
    Result<> readAResult = a.copyIntoBuffer(offset, nonstd::span<float32>(bufA.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(readAResult);
    Result<> readBResult = b.copyIntoBuffer(offset, nonstd::span<float32>(bufB.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(readBResult);
    for(usize i = 0; i < count; ++i)
    {
      if(bufA[i] == bufB[i])
      {
        continue;
      }
      if(std::isnan(bufA[i]) && std::isnan(bufB[i]))
      {
        continue;
      }
      failed = true;
      failIndex = offset + i;
      failA = bufA[i];
      failB = bufB[i];
      break;
    }
  }
  if(failed)
  {
    UNSCOPED_INFO(fmt::format("EXACT float32 mismatch @{}: {} != {}", failIndex, failA, failB));
  }
  REQUIRE(!failed);
}

/**
 * @brief Exact element-wise comparison for ANY floating-point element type T (float32 OR float64) -- the same NO-
 *        tolerance contract as @ref RequireExactFloat32, generalized so a SameAsInput floating-point filter (e.g.
 *        MinMaxCurvatureFlow, which accepts both float32 and float64) can gate BOTH widths bit-exactly with one
 *        helper. @ref RequireExactFloat32 stays as-is (its float32-only call sites are unaffected).
 */
template <class T>
inline void RequireExact(const IDataArray& left, const IDataArray& right)
{
  static_assert(std::is_floating_point_v<T>, "RequireExact<T> is for floating-point element types only.");
  const auto& a = left.getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto& b = right.getIDataStoreRefAs<AbstractDataStore<T>>();
  REQUIRE(a.getSize() == b.getSize());
  const usize total = a.getSize();
  constexpr usize k_ChunkSize = 40000;
  auto bufA = std::make_unique<T[]>(k_ChunkSize);
  auto bufB = std::make_unique<T[]>(k_ChunkSize);
  bool failed = false;
  usize failIndex = 0;
  T failA = T{0};
  T failB = T{0};
  for(usize offset = 0; offset < total && !failed; offset += k_ChunkSize)
  {
    const usize count = std::min(k_ChunkSize, total - offset);
    Result<> readAResult = a.copyIntoBuffer(offset, nonstd::span<T>(bufA.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(readAResult);
    Result<> readBResult = b.copyIntoBuffer(offset, nonstd::span<T>(bufB.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(readBResult);
    for(usize i = 0; i < count; ++i)
    {
      if(bufA[i] == bufB[i])
      {
        continue;
      }
      if(std::isnan(bufA[i]) && std::isnan(bufB[i]))
      {
        continue;
      }
      failed = true;
      failIndex = offset + i;
      failA = bufA[i];
      failB = bufB[i];
      break;
    }
  }
  if(failed)
  {
    UNSCOPED_INFO(fmt::format("EXACT mismatch @{}: {} != {}", failIndex, failA, failB));
  }
  REQUIRE(!failed);
}

//------------------------------------------------------------------------------
// Real-image golden helpers. These mirror the object-morphology / grayscale-morphology golden helpers: they read the
// real input image through OUR reader, run OUR filter with the case's parameters, then assert the DURABLE golden --
// an embedded md5 of OUR output (md5-validity-first, plan Sec.4), or a stored baseline compared via CompareImages at
// the case's tolerance. Each case is pinned ForceInCore.

using ExtraParamSetter = std::function<void(Arguments&)>;
inline ExtraParamSetter NoExtraParams()
{
  return [](Arguments&) {};
}

//------------------------------------------------------------------------------
// Task 9 (reconstruction + regional extrema): md5-golden. Runs OUR filter, then asserts (A) md5-validity-first
// against the committed hash. @p setExtraParams sets the case's op-specific params (Height / kernel / foreground /
// etc.); it defaults to none for the parameter-free filters.
template <class FilterT>
inline void RunReconstructionMd5ItkGolden(const std::string& inputFile, const std::string& committedMd5, const ExtraParamSetter& setExtraParams = NoExtraParams())
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // --- read input through OUR reader ---
  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  // --- run OUR filter with the case's parameters ---
  Arguments args;
  args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setExtraParams(args);
  FilterT filter;
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (A) DURABLE golden: md5-validity-first (plan Sec.4) -- our output's md5 must equal the committed hash. ---
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("{} md5: ours='{}' committed='{}'", inputFile, ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  UnitTest::CheckArraysInheritTupleDims(ds);
}

//------------------------------------------------------------------------------
// Task 10 (distance transforms): baseline-golden. Runs OUR filter, then asserts (A) a stored baseline .nrrd compared
// via CompareImages @ @p tolerance. All five distance filters emit a fixed float32 output on an integer
// (2th_cthead1.png) input.
template <class FilterT>
inline void RunDistanceMapItkGoldenBaseline(const std::string& inputFile, const std::string& baselineFile, float64 tolerance, const ExtraParamSetter& setExtraParams = NoExtraParams(),
                                            UnitTest::AlgorithmTestScope* scope = nullptr)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // --- read input through OUR reader ---
  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);
  if(scope != nullptr)
  {
    scope->requireExpectedStore(ds.getDataRefAs<IDataArray>(input));
  }

  // --- run OUR filter with the case's parameters ---
  Arguments args;
  args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setExtraParams(args);
  FilterT filter;
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = scope == nullptr ? filter.execute(ds, args) : scope->executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (A) DURABLE baseline golden: stored baseline compared at the case's tolerance. ---
  const DataPath baselineGeom({"Baseline Geometry"});
  const DataPath baselineData = baselineGeom.createChildPath("CellData").createChildPath("Baseline");
  const Result<> readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), baselineGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const Result<> compareResult = ip_golden::CompareImages(ds, baselineGeom, baselineData, geom, output, tolerance);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace recon_test
