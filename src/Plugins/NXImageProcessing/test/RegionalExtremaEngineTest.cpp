#include "simplnx/Utilities/ImageProcessing/RegionalExtremaEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return z * (dimY * dimX) + y * dimX + x;
}

template <class T>
DataStore<T> MakeStore(const std::vector<T>& values, usize dimX, usize dimY, usize dimZ)
{
  DataStore<T> store(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < values.size(); ++i)
  {
    store.setValue(i, values[i]);
  }
  return store;
}

// INDEPENDENT oracle: mark every pixel that is NOT a regional extremum, via a Jacobi iterate on a SNAPSHOT to
// stability. A pixel is marked if it has a strictly-more-extreme neighbor (seed) OR a same-value neighbor is already
// marked (flat-zone propagation). Output = mark ? markerValue : input. Structurally independent of the scan-order
// stack flood and the directional plane sweep (no stack, no scan order, no streaming), so a bug in either engine
// cannot hide behind it. Converges (monotone: marks only ever added).
template <class T>
std::vector<T> RegionalExtremaOracle(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, RegionalExtremaOp op, bool fullyConnected)
{
  const bool maxima = (op == RegionalExtremaOp::Maxima);
  const T markerValue = maxima ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
  auto beyond = [maxima](T a, T b) { return maxima ? (a > b) : (a < b); };

  std::vector<std::array<int, 3>> offs;
  for(int dz = -1; dz <= 1; ++dz)
  {
    for(int dy = -1; dy <= 1; ++dy)
    {
      for(int dx = -1; dx <= 1; ++dx)
      {
        if(dx == 0 && dy == 0 && dz == 0)
        {
          continue;
        }
        if(!fullyConnected && (std::abs(dx) + std::abs(dy) + std::abs(dz)) != 1)
        {
          continue;
        }
        offs.push_back({dx, dy, dz});
      }
    }
  }
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);
  auto at = [&](int64 x, int64 y, int64 z) { return input[FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dimX, dimY)]; };

  std::vector<char> mark(input.size(), 0);
  bool changed = true;
  while(changed)
  {
    changed = false;
    const std::vector<char> snap = mark;
    for(int64 z = 0; z < nZ; ++z)
    {
      for(int64 y = 0; y < nY; ++y)
      {
        for(int64 x = 0; x < nX; ++x)
        {
          const usize p = FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dimX, dimY);
          if(mark[p] != 0)
          {
            continue;
          }
          const T v = at(x, y, z);
          bool m = false;
          for(const auto& o : offs)
          {
            const int64 qx = x + o[0];
            const int64 qy = y + o[1];
            const int64 qz = z + o[2];
            if(qx < 0 || qx >= nX || qy < 0 || qy >= nY || qz < 0 || qz >= nZ)
            {
              continue;
            }
            const usize q = FlatIndex(static_cast<usize>(qx), static_cast<usize>(qy), static_cast<usize>(qz), dimX, dimY);
            if(beyond(at(qx, qy, qz), v) || (at(qx, qy, qz) == v && snap[q] != 0))
            {
              m = true;
              break;
            }
          }
          if(m)
          {
            mark[p] = 1;
            changed = true;
          }
        }
      }
    }
  }
  std::vector<T> out(input.size());
  for(usize i = 0; i < input.size(); ++i)
  {
    out[i] = (mark[i] != 0) ? markerValue : input[i];
  }
  return out;
}

template <template <class> class EngineT, class T>
std::vector<T> RunEngine(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, RegionalExtremaOp op, bool fullyConnected)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> r = EngineT<T>{inStore, outStore, SizeVec3{dimX, dimY, dimZ}, op, fullyConnected, shouldCancel, messageHandler}();
  REQUIRE(r.valid());
  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

template <class T>
std::vector<T> RunSweep(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, RegionalExtremaOp op, bool fullyConnected, usize maxSlabValues = RegionalExtremaSweep<T>::k_DefaultSlabValues)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> result = RegionalExtremaSweep<T>{inStore, outStore, SizeVec3{dimX, dimY, dimZ}, op, fullyConnected, shouldCancel, messageHandler, maxSlabValues}();
  REQUIRE(result.valid());
  std::vector<T> output(input.size());
  for(usize i = 0; i < output.size(); ++i)
  {
    output[i] = outStore.getValue(i);
  }
  return output;
}

