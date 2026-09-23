#include "simplnx/Utilities/ImageProcessing/DanielssonDistanceMapEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return (z * dimY + y) * dimX + x;
}

template <class T>
class ReadCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  [[nodiscard]] usize readCount() const noexcept
  {
    return m_ReadCount;
  }

private:
  mutable usize m_ReadCount = 0;
};

template <class T>
class TransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_ReadValues += buffer.size();
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_WrittenValues += buffer.size();
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  [[nodiscard]] usize readValues() const noexcept
  {
    return m_ReadValues;
  }

  [[nodiscard]] usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

private:
  mutable usize m_ReadValues = 0;
  usize m_WrittenValues = 0;
};

class VectorScratchCountingDataStore : public DataStore<int32>
{
public:
  using DataStore<int32>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<int32> buffer) const override
  {
    m_Reads.emplace_back(startIndex, buffer.size());
    return DataStore<int32>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const int32> buffer) override
  {
    m_Writes.emplace_back(startIndex, buffer.size());
    return DataStore<int32>::copyFromBuffer(startIndex, buffer);
  }

  [[nodiscard]] const std::vector<std::pair<usize, usize>>& reads() const noexcept
  {
    return m_Reads;
  }

  [[nodiscard]] const std::vector<std::pair<usize, usize>>& writes() const noexcept
  {
    return m_Writes;
  }

private:
  mutable std::vector<std::pair<usize, usize>> m_Reads;
  std::vector<std::pair<usize, usize>> m_Writes;
};