template <class T>
void CheckRegionalExtrema(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ)
{
  for(RegionalExtremaOp op : {RegionalExtremaOp::Maxima, RegionalExtremaOp::Minima})
  {
    for(bool fullyConnected : {false, true})
    {
      const std::vector<T> oracle = RegionalExtremaOracle<T>(input, dimX, dimY, dimZ, op, fullyConnected);
      const std::vector<T> flood = RunEngine<RegionalExtremaFlood, T>(input, dimX, dimY, dimZ, op, fullyConnected);
      const std::vector<T> sweep = RunEngine<RegionalExtremaSweep, T>(input, dimX, dimY, dimZ, op, fullyConnected);
      REQUIRE(flood == oracle);
      REQUIRE(sweep == oracle);
      REQUIRE(sweep == flood);
    }
  }
}

// Deterministic multi-plateau pattern: coarse flat blocks at distinct levels + a diagonal ramp + pseudo-noise
// islands, values in [0, 90]. Guarantees multiple isolated flat maxima/minima, ramps (chains of non-extrema), and
// plateaus touching the border. Non-cubic-friendly.
template <class T>
std::vector<T> MakePlateauPattern(usize dimX, usize dimY, usize dimZ)
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
} // namespace

TEST_CASE("NXImageProcessing::RegionalExtremaEngine: total working-memory grants scale checked halo-slab plans without changing output", "[NXImageProcessing][RegionalExtremaEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 4;
  constexpr usize k_DimY = 3;
  constexpr usize k_DimZ = 7;
  constexpr usize k_PlaneValues = k_DimX * k_DimY;
  constexpr usize k_SmallerTargetBytes = 2 * 4 * k_PlaneValues * sizeof(int16);
  constexpr usize k_LargerTargetBytes = 2 * 6 * k_PlaneValues * sizeof(int16);

  auto smallerResult = ImageProcessing::detail::CreateRegionalExtremaWorkingMemoryPlan<int16>(SizeVec3{k_DimX, k_DimY, k_DimZ}, k_SmallerTargetBytes);
  REQUIRE(smallerResult.valid());
  const auto& smaller = smallerResult.value();
  REQUIRE(smaller.maxSlabValues == 4 * k_PlaneValues);
  REQUIRE(smaller.residentBytes <= k_SmallerTargetBytes);

  auto largerResult = ImageProcessing::detail::CreateRegionalExtremaWorkingMemoryPlan<int16>(SizeVec3{k_DimX, k_DimY, k_DimZ}, k_LargerTargetBytes);
  REQUIRE(largerResult.valid());
  const auto& larger = largerResult.value();
  REQUIRE(larger.maxSlabValues == 6 * k_PlaneValues);
  REQUIRE(larger.maxSlabValues > smaller.maxSlabValues);
  REQUIRE(larger.residentBytes <= k_LargerTargetBytes);

  constexpr usize k_NormalizedDimZ = 8;
  constexpr usize k_NormalizedTargetBytes = 2 * 7 * k_PlaneValues * sizeof(int16);
  auto normalizedResult = ImageProcessing::detail::CreateRegionalExtremaWorkingMemoryPlan<int16>(SizeVec3{k_DimX, k_DimY, k_NormalizedDimZ}, k_NormalizedTargetBytes);
  REQUIRE(normalizedResult.valid());
  REQUIRE(normalizedResult.value().maxSlabValues == 6 * k_PlaneValues);

  auto usefulResult = ImageProcessing::detail::CalculateRegionalExtremaUsefulWorkingMemoryBytes<int16>(SizeVec3{k_DimX, k_DimY, k_DimZ});
  REQUIRE(usefulResult.valid());
  REQUIRE(usefulResult.value() == 2 * k_DimX * k_DimY * k_DimZ * sizeof(int16));
  auto minimumResult = ImageProcessing::detail::CalculateRegionalExtremaMinimumWorkingMemoryBytes<int16>(SizeVec3{k_DimX, k_DimY, k_DimZ});
  REQUIRE(minimumResult.valid());
  REQUIRE(minimumResult.value() == 2 * 3 * k_PlaneValues * sizeof(int16));

  const std::vector<int16> input = MakePlateauPattern<int16>(k_DimX, k_DimY, k_DimZ);
  const std::vector<int16> expected = RegionalExtremaOracle(input, k_DimX, k_DimY, k_DimZ, RegionalExtremaOp::Maxima, true);
  REQUIRE(RunSweep(input, k_DimX, k_DimY, k_DimZ, RegionalExtremaOp::Maxima, true, smaller.maxSlabValues) == expected);
  REQUIRE(RunSweep(input, k_DimX, k_DimY, k_DimZ, RegionalExtremaOp::Maxima, true, larger.maxSlabValues) == expected);

  constexpr usize k_2DDimX = 7;
  constexpr usize k_2DDimY = 9;
  constexpr usize k_2DValues = k_2DDimX * k_2DDimY;
  constexpr usize k_2DTargetBytes = (k_2DValues + 2 * 4 * k_2DDimX) * sizeof(int16);
  const SizeVec3 k2DDims{k_2DDimX, k_2DDimY, 1};
  auto fixed2DResult = ImageProcessing::detail::CreateRegionalExtremaWorkingMemoryPlan<int16>(k2DDims, k_2DTargetBytes);
  REQUIRE(fixed2DResult.valid());
  REQUIRE(fixed2DResult.value().fixedWorkValues == k_2DValues);
  REQUIRE(fixed2DResult.value().fixedBufferValues >= 3 * k_2DDimX);
  REQUIRE(fixed2DResult.value().residentBytes <= k_2DTargetBytes);

  constexpr usize k_OverwideTargetBytes = 72;
  auto overwideResult = ImageProcessing::detail::CreateRegionalExtremaWorkingMemoryPlan<int16>(SizeVec3{31, 4, 1}, k_OverwideTargetBytes);
  REQUIRE(overwideResult.valid());
  const auto& overwide = overwideResult.value();
  REQUIRE(overwide.fixedWorkValues == 0);
  auto overwideSweepResult = ImageProcessing::detail::CreateSweep2DPlan(/*dimX=*/31, /*dimY=*/4, overwide.maxSlabValues, /*fullWidthHaloRows=*/2, /*tiledRows=*/3);
  REQUIRE(overwideSweepResult.valid());
  REQUIRE_FALSE(overwideSweepResult.value().fullWidth);
  const usize actualTileBytes = 2 * 3 * (overwideSweepResult.value().coreColumns + 2) * sizeof(int16);
  REQUIRE(actualTileBytes <= k_OverwideTargetBytes);
  REQUIRE(overwide.residentBytes >= actualTileBytes);

  REQUIRE(ImageProcessing::detail::CreateRegionalExtremaWorkingMemoryPlan<int16>(SizeVec3{k_DimX, k_DimY, k_DimZ}, /*targetBytes=*/1).invalid());

  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(256 * k_MiB);
  {
    ScopedWorkingMemoryTuningOverride override(128 * k_MiB);
    auto allocationResult = ImageProcessing::detail::ReserveRegionalExtremaWorkingMemoryPlan<int16>(SizeVec3{512, 512, 128});
    REQUIRE(allocationResult.valid());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == allocationResult.value().plan.residentBytes);
    REQUIRE(allocationResult.value().plan.residentBytes < 64 * k_MiB);
    REQUIRE(manager.reservedWorkingMemoryBytes() == allocationResult.value().plan.residentBytes);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("NXImageProcessing::RegionalExtremaEngine: multi-slab short tail preserves the exact fixpoint", "[RegionalExtremaEngine]")
{
  // A two-plane core over seven planes forces 2 + 2 + 2 + 1 in both directions. Only the first corridor voxel has
  // a strictly-more-extreme neighbor, so the marker must cross every slab boundary to reach the short tail.
  constexpr usize k_DimX = 4;
  constexpr usize k_DimY = 3;
  constexpr usize k_DimZ = 7;
  constexpr usize k_PlaneSize = k_DimX * k_DimY;
  constexpr usize k_SlabValueBudget = 4 * k_PlaneSize; // two core planes plus the two halo planes

  for(const RegionalExtremaOp op : {RegionalExtremaOp::Maxima, RegionalExtremaOp::Minima})
  {
    const int16 fieldValue = op == RegionalExtremaOp::Maxima ? int16{0} : int16{100};
    const int16 corridorValue = int16{50};
    const int16 seedValue = op == RegionalExtremaOp::Maxima ? int16{100} : int16{0};
    std::vector<int16> input(k_DimX * k_DimY * k_DimZ, fieldValue);
    for(usize z = 0; z < k_DimZ; ++z)
    {
      input[FlatIndex(1, 1, z, k_DimX, k_DimY)] = corridorValue;
    }
    input[FlatIndex(1, 1, 0, k_DimX, k_DimY)] = seedValue;

    for(const bool fullyConnected : {false, true})
    {
      const std::vector<int16> expected = RegionalExtremaOracle(input, k_DimX, k_DimY, k_DimZ, op, fullyConnected);
      const std::vector<int16> flood = RunEngine<RegionalExtremaFlood, int16>(input, k_DimX, k_DimY, k_DimZ, op, fullyConnected);
      const std::vector<int16> sweep = RunSweep(input, k_DimX, k_DimY, k_DimZ, op, fullyConnected, k_SlabValueBudget);
      INFO("op=" << static_cast<int>(op) << " fullyConnected=" << fullyConnected);
      REQUIRE(sweep == expected);
      REQUIRE(sweep == flood);
      REQUIRE(sweep[FlatIndex(1, 1, k_DimZ - 1, k_DimX, k_DimY)] == (op == RegionalExtremaOp::Maxima ? std::numeric_limits<int16>::lowest() : std::numeric_limits<int16>::max()));
    }
  }
}

TEST_CASE("NXImageProcessing::RegionalExtremaEngine: full-connectivity diagonal crosses slab halos and the short tail", "[RegionalExtremaEngine]")
{
  // Alternating XY positions make consecutive corridor voxels touch only through a vertex across Z. Two core planes
  // over seven planes force 2 + 2 + 2 + 1, so full-connectivity propagation crosses every slab halo and reaches the
  // short tail. Face connectivity must leave the disconnected corridor voxels unchanged.
  constexpr usize k_DimX = 4;
  constexpr usize k_DimY = 4;
  constexpr usize k_DimZ = 7;
  constexpr usize k_PlaneSize = k_DimX * k_DimY;
  constexpr usize k_SlabValueBudget = 4 * k_PlaneSize; // two core planes plus the two halo planes

  for(const RegionalExtremaOp op : {RegionalExtremaOp::Maxima, RegionalExtremaOp::Minima})
  {
    const int16 fieldValue = op == RegionalExtremaOp::Maxima ? int16{0} : int16{100};
    const int16 corridorValue = int16{50};
    const int16 seedValue = op == RegionalExtremaOp::Maxima ? int16{100} : int16{0};
    const int16 markerValue = op == RegionalExtremaOp::Maxima ? std::numeric_limits<int16>::lowest() : std::numeric_limits<int16>::max();
    std::vector<int16> input(k_DimX * k_DimY * k_DimZ, fieldValue);
    input[FlatIndex(1, 1, 0, k_DimX, k_DimY)] = seedValue;
    for(usize z = 1; z < k_DimZ; ++z)
    {
      const usize xy = z % 2 == 0 ? 1 : 2;
      input[FlatIndex(xy, xy, z, k_DimX, k_DimY)] = corridorValue;
    }

    const usize tailXY = (k_DimZ - 1) % 2 == 0 ? 1 : 2;
    const usize tailIndex = FlatIndex(tailXY, tailXY, k_DimZ - 1, k_DimX, k_DimY);
    const std::vector<int16> fullOracle = RegionalExtremaOracle(input, k_DimX, k_DimY, k_DimZ, op, true);
    const std::vector<int16> fullFlood = RunEngine<RegionalExtremaFlood, int16>(input, k_DimX, k_DimY, k_DimZ, op, true);
    const std::vector<int16> fullSweep = RunSweep(input, k_DimX, k_DimY, k_DimZ, op, true, k_SlabValueBudget);
    const std::vector<int16> faceOracle = RegionalExtremaOracle(input, k_DimX, k_DimY, k_DimZ, op, false);
    const std::vector<int16> faceFlood = RunEngine<RegionalExtremaFlood, int16>(input, k_DimX, k_DimY, k_DimZ, op, false);
    const std::vector<int16> faceSweep = RunSweep(input, k_DimX, k_DimY, k_DimZ, op, false, k_SlabValueBudget);

    INFO("op=" << static_cast<int>(op));
    REQUIRE(fullSweep == fullOracle);
    REQUIRE(fullSweep == fullFlood);
    REQUIRE(fullSweep[tailIndex] == markerValue);
    REQUIRE(faceSweep == faceOracle);
    REQUIRE(faceSweep == faceFlood);
    REQUIRE(faceSweep[tailIndex] == corridorValue);
  }
}

TEST_CASE("NXImageProcessing::RegionalExtremaEngine: bounded 2D row blocks and overwide tiles preserve the exact fixpoint", "[RegionalExtremaEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 9;
  constexpr usize k_DimZ = 1;
  constexpr usize k_FullWidthBudget = 4 * k_DimX; // two core rows plus two halo rows
  constexpr usize k_OverwideBudget = 18;        // three rows of four core columns plus two X halos
  const std::vector<int16> input = MakePlateauPattern<int16>(k_DimX, k_DimY, k_DimZ);

  for(const RegionalExtremaOp op : {RegionalExtremaOp::Maxima, RegionalExtremaOp::Minima})
  {
    for(const bool fullyConnected : {false, true})
    {
      const std::vector<int16> expected = RegionalExtremaOracle(input, k_DimX, k_DimY, k_DimZ, op, fullyConnected);
      const std::vector<int16> blocked = RunSweep(input, k_DimX, k_DimY, k_DimZ, op, fullyConnected, k_FullWidthBudget);
      const std::vector<int16> tiled = RunSweep(input, k_DimX, k_DimY, k_DimZ, op, fullyConnected, k_OverwideBudget);
      INFO("op=" << static_cast<int>(op) << " fullyConnected=" << fullyConnected);
      REQUIRE(blocked == expected);
      REQUIRE(tiled == expected);
    }
  }
}

TEST_CASE("NXImageProcessing::RegionalExtremaEngine: sweep validates dimensions and store sizes", "[RegionalExtremaEngine]")
{
  DataStore<int8> inputStore(ShapeType{1}, ShapeType{1}, int8{7});
  DataStore<int8> outputStore(ShapeType{1}, ShapeType{1}, int8{42});
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());

  SECTION("dimension exceeds the signed traversal range")
  {
    const Result<> result = RegionalExtremaSweep<int8>{inputStore, outputStore, SizeVec3{k_Int64Max + 1, 1, 1}, RegionalExtremaOp::Maxima, false, shouldCancel, messageHandler}();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8660);
  }
  SECTION("XY product overflow")
  {
    const Result<> result = RegionalExtremaSweep<int8>{inputStore, outputStore, SizeVec3{k_Int64Max, 3, 1}, RegionalExtremaOp::Maxima, false, shouldCancel, messageHandler}();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8661);
  }
  SECTION("XYZ product overflow")
  {
    const Result<> result = RegionalExtremaSweep<int8>{inputStore, outputStore, SizeVec3{k_Int64Max, 1, 3}, RegionalExtremaOp::Maxima, false, shouldCancel, messageHandler}();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8662);
  }
  SECTION("store size mismatch")
  {
    const Result<> result = RegionalExtremaSweep<int8>{inputStore, outputStore, SizeVec3{2, 2, 2}, RegionalExtremaOp::Maxima, false, shouldCancel, messageHandler}();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8663);
  }
  SECTION("zero dimension is an explicit no-op")
  {
    const Result<> result = RegionalExtremaSweep<int8>{inputStore, outputStore, SizeVec3{0, 2, 3}, RegionalExtremaOp::Maxima, false, shouldCancel, messageHandler}();
    REQUIRE(result.valid());
    REQUIRE(outputStore.getValue(0) == int8{42});
  }
  SECTION("pre-cancel leaves the output untouched")
  {
    shouldCancel = true;
    const Result<> result = RegionalExtremaSweep<int8>{inputStore, outputStore, SizeVec3{1, 1, 1}, RegionalExtremaOp::Maxima, false, shouldCancel, messageHandler}();
    REQUIRE(result.valid());
    REQUIRE(outputStore.getValue(0) == int8{42});
  }
}