// Exact Euclidean (squared) distance to the nearest NONZERO ("feature") voxel -- Danielsson's convention: nonzero
// voxels are the seeds, zero voxels are solved. This is the EXACT EDT. Danielsson's 4SED equals it EXACTLY only for
// configurations without diagonal occlusion (a full seed plane, a half-space); for sparse seeds the 4SED (and ITK
// itself) over-estimates, so those are checked only with the `mine >= exact` lower-bound invariant. A z spacing of 1
// or a 2D image (dz==1) collapses the z term. Matches the engine's float math (double sum, float32 cast).
std::vector<float32> ExactEdtOracle(const std::vector<int32>& in, usize dx, usize dy, usize dz, bool squared, bool useSpacing, FloatVec3 spacing)
{
  const int64 nX = static_cast<int64>(dx), nY = static_cast<int64>(dy), nZ = static_cast<int64>(dz);
  const float64 sp[3] = {static_cast<float64>(spacing[0]), static_cast<float64>(spacing[1]), static_cast<float64>(spacing[2])};
  auto at = [&](int64 x, int64 y, int64 z) { return in[FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dx, dy)]; };

  std::vector<std::array<int64, 3>> feats;
  for(int64 z = 0; z < nZ; ++z)
  {
    for(int64 y = 0; y < nY; ++y)
    {
      for(int64 x = 0; x < nX; ++x)
      {
        if(at(x, y, z) != 0)
        {
          feats.push_back({x, y, z});
        }
      }
    }
  }

  std::vector<float32> out(dx * dy * dz);
  for(int64 z = 0; z < nZ; ++z)
  {
    for(int64 y = 0; y < nY; ++y)
    {
      for(int64 x = 0; x < nX; ++x)
      {
        float64 best = std::numeric_limits<float64>::max();
        for(const auto& f : feats)
        {
          const float64 ddx = static_cast<float64>(x - f[0]);
          const float64 ddy = static_cast<float64>(y - f[1]);
          const float64 ddz = static_cast<float64>(z - f[2]);
          const float64 d = useSpacing ? ((ddx * sp[0]) * (ddx * sp[0]) + (ddy * sp[1]) * (ddy * sp[1]) + (ddz * sp[2]) * (ddz * sp[2])) : (ddx * ddx + ddy * ddy + ddz * ddz);
          best = std::min(best, d);
        }
        const float32 v = squared ? static_cast<float32>(best) : static_cast<float32>(std::sqrt(best));
        out[FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dx, dy)] = v;
      }
    }
  }
  return out;
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: hybrid vector-plane router bypasses scratch for resident prefix", "[ImageProcessing][DanielssonDistanceMapEngine][WorkingMemory]")
{
  constexpr usize k_PlaneValues = 6;
  constexpr usize k_VectorValues = k_PlaneValues * 3;
  constexpr usize k_PrefixPlanes = 2;
  VectorScratchCountingDataStore scratch(ShapeType{4, 1, k_PlaneValues}, ShapeType{3}, 0);
  std::vector<int32> prefix(k_PrefixPlanes * k_VectorValues);
  std::atomic_bool shouldCancel{false};
  ImageProcessing::detail::DanielssonHybridVectorPlaneStore router(scratch, prefix, k_PrefixPlanes, k_VectorValues, shouldCancel);

  const std::vector<int32> prefixValues(k_VectorValues, 17);
  const std::vector<int32> secondPrefixValues(k_VectorValues, 23);
  const std::vector<int32> suffixValues(k_VectorValues, 29);
  auto writePlaneResult = router.writePlane(0, prefixValues);
  SIMPLNX_RESULT_REQUIRE_VALID(writePlaneResult);
  auto writePlaneResult2 = router.writePlane(1, secondPrefixValues);
  SIMPLNX_RESULT_REQUIRE_VALID(writePlaneResult2);
  auto writePlaneResult3 = router.writePlane(2, suffixValues);
  SIMPLNX_RESULT_REQUIRE_VALID(writePlaneResult3);

  std::vector<int32> readback(k_VectorValues);
  auto readPlaneResult = router.readPlane(0, readback);
  SIMPLNX_RESULT_REQUIRE_VALID(readPlaneResult);
  REQUIRE(readback == prefixValues);
  auto readPlaneResult2 = router.readPlane(2, readback);
  SIMPLNX_RESULT_REQUIRE_VALID(readPlaneResult2);
  REQUIRE(readback == suffixValues);
  auto readPlaneResult3 = router.readPlane(1, readback);
  SIMPLNX_RESULT_REQUIRE_VALID(readPlaneResult3); // Prefix/suffix boundary-neighbor routing.
  REQUIRE(readback == secondPrefixValues);

  REQUIRE(scratch.writes() == std::vector<std::pair<usize, usize>>{{2 * k_VectorValues, k_VectorValues}});
  REQUIRE(scratch.reads() == std::vector<std::pair<usize, usize>>{{2 * k_VectorValues, k_VectorValues}});
  REQUIRE(prefix.front() == 17);
  REQUIRE(prefix[k_VectorValues] == 23);
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: compact vector scratch preserves components and uses the signed-16 boundary", "[ImageProcessing][DanielssonDistanceMapEngine][WorkingMemory]")
{
  constexpr usize k_PlaneValues = 2;
  constexpr usize k_VectorValues = k_PlaneValues * 3;
  DataStore<int16> scratch(ShapeType{3, 1, k_PlaneValues}, ShapeType{3}, int16{0});
  std::vector<int32> prefix;
  std::vector<int16> staging(k_VectorValues);
  std::atomic_bool shouldCancel{false};
  ImageProcessing::detail::DanielssonVectorTransferStats transferStats;
  ImageProcessing::detail::DanielssonCompactHybridVectorPlaneStore router(scratch, prefix, staging, 0, k_VectorValues, shouldCancel, &transferStats);
  const std::vector<int32> values{std::numeric_limits<int16>::lowest(), -17, 0, 23, 1024, std::numeric_limits<int16>::max()};
  auto writePlaneResult4 = router.writePlane(1, values);
  SIMPLNX_RESULT_REQUIRE_VALID(writePlaneResult4);
  std::vector<int32> readback(k_VectorValues);
  auto readPlaneResult4 = router.readPlane(1, readback);
  SIMPLNX_RESULT_REQUIRE_VALID(readPlaneResult4);
  REQUIRE(readback == values);
  REQUIRE(transferStats.scratchWrites == 1);
  REQUIRE(transferStats.scratchReads == 1);
  REQUIRE(transferStats.scratchWriteBytes == k_VectorValues * sizeof(int16));
  REQUIRE(transferStats.scratchReadBytes == k_VectorValues * sizeof(int16));

  std::vector<int32> tooLarge(k_VectorValues);
  tooLarge.back() = std::numeric_limits<int16>::max() + 1;
  auto writePlaneResult5 = router.writePlane(2, tooLarge);
  SIMPLNX_RESULT_REQUIRE_INVALID(writePlaneResult5);
  REQUIRE(ImageProcessing::detail::DanielssonCanUseCompactVectorScratch(SizeVec3{16383, 1, 1}));
  REQUIRE_FALSE(ImageProcessing::detail::DanielssonCanUseCompactVectorScratch(SizeVec3{16384, 1, 1}));
}

template <class T>
std::vector<float32> RunInCore(const std::vector<int32>& pattern, usize dx, usize dy, usize dz, bool squared, bool useSpacing, FloatVec3 spacing)
{
  DataStore<T> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < pattern.size(); ++i)
  {
    inStore.setValue(i, static_cast<T>(pattern[i]));
  }
  DataStore<float32> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0.0f);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  DanielssonDistanceInCore<T> engine(inStore, outStore, SizeVec3{dx, dy, dz}, squared, useSpacing, spacing, shouldCancel, messageHandler);
  const Result<> r = engine();
  REQUIRE(r.valid());
  std::vector<float32> out(pattern.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

template <class T>
std::vector<float32> RunSlab(const std::vector<int32>& pattern, usize dx, usize dy, usize dz, bool squared, bool useSpacing, FloatVec3 spacing)
{
  DataStore<T> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < pattern.size(); ++i)
  {
    inStore.setValue(i, static_cast<T>(pattern[i]));
  }
  DataStore<float32> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0.0f);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  DanielssonDistanceSlab<T> engine(inStore, outStore, SizeVec3{dx, dy, dz}, squared, useSpacing, spacing, shouldCancel, messageHandler);
  const Result<> r = engine();
  REQUIRE(r.valid());
  std::vector<float32> out(pattern.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// A deterministic mixed image: object blobs (value 1) on a background (0), plus a scattering of specks.
std::vector<int32> MakePattern(usize dx, usize dy, usize dz)
{
  std::vector<int32> v(dx * dy * dz, 0);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        const bool block = (x >= dx / 2) && (y >= dy / 2);
        const bool speck = ((x + 2 * y + 3 * z) % 5 == 0);
        v[FlatIndex(x, y, z, dx, dy)] = (block || speck) ? 1 : 0;
      }
    }
  }
  return v;
}
} // namespace

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: tracked update keeps the vector norm exact", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  constexpr std::array<int32, 5> k_HereComponents{-3, -1, 0, 2, 5};
  constexpr std::array<int32, 4> k_ThereComponents{-4, 0, 1, 3};
  constexpr std::array<std::array<int32, 3>, 6> k_Offsets{{{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}}};
  constexpr std::array<std::array<int32, 3>, 3> k_ChainOffsets{{{-1, 0, 0}, {0, -1, 0}, {0, 0, -1}}};

  for(const bool useSpacing : {false, true})
  {
    const std::array<float64, 3> spacing = useSpacing ? std::array<float64, 3>{1.5, 2.0, 0.5} : std::array<float64, 3>{1.0, 1.0, 1.0};
    for(const int32 hereX : k_HereComponents)
    {
      for(const int32 hereY : k_HereComponents)
      {
        for(const int32 hereZ : k_HereComponents)
        {
          for(const int32 thereX : k_ThereComponents)
          {
            for(const int32 thereY : k_ThereComponents)
            {
              for(const int32 thereZ : k_ThereComponents)
              {
                const std::array<int32, 3> initialHere{hereX, hereY, hereZ};
                const std::array<int32, 3> there{thereX, thereY, thereZ};
                for(const std::array<int32, 3>& offset : k_Offsets)
                {
                  std::array<int32, 3> plainHere = initialHere;
                  std::array<int32, 3> trackedHere = initialHere;
                  float64 hereNorm = ImageProcessing::detail::DanielssonVectorNorm(trackedHere.data(), useSpacing, spacing.data());

                  ImageProcessing::detail::DanielssonUpdateCore(plainHere.data(), there.data(), offset[0], offset[1], offset[2], useSpacing, spacing.data());
                  ImageProcessing::detail::DanielssonUpdateCoreTracked(trackedHere.data(), hereNorm, there.data(), offset[0], offset[1], offset[2], useSpacing, spacing.data());

                  CAPTURE(useSpacing, hereX, hereY, hereZ, thereX, thereY, thereZ, offset[0], offset[1], offset[2]);
                  REQUIRE(trackedHere == plainHere);
                  REQUIRE(hereNorm == ImageProcessing::detail::DanielssonVectorNorm(trackedHere.data(), useSpacing, spacing.data()));
                }

                std::array<int32, 3> plainChain = initialHere;
                std::array<int32, 3> trackedChain = initialHere;
                float64 chainNorm = ImageProcessing::detail::DanielssonVectorNorm(trackedChain.data(), useSpacing, spacing.data());
                for(const std::array<int32, 3>& offset : k_ChainOffsets)
                {
                  ImageProcessing::detail::DanielssonUpdateCore(plainChain.data(), there.data(), offset[0], offset[1], offset[2], useSpacing, spacing.data());
                  ImageProcessing::detail::DanielssonUpdateCoreTracked(trackedChain.data(), chainNorm, there.data(), offset[0], offset[1], offset[2], useSpacing, spacing.data());
                }

                CAPTURE(useSpacing, hereX, hereY, hereZ, thereX, thereY, thereZ);
                REQUIRE(trackedChain == plainChain);
                REQUIRE(chainNorm == ImageProcessing::detail::DanielssonVectorNorm(trackedChain.data(), useSpacing, spacing.data()));
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: in-core engine matches through a transfer-counting input", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  constexpr usize k_DimX = 37;
  constexpr usize k_DimY = 29;
  constexpr usize k_DimZ = 11;
  constexpr usize k_Volume = k_DimX * k_DimY * k_DimZ;
  const SizeVec3 dims{k_DimX, k_DimY, k_DimZ};
  const FloatVec3 spacing{1.25f, 2.0f, 0.75f};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  DataStore<int32> plainInputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{0});
  TransferCountingDataStore<int32> countingInputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{0});
  for(usize index = 0; index < pattern.size(); ++index)
  {
    plainInputStore.setValue(index, pattern[index]);
    countingInputStore.setValue(index, pattern[index]);
  }
  DataStore<float32> plainOutputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
  TransferCountingDataStore<float32> countingOutputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  DanielssonDistanceInCore<int32> plainEngine(plainInputStore, plainOutputStore, dims, /*squaredDistance=*/false, /*useSpacing=*/true, spacing, shouldCancel, messageHandler);
  DanielssonDistanceInCore<int32> countingEngine(countingInputStore, countingOutputStore, dims, /*squaredDistance=*/false, /*useSpacing=*/true, spacing, shouldCancel, messageHandler);
  REQUIRE(plainEngine().valid());
  REQUIRE(countingEngine().valid());

  std::vector<float32> plainOutput(k_Volume);
  std::vector<float32> countingOutput(k_Volume);
  for(usize index = 0; index < k_Volume; ++index)
  {
    plainOutput[index] = plainOutputStore.getValue(index);
    countingOutput[index] = countingOutputStore.getValue(index);
  }
  REQUIRE(countingOutput == plainOutput);
  REQUIRE(countingInputStore.readValues() == 0);
  REQUIRE(countingOutputStore.writtenValues() == 0);
}

// A full seed plane on one axis makes the transform purely 1D along that axis -- Danielsson's 4SED is EXACT there, so
// this pins down each axis's forward/backward propagation and the spacing weighting against the exact EDT. Exercises
// all three axes and both sqrt/squared and spacing branches.
TEMPLATE_TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: axis seed planes == exact EDT", "[ImageProcessing][DanielssonDistanceMapEngine]", uint8, int16, int32)
{
  using T = TestType;
  const bool squared = GENERATE(false, true);
  const bool useSpacing = GENERATE(false, true);
  const FloatVec3 spacing = useSpacing ? FloatVec3{2.0f, 1.5f, 3.0f} : FloatVec3{1.0f, 1.0f, 1.0f};
  CAPTURE(squared, useSpacing);

  constexpr usize DX = 6, DY = 5, DZ = 4;
  auto runAxis = [&](int axis) {
    std::vector<int32> pattern(DX * DY * DZ, 0);
    for(usize z = 0; z < DZ; ++z)
    {
      for(usize y = 0; y < DY; ++y)
      {
        for(usize x = 0; x < DX; ++x)
        {
          const bool seed = (axis == 0 && x == 0) || (axis == 1 && y == 0) || (axis == 2 && z == 0);
          if(seed)
          {
            pattern[FlatIndex(x, y, z, DX, DY)] = 1;
          }
        }
      }
    }
    const std::vector<float32> oracle = ExactEdtOracle(pattern, DX, DY, DZ, squared, useSpacing, spacing);
    const std::vector<float32> got = RunInCore<T>(pattern, DX, DY, DZ, squared, useSpacing, spacing);
    for(usize i = 0; i < got.size(); ++i)
    {
      INFO("axis " << axis << " index " << i);
      REQUIRE(got[i] == oracle[i]);
    }
  };

  SECTION("seed plane x=0")
  {
    runAxis(0);
  }
  SECTION("seed plane y=0")
  {
    runAxis(1);
  }
  SECTION("seed plane z=0")
  {
    runAxis(2);
  }
}

// A filled half-space of features {x >= k}: the zero region x<k has its nearest feature on the x=k plane, so the
// distance is exactly (k-x) along x -- 4SED-exact and hand-obvious.
TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: half-space == exact EDT", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  constexpr usize DX = 8, DY = 4, DZ = 3, K = 5;
  std::vector<int32> pattern(DX * DY * DZ, 0);
  for(usize z = 0; z < DZ; ++z)
  {
    for(usize y = 0; y < DY; ++y)
    {
      for(usize x = K; x < DX; ++x)
      {
        pattern[FlatIndex(x, y, z, DX, DY)] = 1;
      }
    }
  }
  const std::vector<float32> got = RunInCore<int32>(pattern, DX, DY, DZ, /*squared=*/true, /*useSpacing=*/false, FloatVec3{1.0f, 1.0f, 1.0f});
  for(usize z = 0; z < DZ; ++z)
  {
    for(usize y = 0; y < DY; ++y)
    {
      for(usize x = 0; x < DX; ++x)
      {
        const int64 d = (x >= K) ? 0 : static_cast<int64>(K - x);
        INFO("x=" << x << " y=" << y << " z=" << z);
        REQUIRE(got[FlatIndex(x, y, z, DX, DY)] == static_cast<float32>(d * d));
      }
    }
  }
}

// 4SED invariants that hold for ANY pattern (Danielsson's vectors always point to a real feature, so the distance is
// never below the exact EDT, and feature voxels are always 0). Runs on a realistic blob+speck image, 3D and 2D. The
// exact-to-ITK correctness bar (where 4SED over-estimates the exact EDT) lives in the filter test's live-ITK grid.
TEMPLATE_TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: 4SED lower-bound + feature-zero invariants", "[ImageProcessing][DanielssonDistanceMapEngine]", uint8, int16, int32)
{
  using T = TestType;
  auto check = [&](usize dx, usize dy, usize dz) {
    const std::vector<int32> pattern = MakePattern(dx, dy, dz);
    const std::vector<float32> oracle = ExactEdtOracle(pattern, dx, dy, dz, /*squared=*/true, /*useSpacing=*/false, FloatVec3{1.0f, 1.0f, 1.0f});
    const std::vector<float32> got = RunInCore<T>(pattern, dx, dy, dz, /*squared=*/true, /*useSpacing=*/false, FloatVec3{1.0f, 1.0f, 1.0f});
    REQUIRE(got.size() == oracle.size());
    for(usize i = 0; i < got.size(); ++i)
    {
      INFO("index " << i << " dims " << dx << "x" << dy << "x" << dz);
      REQUIRE(got[i] >= 0.0f);
      REQUIRE(got[i] >= oracle[i]); // 4SED never underestimates the exact EDT (squared -> integer-exact comparison)
      if(pattern[i] != 0)
      {
        REQUIRE(got[i] == 0.0f); // a feature voxel is its own nearest feature
      }
    }
  };

  SECTION("3D 7x6x5")
  {
    check(7, 6, 5);
  }
  SECTION("2D 11x9x1")
  {
    check(11, 9, 1);
  }
}

// D3 gate: the out-of-core z-streamed slab must produce BYTE-IDENTICAL output to the in-core driver, since both run
// the identical reflective odometer via DanielssonPlanePass. Covered across types, sqrt/squared, spacing, and 3D/2D on
// a realistic blob+speck image.
TEMPLATE_TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: OOC slab == in-core (D3 gate)", "[ImageProcessing][DanielssonDistanceMapEngine]", uint8, int16, int32)
{
  using T = TestType;
  const bool squared = GENERATE(false, true);
  const bool useSpacing = GENERATE(false, true);
  const FloatVec3 spacing = useSpacing ? FloatVec3{1.5f, 2.0f, 0.5f} : FloatVec3{1.0f, 1.0f, 1.0f};
  CAPTURE(squared, useSpacing);

  auto check = [&](usize dx, usize dy, usize dz) {
    const std::vector<int32> pattern = MakePattern(dx, dy, dz);
    const std::vector<float32> inCore = RunInCore<T>(pattern, dx, dy, dz, squared, useSpacing, spacing);
    const std::vector<float32> slab = RunSlab<T>(pattern, dx, dy, dz, squared, useSpacing, spacing);
    REQUIRE(inCore.size() == slab.size());
    for(usize i = 0; i < inCore.size(); ++i)
    {
      INFO("index " << i << " dims " << dx << "x" << dy << "x" << dz);
      REQUIRE(slab[i] == inCore[i]);
    }
  };

  SECTION("3D 8x7x6")
  {
    check(8, 7, 6);
  }
  SECTION("2D 13x10x1")
  {
    check(13, 10, 1);
  }
  SECTION("small 3x3x3")
  {
    check(3, 3, 3);
  }
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: rolling z-neighbor edge cases preserve parity", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  auto check = [](usize dx, usize dy, usize dz) {
    std::vector<int32> pattern(dx * dy * dz, 0);
    pattern[FlatIndex(1, 0, 0, dx, dy)] = 1;
    pattern[FlatIndex(dx - 2, dy - 1, dz - 1, dx, dy)] = 1;
    if(dz > 2)
    {
      pattern[FlatIndex(dx / 2, 1, dz / 2, dx, dy)] = 1;
    }

    const FloatVec3 spacing{1.25f, 2.0f, 0.75f};
    const std::vector<float32> inCore = RunInCore<int32>(pattern, dx, dy, dz, /*squared=*/true, /*useSpacing=*/true, spacing);
    const std::vector<float32> slab = RunSlab<int32>(pattern, dx, dy, dz, /*squared=*/true, /*useSpacing=*/true, spacing);
    REQUIRE(slab == inCore);
  };

  SECTION("Z=1 has no adjacent plane")
  {
    check(7, 5, 1);
  }
  SECTION("Z=2 turns directly from the forward to backward sub-sweep")
  {
    check(7, 5, 2);
  }
  SECTION("asymmetric 3D seeds require both sweep directions")
  {
    check(7, 5, 6);
  }
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: pre-cancel preserves poison output", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  constexpr usize kDimX = 5;
  constexpr usize kDimY = 4;
  constexpr usize kDimZ = 3;
  constexpr float32 kPoison = -12345.0f;
  DataStore<int32> inputStore(ShapeType{kDimZ, kDimY, kDimX}, ShapeType{1}, 0);
  DataStore<float32> outputStore(ShapeType{kDimZ, kDimY, kDimX}, ShapeType{1}, kPoison);
  std::atomic_bool shouldCancel{true};
  IFilter::MessageHandler messageHandler{};

  DanielssonDistanceSlab<int32> engine(inputStore, outputStore, SizeVec3{kDimX, kDimY, kDimZ}, /*squaredDistance=*/true, /*useSpacing=*/false, FloatVec3{1.0f, 1.0f, 1.0f}, shouldCancel,
                                       messageHandler);
  const Result<> result = engine();
  REQUIRE(result.valid());
  for(const float32 value : outputStore)
  {
    REQUIRE(value == kPoison);
  }
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: in-core pre-cancel preserves poison output", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 4;
  constexpr usize k_DimZ = 3;
  constexpr float32 k_Poison = -12345.0f;
  DataStore<int32> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{0});
  DataStore<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
  std::atomic_bool shouldCancel{true};
  IFilter::MessageHandler messageHandler{};

  DanielssonDistanceInCore<int32> engine(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, /*squaredDistance=*/true, /*useSpacing=*/false, FloatVec3{1.0f, 1.0f, 1.0f}, shouldCancel,
                                         messageHandler);
  const Result<> result = engine();
  REQUIRE(result.valid());
  for(const float32 value : outputStore)
  {
    REQUIRE(value == k_Poison);
  }
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: mixed storage selects an OOC working format", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  constexpr const char* kInputOocFormat = "HDF5-OOC-Input";
  constexpr const char* kOutputOocFormat = "HDF5-OOC-Output";

  SECTION("resident input and resident output preserve the input format")
  {
    const auto selectedFormat =
        nx::core::ImageProcessing::detail::SelectDanielssonWorkingDataFormat(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::InMemory, "Alternate-DataStore");
    REQUIRE(selectedFormat == DataStore<float32>::k_DataStore);
  }

  SECTION("resident input and OOC output select the output format")
  {
    const auto selectedFormat =
        nx::core::ImageProcessing::detail::SelectDanielssonWorkingDataFormat(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::OutOfCore, kOutputOocFormat);
    REQUIRE(selectedFormat == kOutputOocFormat);
  }

  SECTION("OOC input and resident output select the input format")
  {
    const auto selectedFormat =
        nx::core::ImageProcessing::detail::SelectDanielssonWorkingDataFormat(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore);
    REQUIRE(selectedFormat == kInputOocFormat);
  }

  SECTION("OOC input takes precedence when both endpoints are OOC")
  {
    const auto selectedFormat =
        nx::core::ImageProcessing::detail::SelectDanielssonWorkingDataFormat(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::OutOfCore, kOutputOocFormat);
    REQUIRE(selectedFormat == kInputOocFormat);
  }
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: resident state requires a complete dataset-scaled reservation", "[ImageProcessing][DanielssonDistanceMapEngine][WorkingMemory]")
{
  constexpr usize dimX = 512;
  constexpr usize dimY = 512;
  constexpr usize dimZ = 32;
  constexpr usize sliceValues = dimX * dimY;
  constexpr usize valueCount = sliceValues * dimZ;
  constexpr usize visitCount = (2 * dimX - 2) + (2 * dimY - 2) + (2 * dimZ - 2);
  constexpr usize expectedBytes =
      valueCount * (3 * sizeof(int32) + sizeof(uint8)) + sliceValues * sizeof(uint8) + sliceValues * sizeof(float32) + visitCount * sizeof(ImageProcessing::detail::AxisVisit);
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{dimX, dimY, dimZ};

  auto requiredResult = ImageProcessing::detail::CalculateDanielssonResidentWorkingMemoryBytes<uint8>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == expectedBytes);
  auto calculateDanielssonResidentWorkingMemoryBytesResult = ImageProcessing::detail::CalculateDanielssonResidentWorkingMemoryBytes<uint8>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(calculateDanielssonResidentWorkingMemoryBytesResult);

  REQUIRE(ImageProcessing::detail::ShouldUseDanielssonResidentState(dims));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseDanielssonResidentState(SizeVec3{dimX, dimY, 1}));

  // Cache budgets of at most 1 GiB avoid the machine-dependent upper cap.
  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  REQUIRE_FALSE(manager.setBudgetBytes(256 * k_MiB));
  {
    auto allocationResult = ImageProcessing::detail::ReserveDanielssonResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 64 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  REQUIRE_FALSE(manager.setBudgetBytes(512 * k_MiB));
  {
    auto allocationResult = ImageProcessing::detail::ReserveDanielssonResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: the slab path reads each input plane once and the resident path reads the span",
          "[ImageProcessing][DanielssonDistanceMapEngine][WorkingMemory]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  constexpr usize sliceValues = dimX * dimY;
  constexpr usize valueCount = sliceValues * dimZ;
  constexpr usize visitCount = (2 * dimX - 2) + (2 * dimY - 2) + (2 * dimZ - 2);
  constexpr uint64 k_RequiredBytes =
      valueCount * (3 * sizeof(int32) + sizeof(uint8)) + sliceValues * sizeof(uint8) + sliceValues * sizeof(float32) + visitCount * sizeof(ImageProcessing::detail::AxisVisit);
  constexpr uint64 k_PartialBudgetBytes = 8192;
  constexpr uint64 k_CompleteBudgetBytes = 4 * k_RequiredBytes;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const FloatVec3 spacing{1.0f, 1.0f, 1.0f};
  const std::vector<int32> pattern = MakePattern(dimX, dimY, dimZ);
  const std::vector<float32> expected = RunInCore<uint8>(pattern, dimX, dimY, dimZ, true, false, spacing);
  const auto requiredResult = ImageProcessing::detail::CalculateDanielssonResidentWorkingMemoryBytes<uint8>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == k_RequiredBytes);

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();

  auto run = [&](uint64 budgetBytes) {
    manager.clear();
    manager.setBudgetBytes(budgetBytes);
    ReadCountingDataStore<uint8> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{0});
    DataStore<float32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
    for(usize index = 0; index < pattern.size(); ++index)
    {
      inputStore.setValue(index, static_cast<uint8>(pattern[index]));
    }

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    DanielssonDistanceWorkingMemory<uint8> engine(inputStore, outputStore, dims, true, false, spacing, shouldCancel, messageHandler);
    auto engineResult = engine();
    SIMPLNX_RESULT_REQUIRE_VALID(engineResult);

    std::vector<float32> actual(pattern.size());
    auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
    REQUIRE(actual == expected);
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    return inputStore.readCount();
  };

  // The partial budget selects the slab, which streams each input plane exactly once. The complete budget selects the
  // resident engine, which reads the in-memory input through its span and performs no bulk transfers.
  REQUIRE(run(k_PartialBudgetBytes) == dimZ);
  REQUIRE(run(k_CompleteBudgetBytes) == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: scalar axis visits match the reflective visit list", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  for(const int64 length : {int64{1}, int64{2}, int64{5}})
  {
    const std::vector<ImageProcessing::detail::AxisVisit> expected = ImageProcessing::detail::BuildAxisVisits(length);
    std::vector<ImageProcessing::detail::AxisVisit> actual;
    ImageProcessing::detail::ForEachAxisVisit(length, [&](const ImageProcessing::detail::AxisVisit& visit) { actual.push_back(visit); });
    CAPTURE(length);
    REQUIRE(actual.size() == expected.size());
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(actual[index].coord == expected[index].coord);
      REQUIRE(actual[index].pull == expected[index].pull);
    }
  }
  REQUIRE(ImageProcessing::detail::DanielssonVisitArraysFit(512, 512, 128));
  REQUIRE_FALSE(ImageProcessing::detail::DanielssonVisitArraysFit(8, 7, 6, 0));
  REQUIRE_FALSE(ImageProcessing::detail::DanielssonVisitArraysFit(1, 1, 1000000000));
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: 2D planner bounds blocks and overwide rows", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  const auto benchmark = ImageProcessing::detail::BuildDanielsson2DBufferPlan(5888, 5888, sizeof(uint8));
  REQUIRE(benchmark.valid);
  REQUIRE(benchmark.coreCols == 5888);
  REQUIRE(benchmark.coreRows > 1);
  REQUIRE(benchmark.residentBytes <= ImageProcessing::detail::k_Danielsson2DResidentLimit);

  for(const usize inputBytes : {sizeof(uint8), sizeof(uint64)})
  {
    const auto stress = ImageProcessing::detail::BuildDanielsson2DBufferPlan(16385, 1025, inputBytes);
    CAPTURE(inputBytes);
    REQUIRE(stress.valid);
    REQUIRE(stress.coreCols == 16385);
    REQUIRE(stress.coreRows > 0);
    REQUIRE(stress.residentBytes <= ImageProcessing::detail::k_Danielsson2DResidentLimit);
  }

  const auto overwide = ImageProcessing::detail::BuildDanielsson2DBufferPlan(100000000, 2, sizeof(uint8));
  REQUIRE(overwide.valid);
  REQUIRE(overwide.coreCols < 100000000);
  REQUIRE(overwide.coreRows == 1);
  REQUIRE(overwide.residentBytes <= ImageProcessing::detail::k_Danielsson2DResidentLimit);

  constexpr usize kSmallLimit = 6000;
  const auto forcedTiled = ImageProcessing::detail::BuildDanielsson2DBufferPlan(100, 8, sizeof(uint8), kSmallLimit);
  REQUIRE(forcedTiled.valid);
  REQUIRE(forcedTiled.coreCols < 100);
  REQUIRE(forcedTiled.coreRows == 1);
  REQUIRE(forcedTiled.residentBytes <= kSmallLimit);

  const auto oneCell = ImageProcessing::detail::BuildDanielsson2DBufferPlan(1, 1, sizeof(uint64));
  REQUIRE(oneCell.valid);
  REQUIRE(oneCell.residentBytes <= ImageProcessing::detail::k_Danielsson2DResidentLimit);
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: 2D planner rejects invalid and overflow dimensions", "[ImageProcessing][DanielssonDistanceMapEngine]")
{
  const auto overflow = ImageProcessing::detail::BuildDanielsson2DBufferPlan(std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max(), sizeof(uint64));
  const auto offsetOverflow = ImageProcessing::detail::BuildDanielsson2DBufferPlan(ImageProcessing::detail::k_DanielssonMaxDimension + 1, 1, sizeof(uint8));
  const auto zeroX = ImageProcessing::detail::BuildDanielsson2DBufferPlan(0, 1, sizeof(uint8));
  const auto zeroY = ImageProcessing::detail::BuildDanielsson2DBufferPlan(1, 0, sizeof(uint8));
  REQUIRE_FALSE(overflow.valid);
  REQUIRE(overflow.overflow);
  REQUIRE_FALSE(offsetOverflow.valid);
  REQUIRE(offsetOverflow.overflow);
  REQUIRE_FALSE(zeroX.valid);
  REQUIRE(zeroX.overflow);
  REQUIRE_FALSE(zeroY.valid);
  REQUIRE(zeroY.overflow);
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: resident vector-prefix plan reserves only fixed live planes plus complete prefix planes",
          "[ImageProcessing][DanielssonDistanceMapEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 1024;
  constexpr usize k_DimY = 1024;
  constexpr usize k_DimZ = 256;
  constexpr usize k_PlaneValues = k_DimX * k_DimY;
  constexpr usize k_GrantBytes = 128ULL * 1024ULL * 1024ULL;
  // The bounded path keeps one input plane, two live int32 vector planes, one float32 output plane, one int16
  // staging plane, and the X/Y reflective-visit payload while processing the Z schedule.
  constexpr usize k_AxisVisitBytes = 2 * (2 * k_DimX - 2) * sizeof(ImageProcessing::detail::AxisVisit);
  constexpr usize k_FixedBytes = k_PlaneValues * (sizeof(uint8) + 2 * 3 * sizeof(int32) + sizeof(float32) + 3 * sizeof(int16)) + k_AxisVisitBytes;
  constexpr usize k_VectorPlaneBytes = k_PlaneValues * 3 * sizeof(int32);
  constexpr usize k_ExpectedPrefixPlanes = (k_GrantBytes - k_FixedBytes) / k_VectorPlaneBytes;
  const SizeVec3 dims{k_DimX, k_DimY, k_DimZ};

  const auto partialPlan = ImageProcessing::detail::CreateDanielssonResidentVectorPrefixPlan<uint8>(dims, k_GrantBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(partialPlan);
  REQUIRE(partialPlan.value().fixedBytes == k_FixedBytes);
  REQUIRE(partialPlan.value().axisVisitBytes == k_AxisVisitBytes);
  REQUIRE(partialPlan.value().residentPrefixPlanes == k_ExpectedPrefixPlanes);
  REQUIRE(partialPlan.value().residentPrefixPlanes > 0);
  REQUIRE(partialPlan.value().residentPrefixPlanes < k_DimZ);
  REQUIRE(partialPlan.value().residentPrefixValues == k_ExpectedPrefixPlanes * k_PlaneValues * 3);
  REQUIRE(partialPlan.value().residentBytes <= k_GrantBytes);

  const auto noPrefixPlan = ImageProcessing::detail::CreateDanielssonResidentVectorPrefixPlan<uint8>(dims, k_FixedBytes - 1);
  SIMPLNX_RESULT_REQUIRE_VALID(noPrefixPlan);
  REQUIRE(noPrefixPlan.value().residentPrefixPlanes == 0);
  REQUIRE(noPrefixPlan.value().residentBytes == 0);

  const auto turnaroundPlan = ImageProcessing::detail::CreateDanielssonResidentVectorPrefixPlan<uint8>(SizeVec3{7, 5, 4}, std::numeric_limits<usize>::max());
  SIMPLNX_RESULT_REQUIRE_VALID(turnaroundPlan);
  REQUIRE(turnaroundPlan.value().residentPrefixPlanes == 2);

  constexpr usize k_OverLimitDim = ImageProcessing::detail::k_DanielssonVisitArrayLimit / (2 * sizeof(ImageProcessing::detail::AxisVisit)) + 2;
  const auto generatedPlan = ImageProcessing::detail::CreateDanielssonResidentVectorPrefixPlan<uint8>(SizeVec3{k_OverLimitDim, 1, 3}, std::numeric_limits<usize>::max());
  SIMPLNX_RESULT_REQUIRE_VALID(generatedPlan);
  REQUIRE(generatedPlan.value().axisVisitBytes == 0);

  auto createDanielssonResidentVectorPrefixPlanResult = ImageProcessing::detail::CreateDanielssonResidentVectorPrefixPlan<uint8>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2}, k_GrantBytes);
  SIMPLNX_RESULT_REQUIRE_INVALID(createDanielssonResidentVectorPrefixPlanResult);

  // The last forward state remains in the two existing live vector planes at the Z turnaround. A prefix can therefore
  // cover only planes 0..Z-3; it has zero vector-scratch reads and writes.
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: resident vector prefixes preserve 3D reflective output order", "[ImageProcessing][DanielssonDistanceMapEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 4;
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  const FloatVec3 spacing{0.5f, 1.25f, 2.0f};

  const auto run = [&](usize prefixPlanes, bool squared, bool useSpacing) {
    DataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
    for(usize index = 0; index < pattern.size(); ++index)
    {
      inputStore.setValue(index, static_cast<uint8>(pattern[index]));
    }
    DataStore<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    DanielssonDistanceSlab<uint8> engine(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, squared, useSpacing, spacing, shouldCancel, messageHandler,
                                         ImageProcessing::detail::k_Danielsson2DResidentLimit, ImageProcessing::detail::k_DanielssonVisitArrayLimit, prefixPlanes);
    auto engineResult = engine();
    SIMPLNX_RESULT_REQUIRE_VALID(engineResult);
    std::vector<float32> output(pattern.size());
    auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<float32>(output.data(), output.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
    return output;
  };

  for(const bool squared : {false, true})
  {
    for(const bool useSpacing : {false, true})
    {
      const std::vector<float32> baseline = run(/*prefixPlanes=*/0, squared, useSpacing);
      REQUIRE(run(/*prefixPlanes=*/2, squared, useSpacing) == baseline);
      REQUIRE(run(/*prefixPlanes=*/k_DimZ, squared, useSpacing) == baseline);
    }
  }
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: compact scratch preserves 3D generated-schedule parity at the int16 boundary", "[ImageProcessing][DanielssonDistanceMapEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 16383;
  constexpr usize k_DimY = 1;
  constexpr usize k_DimZ = 3;
  constexpr usize k_VoxelCount = k_DimX * k_DimY * k_DimZ;
  const SizeVec3 dims{k_DimX, k_DimY, k_DimZ};
  REQUIRE(ImageProcessing::detail::DanielssonCanUseCompactVectorScratch(dims));

  std::vector<uint8> pattern(k_VoxelCount, uint8{0});
  pattern[FlatIndex(0, 0, 1, k_DimX, k_DimY)] = 1;
  DataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
  auto copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const uint8>(pattern.data(), pattern.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  DataStore<float32> inCoreOutput(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0F);
  DataStore<float32> slabOutput(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0F);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  DanielssonDistanceInCore<uint8> inCore(inputStore, inCoreOutput, dims, /*squaredDistance=*/true, /*useSpacing=*/false, FloatVec3{1.0F, 1.0F, 1.0F}, shouldCancel, messageHandler);
  auto inCoreResult = inCore();
  SIMPLNX_RESULT_REQUIRE_VALID(inCoreResult);
  DanielssonDistanceSlab<uint8> slab(inputStore, slabOutput, dims, /*squaredDistance=*/true, /*useSpacing=*/false, FloatVec3{1.0F, 1.0F, 1.0F}, shouldCancel, messageHandler,
                                     ImageProcessing::detail::k_Danielsson2DResidentLimit, /*visitArrayLimit=*/0);
  auto slabResult = slab();
  SIMPLNX_RESULT_REQUIRE_VALID(slabResult);

  std::vector<float32> inCoreValues(k_VoxelCount);
  std::vector<float32> slabValues(k_VoxelCount);
  auto copyIntoBufferResult = inCoreOutput.copyIntoBuffer(0, nonstd::span<float32>(inCoreValues.data(), inCoreValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  auto copyIntoBufferResult2 = slabOutput.copyIntoBuffer(0, nonstd::span<float32>(slabValues.data(), slabValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult2);
  REQUIRE(slabValues == inCoreValues);
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapEngine: fused 3D schedule transfers each scratch suffix plane once", "[ImageProcessing][DanielssonDistanceMapEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 3;
  constexpr usize k_DimY = 2;
  constexpr usize k_DimZ = 4;
  constexpr usize k_PrefixPlanes = 1;
  constexpr usize k_VectorValuesPerPlane = k_DimX * k_DimY * 3;

  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  DataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
  for(usize index = 0; index < pattern.size(); ++index)
  {
    inputStore.setValue(index, static_cast<uint8>(pattern[index]));
  }
  DataStore<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0F);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  ImageProcessing::detail::DanielssonVectorTransferStats transferStats;
  DanielssonDistanceSlab<uint8> engine(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, /*squaredDistance=*/true, /*useSpacing=*/false, FloatVec3{1.0F, 1.0F, 1.0F}, shouldCancel,
                                       messageHandler, ImageProcessing::detail::k_Danielsson2DResidentLimit, ImageProcessing::detail::k_DanielssonVisitArrayLimit, k_PrefixPlanes, &transferStats);
  auto engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);

  REQUIRE(transferStats.scratchWrites == k_DimZ - 2 - k_PrefixPlanes);
  REQUIRE(transferStats.scratchReads == k_DimZ - 2 - k_PrefixPlanes);
  REQUIRE(transferStats.scratchWritePlanes == std::vector<usize>{1});
  REQUIRE(transferStats.scratchReadPlanes == std::vector<usize>{1});
  REQUIRE(transferStats.scratchWriteValues == (k_DimZ - 2 - k_PrefixPlanes) * k_VectorValuesPerPlane);
  REQUIRE(transferStats.scratchReadValues == (k_DimZ - 2 - k_PrefixPlanes) * k_VectorValuesPerPlane);
  REQUIRE(transferStats.scratchWriteBytes == (k_DimZ - 2 - k_PrefixPlanes) * k_VectorValuesPerPlane * sizeof(int16));
  REQUIRE(transferStats.scratchReadBytes == (k_DimZ - 2 - k_PrefixPlanes) * k_VectorValuesPerPlane * sizeof(int16));
}