// -----------------------------------------------------------------------------
// Cross-validation: the in-core Flood, the out-of-core Sweep, and the independent Jacobi oracle must AGREE
// byte-for-byte for both ops and both connectivities, across element types and a battery of shapes/patterns.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("NXImageProcessing::RegionalExtremaEngine: Flood == Sweep == oracle", "[RegionalExtremaEngine]", uint8, int16, int32, float32)
{
  using T = TestType;

  SECTION("multi-plateau 3D (5x6x7)")
  {
    CheckRegionalExtrema<T>(MakePlateauPattern<T>(5, 6, 7), 5, 6, 7);
  }
  SECTION("multi-plateau 2D (9x8x1)")
  {
    CheckRegionalExtrema<T>(MakePlateauPattern<T>(9, 8, 1), 9, 8, 1);
  }
  SECTION("small 3x3x3")
  {
    CheckRegionalExtrema<T>(MakePlateauPattern<T>(3, 3, 3), 3, 3, 3);
  }
  SECTION("needle 1x7x1")
  {
    CheckRegionalExtrema<T>(MakePlateauPattern<T>(1, 7, 1), 1, 7, 1);
  }
  SECTION("flat image (all one value) -> unchanged")
  {
    std::vector<T> flatImg(4 * 5 * 3, static_cast<T>(42));
    CheckRegionalExtrema<T>(flatImg, 4, 5, 3);
    // The valued result of a flat image is the image unchanged (no strictly-more-extreme neighbor anywhere).
    for(RegionalExtremaOp op : {RegionalExtremaOp::Maxima, RegionalExtremaOp::Minima})
    {
      const std::vector<T> flood = RunEngine<RegionalExtremaFlood, T>(flatImg, 4, 5, 3, op, /*fullyConnected=*/false);
      REQUIRE(flood == flatImg);
    }
  }
  SECTION("monotone ramp along X -> only the extreme end plateau kept")
  {
    constexpr usize k_Dx = 6;
    constexpr usize k_Dy = 3;
    constexpr usize k_Dz = 1;
    std::vector<T> ramp(k_Dx * k_Dy * k_Dz);
    for(usize z = 0; z < k_Dz; ++z)
    {
      for(usize y = 0; y < k_Dy; ++y)
      {
        for(usize x = 0; x < k_Dx; ++x)
        {
          ramp[FlatIndex(x, y, z, k_Dx, k_Dy)] = static_cast<T>(x * 10); // strictly increasing in x
        }
      }
    }
    CheckRegionalExtrema<T>(ramp, k_Dx, k_Dy, k_Dz);
    // Maxima: only the x==DX-1 column (global max) is a regional maximum; every other pixel has a higher x+1
    // neighbor -> marker value.
    const std::vector<T> flood = RunEngine<RegionalExtremaFlood, T>(ramp, k_Dx, k_Dy, k_Dz, RegionalExtremaOp::Maxima, /*fullyConnected=*/false);
    const T markerMax = std::numeric_limits<T>::lowest();
    for(usize y = 0; y < k_Dy; ++y)
    {
      REQUIRE(flood[FlatIndex(k_Dx - 1, y, 0, k_Dx, k_Dy)] == static_cast<T>((k_Dx - 1) * 10)); // kept
      REQUIRE(flood[FlatIndex(0, y, 0, k_Dx, k_Dy)] == markerMax);                          // not a maximum
    }
  }
  SECTION("marker-value-in-input edge case (a voxel at the type extreme)")
  {
    // Put the maxima marker value (type lowest) and the minima marker value (type max) into the input; Flood,
    // Sweep, and the oracle must still agree (documents ITK's visited-test behavior on marker-valued inputs).
    std::vector<T> img = MakePlateauPattern<T>(4, 4, 2);
    img[FlatIndex(1, 1, 0, 4, 4)] = std::numeric_limits<T>::lowest();
    img[FlatIndex(2, 2, 1, 4, 4)] = std::numeric_limits<T>::max();
    CheckRegionalExtrema<T>(img, 4, 4, 2);
  }

  SECTION("long serpentine equal-value corridor -> multi-pass in-plane sweep convergence")
  {
    // A single thin equal-value (50) corridor winding boustrophedon across a 2D plane, seeded at one end by a
    // strictly-higher pixel (100). For the maxima op the whole corridor is a non-maximum, so the mark must
    // propagate along its entire winding length -- the case the out-of-core sweep's reverse-in-plane traversal
    // exists to converge quickly on. Flood == Sweep == oracle confirms the streamed sweep reaches the exact
    // fixpoint regardless of how many passes it takes (this would be O(corridor length) passes without the
    // bidirectional in-plane traversal, and terminates either way).
    constexpr usize k_Dx = 20;
    constexpr usize k_Dy = 11;
    constexpr usize k_Dz = 1;
    std::vector<T> img(k_Dx * k_Dy * k_Dz, static_cast<T>(0));
    for(usize y = 0; y < k_Dy; ++y)
    {
      if(y % 2 == 0)
      {
        for(usize x = 0; x < k_Dx; ++x)
        {
          img[FlatIndex(x, y, 0, k_Dx, k_Dy)] = static_cast<T>(50);
        }
      }
      else
      {
        // Odd rows carry a single connector cell (alternating end) so the even-row bars link into one snake.
        const usize connectX = ((y / 2) % 2 == 0) ? (k_Dx - 1) : 0;
        img[FlatIndex(connectX, y, 0, k_Dx, k_Dy)] = static_cast<T>(50);
      }
    }
    img[FlatIndex(0, 0, 0, k_Dx, k_Dy)] = static_cast<T>(100); // seed one end of the corridor with a higher value
    CheckRegionalExtrema<T>(img, k_Dx, k_Dy, k_Dz);
  }
}

// -----------------------------------------------------------------------------
// Hand-computed: a single central peak plateau on a lower field is kept at its value; everything else (lower) is a
// non-maximum -> marker value. Dual for a central pit under Minima.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::RegionalExtremaEngine: hand-computed central extremum", "[RegionalExtremaEngine]")
{
  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  const int32 field = 10;
  const int32 peak = 50;

  std::vector<int32> img(k_Dx * k_Dy * k_Dz, field);
  // 2x2 central peak plateau (a genuine regional maximum: strictly higher than all its neighbors).
  img[FlatIndex(2, 2, 0, k_Dx, k_Dy)] = peak;
  img[FlatIndex(3, 2, 0, k_Dx, k_Dy)] = peak;
  img[FlatIndex(2, 3, 0, k_Dx, k_Dy)] = peak;
  img[FlatIndex(3, 3, 0, k_Dx, k_Dy)] = peak;

  const std::vector<int32> maxima = RunEngine<RegionalExtremaFlood, int32>(img, k_Dx, k_Dy, k_Dz, RegionalExtremaOp::Maxima, /*fullyConnected=*/false);
  const int32 markerMax = std::numeric_limits<int32>::lowest();
  REQUIRE(maxima[FlatIndex(2, 2, 0, k_Dx, k_Dy)] == peak);      // peak plateau kept
  REQUIRE(maxima[FlatIndex(3, 3, 0, k_Dx, k_Dy)] == peak);      // peak plateau kept
  REQUIRE(maxima[FlatIndex(0, 0, 0, k_Dx, k_Dy)] == markerMax); // field is not a maximum (has a higher neighbor path)

  // The same image under Minima: the field (value 10) is the regional minimum (kept); the peak is a non-minimum.
  const std::vector<int32> minima = RunEngine<RegionalExtremaFlood, int32>(img, k_Dx, k_Dy, k_Dz, RegionalExtremaOp::Minima, /*fullyConnected=*/false);
  const int32 markerMin = std::numeric_limits<int32>::max();
  REQUIRE(minima[FlatIndex(0, 0, 0, k_Dx, k_Dy)] == field);     // field plateau is the regional minimum -> kept
  REQUIRE(minima[FlatIndex(2, 2, 0, k_Dx, k_Dy)] == markerMin); // the peak is not a minimum
}
