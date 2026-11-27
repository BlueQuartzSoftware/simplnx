#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
class TestSweepTemporaryRecordStore : public ITemporaryRecordStore
{
public:
  TestSweepTemporaryRecordStore(uint64 recordSize, uint64 recordCount, uint64 maxRecordsPerBatch)
  : m_RecordSize(recordSize)
  , m_RecordCount(recordCount)
  , m_MaxRecordsPerBatch(maxRecordsPerBatch)
  , m_Bytes(static_cast<usize>(recordSize * recordCount))
  {
  }

  uint64 recordSize() const override
  {
    return m_RecordSize;
  }
  uint64 recordCount() const override
  {
    return m_RecordCount;
  }
  uint64 maxRecordsPerBatch() const override
  {
    return m_MaxRecordsPerBatch;
  }
  bool isReadOnly() const override
  {
    return false;
  }
  Result<uint64> read(uint64 recordOffset, uint64 requestedRecordCount, nonstd::span<std::byte> records, const std::atomic_bool& /*shouldCancel*/) const override
  {
    ++read_calls;
    if(fail_read)
    {
      return MakeErrorResult<uint64>(-7680, "Injected sweep scratch read failure");
    }
    const uint64 returnedRecords = short_read && requestedRecordCount > 0 ? requestedRecordCount - 1 : requestedRecordCount;
    std::memcpy(records.data(), m_Bytes.data() + static_cast<usize>(recordOffset * m_RecordSize), static_cast<usize>(returnedRecords * m_RecordSize));
    return {returnedRecords};
  }
  Result<> write(uint64 recordOffset, uint64 recordCount, nonstd::span<const std::byte> records, const std::atomic_bool& /*shouldCancel*/) override
  {
    ++write_calls;
    if(fail_write)
    {
      return MakeErrorResult(-7681, "Injected sweep scratch write failure");
    }
    std::memcpy(m_Bytes.data() + static_cast<usize>(recordOffset * m_RecordSize), records.data(), static_cast<usize>(recordCount * m_RecordSize));
    return {};
  }
  Result<> fill(uint64 /*recordOffset*/, uint64 /*recordCount*/, nonstd::span<const std::byte> /*record*/, const std::atomic_bool& /*shouldCancel*/) override
  {
    return MakeErrorResult(-7682, "Unexpected sweep scratch fill");
  }
  Result<> resize(uint64 /*recordCount*/, const std::atomic_bool& /*shouldCancel*/) override
  {
    return MakeErrorResult(-7683, "Unexpected sweep scratch resize");
  }

  mutable usize read_calls = 0;
  usize write_calls = 0;
  bool fail_read = false;
  bool fail_write = false;
  bool short_read = false;

private:
  uint64 m_RecordSize = 0;
  uint64 m_RecordCount = 0;
  uint64 m_MaxRecordsPerBatch = 0;
  std::vector<std::byte> m_Bytes;
};

template <class T>
class CountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_ReadCalls++;
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_WriteCalls++;
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize readCallCount() const
  {
    return m_ReadCalls;
  }

  usize writeCallCount() const
  {
    return m_WriteCalls;
  }

private:
  mutable usize m_ReadCalls = 0;
  usize m_WriteCalls = 0;
};

// Non-cubic dims so a transposed/swapped-axis or bad-stride bug cannot pass: X even, Y odd, Z even.
constexpr usize k_X = 4;
constexpr usize k_Y = 5;
constexpr usize k_Z = 6;

usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return z * (dimY * dimX) + y * dimX + x;
}

// Local saturating double->T cast (independent of the engine helpers).
template <class T>
T SatCast(double v)
{
  const double lo = static_cast<double>(std::numeric_limits<T>::lowest());
  const double hi = static_cast<double>(std::numeric_limits<T>::max());
  if(v < lo)
  {
    return std::numeric_limits<T>::lowest();
  }
  if(v > hi)
  {
    return std::numeric_limits<T>::max();
  }
  return static_cast<T>(v);
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

// INDEPENDENT oracle: Jacobi (read-a-snapshot) iterate of the full-neighborhood geodesic step to stability.
// Structurally independent of both engine paths (no scan-order, no previous/later split, no FIFO), so a
// slab/stride/scan/queue bug in the engine cannot hide behind it. Converges to the unique fixpoint (monotone).
template <class T>
std::vector<T> ReconstructOracle(const std::vector<T>& marker, const std::vector<T>& mask, usize dimX, usize dimY, usize dimZ, ReconstructOp op, bool fullyConnected)
{
  const bool dilation = (op == ReconstructOp::Dilation);
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

  std::vector<T> work = marker;
  bool changed = true;
  while(changed)
  {
    changed = false;
    const std::vector<T> prev = work; // snapshot (Jacobi)
    for(int64 z = 0; z < nZ; ++z)
    {
      for(int64 y = 0; y < nY; ++y)
      {
        for(int64 x = 0; x < nX; ++x)
        {
          const usize p = FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dimX, dimY);
          T v = prev[p];
          for(const auto& o : offs)
          {
            const int64 nx = x + o[0];
            const int64 ny = y + o[1];
            const int64 nz = z + o[2];
            if(nx < 0 || nx >= nX || ny < 0 || ny >= nY || nz < 0 || nz >= nZ)
            {
              continue;
            }
            const T nv = prev[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)];
            v = dilation ? std::max(v, nv) : std::min(v, nv);
          }
          const T clamped = dilation ? std::min(v, mask[p]) : std::max(v, mask[p]);
          if(clamped != prev[p])
          {
            changed = true;
          }
          work[p] = clamped;
        }
      }
    }
  }
  return work;
}

// Independent reference for exactly one sequential forward-raster + reverse-anti-raster sweep pair. Unlike the
// fixpoint oracle above, this exposes scan-order changes that convergence could otherwise hide.
template <class T>
std::vector<T> ReconstructSweepPairOracle(const std::vector<T>& marker, const std::vector<T>& mask, usize dimX, usize dimY, usize dimZ, ReconstructOp op, bool fullyConnected)
{
  const bool dilation = (op == ReconstructOp::Dilation);
  std::vector<std::array<int, 3>> previous;
  std::vector<std::array<int, 3>> later;
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
        auto& half = ((dz < 0) || (dz == 0 && dy < 0) || (dz == 0 && dy == 0 && dx < 0)) ? previous : later;
        half.push_back({dx, dy, dz});
      }
    }
  }

  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);
  std::vector<T> work = marker;
  auto sweep = [&](bool forward, const std::vector<std::array<int, 3>>& half) {
    for(usize zi = 0; zi < dimZ; ++zi)
    {
      const int64 z = forward ? static_cast<int64>(zi) : static_cast<int64>(dimZ - 1 - zi);
      for(usize yi = 0; yi < dimY; ++yi)
      {
        const int64 y = forward ? static_cast<int64>(yi) : static_cast<int64>(dimY - 1 - yi);
        for(usize xi = 0; xi < dimX; ++xi)
        {
          const int64 x = forward ? static_cast<int64>(xi) : static_cast<int64>(dimX - 1 - xi);
          const usize p = FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dimX, dimY);
          T value = work[p];
          for(const auto& offset : half)
          {
            const int64 nx = x + offset[0];
            const int64 ny = y + offset[1];
            const int64 nz = z + offset[2];
            if(nx < 0 || nx >= nX || ny < 0 || ny >= nY || nz < 0 || nz >= nZ)
            {
              continue;
            }
            const T neighbor = work[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)];
            value = dilation ? std::max(value, neighbor) : std::min(value, neighbor);
          }
          work[p] = dilation ? std::min(value, mask[p]) : std::max(value, mask[p]);
        }
      }
    }
  };
  sweep(true, previous);
  sweep(false, later);
  return work;
}

template <class T>
std::vector<T> RunVincent(const std::vector<T>& marker, const std::vector<T>& mask, usize dimX, usize dimY, usize dimZ, ReconstructOp op, bool fullyConnected,
                          usize frontierCapacity = std::numeric_limits<usize>::max())
{
  DataStore<T> markerStore = MakeStore(marker, dimX, dimY, dimZ);
  DataStore<T> maskStore = MakeStore(mask, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> r = ReconstructVincent<T>{markerStore, maskStore, outStore, SizeVec3Type{dimX, dimY, dimZ}, op, fullyConnected, shouldCancel, messageHandler, {}, frontierCapacity}();
  REQUIRE(r.valid());
  std::vector<T> out(marker.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

template <class T>
std::vector<T> RunSweep(const std::vector<T>& marker, const std::vector<T>& mask, usize dimX, usize dimY, usize dimZ, ReconstructOp op, bool fullyConnected,
                        usize maxSlabValues = ReconstructSweep<T>::k_DefaultSlabValues, usize maxSweepPairs = std::numeric_limits<usize>::max())
{
  DataStore<T> markerStore = MakeStore(marker, dimX, dimY, dimZ);
  DataStore<T> maskStore = MakeStore(mask, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> r = ReconstructSweep<T>{markerStore, maskStore, outStore, SizeVec3Type{dimX, dimY, dimZ}, op, fullyConnected, shouldCancel, messageHandler, maxSlabValues, maxSweepPairs}();
  REQUIRE(r.valid());
  std::vector<T> out(marker.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// Deterministic non-trivial mask (gradient + coarse blocks + pseudo-noise), values in [0, 199].
template <class T>
std::vector<T> MakeMask(usize dimX, usize dimY, usize dimZ)
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

// A dilation-valid marker (marker <= mask): saturating (mask - k).
template <class T>
std::vector<T> MarkerBelow(const std::vector<T>& mask, double k)
{
  std::vector<T> m(mask.size());
  for(usize i = 0; i < mask.size(); ++i)
  {
    m[i] = SatCast<T>(static_cast<double>(mask[i]) - k);
  }
  return m;
}

// An erosion-valid marker (marker >= mask): saturating (mask + k).
template <class T>
std::vector<T> MarkerAbove(const std::vector<T>& mask, double k)
{
  std::vector<T> m(mask.size());
  for(usize i = 0; i < mask.size(); ++i)
  {
    m[i] = SatCast<T>(static_cast<double>(mask[i]) + k);
  }
  return m;
}

// A dilation SEED marker: type lowest everywhere (<= mask) except a few points set to the mask value, so
// reconstruction must FLOOD from the seeds. Strong propagation test.
template <class T>
std::vector<T> SeedMarkerBelow(const std::vector<T>& mask, usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> m(mask.size(), std::numeric_limits<T>::lowest());
  const std::array<std::array<usize, 3>, 3> seeds = {{{0, 0, 0}, {dimX / 2, dimY / 2, dimZ / 2}, {dimX - 1, dimY - 1, dimZ - 1}}};
  for(const auto& s : seeds)
  {
    const usize p = FlatIndex(s[0], s[1], s[2], dimX, dimY);
    m[p] = mask[p];
  }
  return m;
}

// An erosion SEED marker: type max everywhere (>= mask) except a few points at the mask value.
template <class T>
std::vector<T> SeedMarkerAbove(const std::vector<T>& mask, usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> m(mask.size(), std::numeric_limits<T>::max());
  const std::array<std::array<usize, 3>, 3> seeds = {{{0, 0, 0}, {dimX / 2, dimY / 2, dimZ / 2}, {dimX - 1, dimY - 1, dimZ - 1}}};
  for(const auto& s : seeds)
  {
    const usize p = FlatIndex(s[0], s[1], s[2], dimX, dimY);
    m[p] = mask[p];
  }
  return m;
}

// For every connectivity x op x marker: Vincent == Oracle, Sweep == Oracle, and Sweep == Vincent byte-for-byte.
template <class T>
void CheckReconstruction(usize dimX, usize dimY, usize dimZ)
{
  const std::vector<T> mask = MakeMask<T>(dimX, dimY, dimZ);

  for(bool fullyConnected : {false, true})
  {
    // Dilation markers.
    std::vector<std::pair<const char*, std::vector<T>>> dilMarkers;
    dilMarkers.emplace_back("mask-5", MarkerBelow<T>(mask, 5.0));
    dilMarkers.emplace_back("mask-30", MarkerBelow<T>(mask, 30.0));
    dilMarkers.emplace_back("seed", SeedMarkerBelow<T>(mask, dimX, dimY, dimZ));
    for(const auto& [label, marker] : dilMarkers)
    {
      const std::vector<T> oracle = ReconstructOracle<T>(marker, mask, dimX, dimY, dimZ, ReconstructOp::Dilation, fullyConnected);
      const std::vector<T> vincent = RunVincent<T>(marker, mask, dimX, dimY, dimZ, ReconstructOp::Dilation, fullyConnected);
      const std::vector<T> sweep = RunSweep<T>(marker, mask, dimX, dimY, dimZ, ReconstructOp::Dilation, fullyConnected);
      for(usize i = 0; i < mask.size(); ++i)
      {
        INFO("dilation fullyConnected=" << fullyConnected << " marker=" << label << " index=" << i);
        REQUIRE(vincent[i] == oracle[i]);
        REQUIRE(sweep[i] == oracle[i]);
        REQUIRE(sweep[i] == vincent[i]);
      }
    }

    // Erosion markers.
    std::vector<std::pair<const char*, std::vector<T>>> eroMarkers;
    eroMarkers.emplace_back("mask+5", MarkerAbove<T>(mask, 5.0));
    eroMarkers.emplace_back("mask+30", MarkerAbove<T>(mask, 30.0));
    eroMarkers.emplace_back("seed", SeedMarkerAbove<T>(mask, dimX, dimY, dimZ));
    for(const auto& [label, marker] : eroMarkers)
    {
      const std::vector<T> oracle = ReconstructOracle<T>(marker, mask, dimX, dimY, dimZ, ReconstructOp::Erosion, fullyConnected);
      const std::vector<T> vincent = RunVincent<T>(marker, mask, dimX, dimY, dimZ, ReconstructOp::Erosion, fullyConnected);
      const std::vector<T> sweep = RunSweep<T>(marker, mask, dimX, dimY, dimZ, ReconstructOp::Erosion, fullyConnected);
      for(usize i = 0; i < mask.size(); ++i)
      {
        INFO("erosion fullyConnected=" << fullyConnected << " marker=" << label << " index=" << i);
        REQUIRE(vincent[i] == oracle[i]);
        REQUIRE(sweep[i] == oracle[i]);
        REQUIRE(sweep[i] == vincent[i]);
      }
    }
  }
}
} // namespace

TEST_CASE("NXImageProcessing::SweepTemporaryStore: performs typed bulk I/O and rejects backend failures", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  std::atomic_bool shouldCancel{false};
  auto backend = std::make_unique<TestSweepTemporaryRecordStore>(sizeof(float32), 12, 4);
  auto* backendPtr = backend.get();
  ImageProcessing::detail::SweepTemporaryStore<float32> store(std::move(backend), shouldCancel, "reconstruction");
  REQUIRE(store.getSize() == 12);

  const std::array<float32, 6> written = {1.25f, -2.5f, 3.75f, 9.0f, -11.0f, 13.5f};
  REQUIRE(store.copyFromBuffer(3, nonstd::span<const float32>(written.data(), written.size())).valid());
  std::array<float32, 6> read{};
  REQUIRE(store.copyIntoBuffer(3, nonstd::span<float32>(read.data(), read.size())).valid());
  REQUIRE(read == written);
  REQUIRE(backendPtr->write_calls == 2);
  REQUIRE(backendPtr->read_calls == 2);

  REQUIRE(store.copyIntoBuffer(10, nonstd::span<float32>(read.data(), 3)).invalid());
  REQUIRE(backendPtr->read_calls == 2);

  backendPtr->fail_write = true;
  REQUIRE(store.copyFromBuffer(3, nonstd::span<const float32>(written.data(), written.size())).invalid());
  backendPtr->fail_write = false;
  backendPtr->fail_read = true;
  REQUIRE(store.copyIntoBuffer(3, nonstd::span<float32>(read.data(), read.size())).invalid());
  backendPtr->fail_read = false;
  backendPtr->short_read = true;
  REQUIRE(store.copyIntoBuffer(3, nonstd::span<float32>(read.data(), read.size())).invalid());
}

TEST_CASE("NXImageProcessing::Sweep2DPlan: bounds full-width and overwide sweep buffers", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  using ImageProcessing::detail::CreateSweep2DPlan;

  SECTION("reconstruction full-width row blocks")
  {
    const auto result = CreateSweep2DPlan(/*dimX=*/7, /*dimY=*/11, /*maxBufferValues=*/30, /*fullWidthHaloRows=*/1, /*tiledRows=*/1);
    REQUIRE(result.valid());
    const auto& plan = result.value();
    REQUIRE(plan.full_width);
    REQUIRE(plan.core_rows == 3);
    REQUIRE(plan.core_columns == 7);
    REQUIRE((plan.core_rows + 1) * 7 <= 30);
  }

  SECTION("reconstruction overwide X tiles")
  {
    const auto result = CreateSweep2DPlan(/*dimX=*/31, /*dimY=*/4, /*maxBufferValues=*/12, /*fullWidthHaloRows=*/1, /*tiledRows=*/1);
    REQUIRE(result.valid());
    const auto& plan = result.value();
    REQUIRE_FALSE(plan.full_width);
    REQUIRE(plan.core_rows == 1);
    REQUIRE(plan.core_columns == 10);
    REQUIRE(plan.core_columns + 2 <= 12);
  }

  SECTION("regional-extrema overwide X tiles reserve three halo rows")
  {
    const auto result = CreateSweep2DPlan(/*dimX=*/31, /*dimY=*/4, /*maxBufferValues=*/18, /*fullWidthHaloRows=*/2, /*tiledRows=*/3);
    REQUIRE(result.valid());
    const auto& plan = result.value();
    REQUIRE_FALSE(plan.full_width);
    REQUIRE(plan.core_rows == 1);
    REQUIRE(plan.core_columns == 4);
    REQUIRE(3 * (plan.core_columns + 2) <= 18);
  }

  SECTION("insufficient tile budget is rejected")
  {
    const auto result = CreateSweep2DPlan(/*dimX=*/31, /*dimY=*/4, /*maxBufferValues=*/8, /*fullWidthHaloRows=*/2, /*tiledRows=*/3);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8684);
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: total working-memory grants scale checked sweep plans without changing output",
          "[NXImageProcessing][MorphologicalReconstructionEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr usize k_PlaneValues = k_DimX * k_DimY;
  constexpr usize k_SmallerTargetBytes = (2 * 2 * k_PlaneValues + k_PlaneValues) * sizeof(int16);
  constexpr usize k_LargerTargetBytes = (2 * 4 * k_PlaneValues + k_PlaneValues) * sizeof(int16);

  auto smallerResult = ImageProcessing::detail::CreateReconstructionWorkingMemoryPlan<int16>(SizeVec3Type{k_DimX, k_DimY, k_DimZ}, k_SmallerTargetBytes);
  REQUIRE(smallerResult.valid());
  const auto& smaller = smallerResult.value();
  REQUIRE(smaller.max_slab_values == 2 * k_PlaneValues);
  REQUIRE(smaller.resident_bytes <= k_SmallerTargetBytes);
  REQUIRE_FALSE(smaller.use_resident_full_sweep);

  auto largerResult = ImageProcessing::detail::CreateReconstructionWorkingMemoryPlan<int16>(SizeVec3Type{k_DimX, k_DimY, k_DimZ}, k_LargerTargetBytes);
  REQUIRE(largerResult.valid());
  const auto& larger = largerResult.value();
  REQUIRE(larger.max_slab_values == 4 * k_PlaneValues);
  REQUIRE(larger.max_slab_values > smaller.max_slab_values);
  REQUIRE(larger.resident_bytes <= k_LargerTargetBytes);
  REQUIRE_FALSE(larger.use_resident_full_sweep);

  auto usefulResult = ImageProcessing::detail::CalculateReconstructionUsefulWorkingMemoryBytes<int16>(SizeVec3Type{k_DimX, k_DimY, k_DimZ});
  REQUIRE(usefulResult.valid());
  REQUIRE(usefulResult.value() == (2 * k_DimX * k_DimY * k_DimZ + k_PlaneValues) * sizeof(int16));
  auto fullResult = ImageProcessing::detail::CreateReconstructionWorkingMemoryPlan<int16>(SizeVec3Type{k_DimX, k_DimY, k_DimZ}, usefulResult.value());
  REQUIRE(fullResult.valid());
  REQUIRE(fullResult.value().max_slab_values == k_DimX * k_DimY * k_DimZ);
  REQUIRE(fullResult.value().resident_bytes <= usefulResult.value());
  REQUIRE(fullResult.value().use_resident_full_sweep);

  constexpr usize k_PrefixDimX = 1024;
  constexpr usize k_PrefixDimY = 1024;
  constexpr usize k_PrefixDimZ = 256;
  constexpr usize k_PrefixPlaneValues = k_PrefixDimX * k_PrefixDimY;
  auto prefixPlanResult = ImageProcessing::detail::CreateReconstructionPersistentPrefixPlan<int16>(SizeVec3Type{k_PrefixDimX, k_PrefixDimY, k_PrefixDimZ}, /*maxSlabValues=*/127 * k_PrefixPlaneValues);
  REQUIRE(prefixPlanResult.valid());
  REQUIRE(prefixPlanResult.value().resident_planes == 252);
  REQUIRE(prefixPlanResult.value().resident_values == 252 * k_PrefixPlaneValues);
  REQUIRE(prefixPlanResult.value().resident_bytes == 255 * k_PrefixPlaneValues * sizeof(int16));
  REQUIRE(prefixPlanResult.value().resident_bytes < 512ULL * 1024ULL * 1024ULL);

  constexpr usize k_ScaledPlaneValues = 7 * 6;
  auto smallerPrefixResult = ImageProcessing::detail::CreateReconstructionPersistentPrefixPlan<int16>(SizeVec3Type{7, 6, 20}, /*maxSlabValues=*/3 * k_ScaledPlaneValues);
  auto largerPrefixResult = ImageProcessing::detail::CreateReconstructionPersistentPrefixPlan<int16>(SizeVec3Type{7, 6, 20}, /*maxSlabValues=*/6 * k_ScaledPlaneValues);
  REQUIRE(smallerPrefixResult.valid());
  REQUIRE(largerPrefixResult.valid());
  REQUIRE(smallerPrefixResult.value().resident_planes == 4);
  REQUIRE(largerPrefixResult.value().resident_planes == 10);
  REQUIRE(smallerPrefixResult.value().resident_bytes == 7 * k_ScaledPlaneValues * sizeof(int16));
  REQUIRE(largerPrefixResult.value().resident_bytes == 13 * k_ScaledPlaneValues * sizeof(int16));
  REQUIRE(largerPrefixResult.value().resident_planes > smallerPrefixResult.value().resident_planes);

  auto insufficientPrefixResult = ImageProcessing::detail::CreateReconstructionPersistentPrefixPlan<int16>(SizeVec3Type{7, 6, 20}, /*maxSlabValues=*/k_ScaledPlaneValues);
  REQUIRE(insufficientPrefixResult.valid());
  REQUIRE(insufficientPrefixResult.value().resident_planes == 0);

  auto minimumResult = ImageProcessing::detail::CalculateReconstructionMinimumWorkingMemoryBytes<int16>(SizeVec3Type{k_DimX, k_DimY, k_DimZ});
  REQUIRE(minimumResult.valid());
  REQUIRE(minimumResult.value() == 3 * k_PlaneValues * sizeof(int16));

  const std::vector<int16> mask = MakeMask<int16>(k_DimX, k_DimY, k_DimZ);
  const std::vector<int16> marker = SeedMarkerBelow<int16>(mask, k_DimX, k_DimY, k_DimZ);
  const std::vector<int16> expected = ReconstructOracle<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, ReconstructOp::Dilation, true);
  REQUIRE(RunSweep<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, ReconstructOp::Dilation, true, smaller.max_slab_values) == expected);
  REQUIRE(RunSweep<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, ReconstructOp::Dilation, true, larger.max_slab_values) == expected);

  constexpr usize k_2DDimX = 7;
  constexpr usize k_2DDimY = 9;
  constexpr usize k_2DValues = k_2DDimX * k_2DDimY;
  constexpr usize k_2DTargetBytes = (k_2DValues + 2 * 3 * k_2DDimX) * sizeof(int16);
  const SizeVec3Type k2DDims{k_2DDimX, k_2DDimY, 1};
  auto fixed2DResult = ImageProcessing::detail::CreateReconstructionWorkingMemoryPlan<int16>(k2DDims, k_2DTargetBytes);
  REQUIRE(fixed2DResult.valid());
  REQUIRE(fixed2DResult.value().fixed_work_values == k_2DValues);
  REQUIRE(fixed2DResult.value().fixed_buffer_values >= 2 * k_2DDimX);
  REQUIRE(fixed2DResult.value().resident_bytes <= k_2DTargetBytes);

  constexpr usize k_OverwideTargetBytes = 48;
  auto overwideResult = ImageProcessing::detail::CreateReconstructionWorkingMemoryPlan<int16>(SizeVec3Type{31, 4, 1}, k_OverwideTargetBytes);
  REQUIRE(overwideResult.valid());
  const auto& overwide = overwideResult.value();
  REQUIRE(overwide.fixed_work_values == 0);
  auto overwideSweepResult = ImageProcessing::detail::CreateSweep2DPlan(/*dimX=*/31, /*dimY=*/4, overwide.max_slab_values, /*fullWidthHaloRows=*/1, /*tiledRows=*/1);
  REQUIRE(overwideSweepResult.valid());
  REQUIRE_FALSE(overwideSweepResult.value().full_width);
  const usize actualTileBytes = (3 * overwideSweepResult.value().core_columns + 2) * sizeof(int16);
  REQUIRE(actualTileBytes <= k_OverwideTargetBytes);
  REQUIRE(overwide.resident_bytes == actualTileBytes);

  REQUIRE(ImageProcessing::detail::CreateReconstructionWorkingMemoryPlan<int16>(SizeVec3Type{k_DimX, k_DimY, k_DimZ}, /*targetBytes=*/1).invalid());

  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  auto& manager = CacheMemoryBudgetManager::Instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(256 * k_MiB);
  {
    ScopedWorkingMemoryTuningOverride override(128 * k_MiB);
    auto allocationResult = ImageProcessing::detail::ReserveReconstructionWorkingMemoryPlan<int16>(SizeVec3Type{512, 512, 128});
    REQUIRE(allocationResult.valid());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 64 * k_MiB);
    REQUIRE(allocationResult.value().plan.resident_bytes <= 64 * k_MiB);
    REQUIRE(manager.reservedWorkingMemoryBytes() == 64 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(1024 * k_MiB);
  const SizeVec3Type suppliedMarkerDims{512, 512, 128};
  auto suppliedUsefulResult = ImageProcessing::detail::CalculateReconstructionUsefulWorkingMemoryBytes<int16>(suppliedMarkerDims);
  REQUIRE(suppliedUsefulResult.valid());
  {
    auto allocationResult = ImageProcessing::detail::ReserveReconstructionWorkingMemoryPlan<int16>(suppliedMarkerDims);
    REQUIRE(allocationResult.valid());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == suppliedUsefulResult.value());
  }
  {
    auto allocationResult = ImageProcessing::detail::ReserveReconstructionWorkingMemoryPlan<int16>(suppliedMarkerDims, /*preferredNumerator=*/1, /*preferredDenominator=*/4);
    REQUIRE(allocationResult.valid());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == suppliedUsefulResult.value() / 4);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);

  const auto height3DPreference = ImageProcessing::detail::SelectReconstructionWorkingMemoryFraction(SizeVec3Type{7, 6, 5}, ReconstructionMarkerSource::MaskMinusHeight);
  REQUIRE(height3DPreference.numerator == 1);
  REQUIRE(height3DPreference.denominator == 4);
  const auto minimumBorder3DPreference = ImageProcessing::detail::SelectReconstructionWorkingMemoryFraction(SizeVec3Type{7, 6, 5}, ReconstructionMarkerSource::MaskWithMinimumInterior);
  REQUIRE(minimumBorder3DPreference.numerator == 1);
  REQUIRE(minimumBorder3DPreference.denominator == 1);
  const auto maximumBorder3DPreference = ImageProcessing::detail::SelectReconstructionWorkingMemoryFraction(SizeVec3Type{7, 6, 5}, ReconstructionMarkerSource::MaskWithMaximumInterior);
  REQUIRE(maximumBorder3DPreference.numerator == 1);
  REQUIRE(maximumBorder3DPreference.denominator == 1);
  const auto height2DPreference = ImageProcessing::detail::SelectReconstructionWorkingMemoryFraction(SizeVec3Type{7, 6, 1}, ReconstructionMarkerSource::MaskPlusHeight);
  REQUIRE(height2DPreference.numerator == 1);
  REQUIRE(height2DPreference.denominator == 1);
}

TEST_CASE("NXImageProcessing::BoundedMemorySweepStore: provides bulk-only fixed-capacity storage", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  auto storeResult = ImageProcessing::detail::BoundedMemorySweepStore<int16>::Create(/*logicalValues=*/6, /*capacityValues=*/12);
  REQUIRE(storeResult.valid());
  auto store = std::move(storeResult.value());
  REQUIRE(store->getSize() == 6);
  REQUIRE_FALSE(store->getChunkShape().has_value());

  const std::array<int16, 4> written = {4, -3, 8, 11};
  REQUIRE(store->copyFromBuffer(1, nonstd::span<const int16>(written.data(), written.size())).valid());
  std::array<int16, 4> read{};
  REQUIRE(store->copyIntoBuffer(1, nonstd::span<int16>(read.data(), read.size())).valid());
  REQUIRE(read == written);
  REQUIRE(store->copyIntoBuffer(4, nonstd::span<int16>(read.data(), 3)).invalid());
  REQUIRE(ImageProcessing::detail::BoundedMemorySweepStore<int16>::Create(/*logicalValues=*/13, /*capacityValues=*/12).invalid());
}

TEST_CASE("NXImageProcessing::BoundedReconstructionQueue: rejects overflow and reuses released entries", "[NXImageProcessing][MorphologicalReconstructionEngine][WorkingMemory]")
{
  ImageProcessing::detail::BoundedReconstructionQueue<uint64> queue(3);
  REQUIRE(queue.capacity() == 3);
  REQUIRE(queue.empty());
  REQUIRE(queue.tryPush(11));
  REQUIRE(queue.tryPush(22));
  REQUIRE(queue.tryPush(33));
  REQUIRE_FALSE(queue.tryPush(44));
  REQUIRE(queue.front() == 11);
  queue.pop();
  REQUIRE(queue.tryPush(44));
  REQUIRE(queue.front() == 22);
  queue.pop();
  REQUIRE(queue.front() == 33);
  queue.pop();
  REQUIRE(queue.front() == 44);
  queue.pop();
  REQUIRE(queue.empty());
}

TEST_CASE("NXImageProcessing::ReconstructVincent: a bounded frontier uses the resident sweep fallback", "[NXImageProcessing][MorphologicalReconstructionEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 5;
  const std::vector<int16> mask = MakeMask<int16>(k_DimX, k_DimY, k_DimZ);
  for(const bool fullyConnected : {false, true})
  {
    for(const ReconstructOp op : {ReconstructOp::Dilation, ReconstructOp::Erosion})
    {
      const std::vector<int16> marker = op == ReconstructOp::Dilation ? SeedMarkerBelow<int16>(mask, k_DimX, k_DimY, k_DimZ) : SeedMarkerAbove<int16>(mask, k_DimX, k_DimY, k_DimZ);
      const std::vector<int16> expected = ReconstructOracle<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected);
      INFO("fullyConnected=" << fullyConnected << " op=" << static_cast<int>(op));
      REQUIRE(RunVincent<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected, /*frontierCapacity=*/1) == expected);
    }
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: full-grant 3D sweeps keep adaptive state resident", "[NXImageProcessing][MorphologicalReconstructionEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 4;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 6;
  constexpr usize k_VolumeValues = k_DimX * k_DimY * k_DimZ;
  const ShapeType tupleShape{k_DimZ, k_DimY, k_DimX};
  const ShapeType componentShape{1};
  const std::vector<int16> mask = MakeMask<int16>(k_DimX, k_DimY, k_DimZ);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  for(const usize frontierCapacity : {usize{1}, k_VolumeValues})
  {
    for(const bool fullyConnected : {false, true})
    {
      for(const ReconstructOp op : {ReconstructOp::Dilation, ReconstructOp::Erosion})
      {
        const std::vector<int16> marker = op == ReconstructOp::Dilation ? SeedMarkerBelow<int16>(mask, k_DimX, k_DimY, k_DimZ) : SeedMarkerAbove<int16>(mask, k_DimX, k_DimY, k_DimZ);
        const std::vector<int16> expected = ReconstructOracle<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected);

        CountingDataStore<int16> markerStore(tupleShape, componentShape, int16{0});
        CountingDataStore<int16> maskStore(tupleShape, componentShape, int16{0});
        CountingDataStore<int16> outputStore(tupleShape, componentShape, int16{0});
        REQUIRE(markerStore.copyFromBuffer(0, nonstd::span<const int16>(marker.data(), marker.size())).valid());
        REQUIRE(maskStore.copyFromBuffer(0, nonstd::span<const int16>(mask.data(), mask.size())).valid());

        ReconstructSweep<int16, true> sweep{
            markerStore, maskStore, outputStore,     SizeVec3Type{k_DimX, k_DimY, k_DimZ}, op, fullyConnected, shouldCancel, messageHandler, k_VolumeValues, std::numeric_limits<usize>::max(), {}, 0,
            0,           true,      frontierCapacity};
        REQUIRE(sweep().valid());

        std::vector<int16> actual(k_VolumeValues);
        REQUIRE(outputStore.copyIntoBuffer(0, nonstd::span<int16>(actual.data(), actual.size())).valid());
        INFO("frontierCapacity=" << frontierCapacity << " fullyConnected=" << fullyConnected << " op=" << static_cast<int>(op));
        REQUIRE(actual == expected);
        REQUIRE(markerStore.readCallCount() == 1);
        REQUIRE(maskStore.readCallCount() == 1);
        REQUIRE(outputStore.readCallCount() == 1); // The final assertion read only; the sweep must not read the output.
        REQUIRE(outputStore.writeCallCount() == 1);
      }
    }
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: adaptive persistent prefix preserves exact streamed results", "[NXImageProcessing][MorphologicalReconstructionEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 4;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 6;
  constexpr usize k_PlaneValues = k_DimX * k_DimY;
  constexpr usize k_VolumeValues = k_PlaneValues * k_DimZ;
  const ShapeType tupleShape{k_DimZ, k_DimY, k_DimX};
  const ShapeType componentShape{1};
  const std::vector<int16> mask = MakeMask<int16>(k_DimX, k_DimY, k_DimZ);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  for(const bool fullyConnected : {false, true})
  {
    for(const ReconstructOp op : {ReconstructOp::Dilation, ReconstructOp::Erosion})
    {
      const bool dilation = op == ReconstructOp::Dilation;
      const int16 interiorValue = dilation ? std::numeric_limits<int16>::lowest() : std::numeric_limits<int16>::max();
      const ReconstructionMarkerSource source = dilation ? ReconstructionMarkerSource::MaskWithMinimumInterior : ReconstructionMarkerSource::MaskWithMaximumInterior;
      std::vector<int16> marker = mask;
      for(usize z = 0; z < k_DimZ; ++z)
      {
        for(usize y = 0; y < k_DimY; ++y)
        {
          for(usize x = 0; x < k_DimX; ++x)
          {
            if(x > 0 && x + 1 < k_DimX && y > 0 && y + 1 < k_DimY && z > 0 && z + 1 < k_DimZ)
            {
              marker[FlatIndex(x, y, z, k_DimX, k_DimY)] = interiorValue;
            }
          }
        }
      }
      const std::vector<int16> expected = ReconstructOracle<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected);

      DataStore<int16> maskStore = MakeStore(mask, k_DimX, k_DimY, k_DimZ);
      DataStore<int16> outputStore(tupleShape, componentShape, int16{0});
      const ReconstructionMarkerOptions options{.source = source, .height = 0.0};
      ReconstructSweep<int16, true> sweep{maskStore,
                                          maskStore,
                                          outputStore,
                                          SizeVec3Type{k_DimX, k_DimY, k_DimZ},
                                          op,
                                          fullyConnected,
                                          shouldCancel,
                                          messageHandler,
                                          3 * k_PlaneValues,
                                          std::numeric_limits<usize>::max(),
                                          options,
                                          0,
                                          0,
                                          false,
                                          0};
      REQUIRE(sweep().valid());

      std::vector<int16> actual(k_VolumeValues);
      REQUIRE(outputStore.copyIntoBuffer(0, nonstd::span<int16>(actual.data(), actual.size())).valid());
      INFO("fullyConnected=" << fullyConnected << " op=" << static_cast<int>(op));
      REQUIRE(actual == expected);
    }
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: one sweep pair preserves intermediate scan order", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  auto checkLayout = [](usize dimZ, usize slabPlanes) {
    constexpr usize k_DimX = 7;
    constexpr usize k_DimY = 6;
    const usize slabValueBudget = slabPlanes * k_DimX * k_DimY;
    const std::vector<int16> mask = MakeMask<int16>(k_DimX, k_DimY, dimZ);
    for(bool fullyConnected : {false, true})
    {
      for(ReconstructOp op : {ReconstructOp::Dilation, ReconstructOp::Erosion})
      {
        const std::vector<int16> marker = (op == ReconstructOp::Dilation) ? SeedMarkerBelow<int16>(mask, k_DimX, k_DimY, dimZ) : SeedMarkerAbove<int16>(mask, k_DimX, k_DimY, dimZ);
        const std::vector<int16> expected = ReconstructSweepPairOracle<int16>(marker, mask, k_DimX, k_DimY, dimZ, op, fullyConnected);
        const std::vector<int16> fixpoint = ReconstructOracle<int16>(marker, mask, k_DimX, k_DimY, dimZ, op, fullyConnected);
        const std::vector<int16> actual = RunSweep<int16>(marker, mask, k_DimX, k_DimY, dimZ, op, fullyConnected, slabValueBudget, 1);
        INFO("dimZ=" << dimZ << " slabPlanes=" << slabPlanes << " fullyConnected=" << fullyConnected << " op=" << static_cast<int>(op));
        REQUIRE(expected != fixpoint); // Proves this assertion observes an intermediate state, not just convergence.
        REQUIRE(actual == expected);
      }
    }
  };

  SECTION("forced one-plane slabs")
  {
    checkLayout(4, 1);
  }
  SECTION("exact multiple of a multi-plane slab")
  {
    checkLayout(6, 2);
  }
  SECTION("short reverse-direction tail slab")
  {
    checkLayout(7, 3);
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: bounded 2D row blocks and overwide tiles preserve exact scan semantics", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 9;
  constexpr usize k_DimZ = 1;
  constexpr usize k_FullWidthBudget = 3 * k_DimX; // two core rows plus one halo row
  constexpr usize k_OverwideBudget = 6;           // four core columns plus two X halos
  const std::vector<int16> mask = MakeMask<int16>(k_DimX, k_DimY, k_DimZ);

  for(bool fullyConnected : {false, true})
  {
    for(ReconstructOp op : {ReconstructOp::Dilation, ReconstructOp::Erosion})
    {
      const std::vector<int16> marker = (op == ReconstructOp::Dilation) ? SeedMarkerBelow<int16>(mask, k_DimX, k_DimY, k_DimZ) : SeedMarkerAbove<int16>(mask, k_DimX, k_DimY, k_DimZ);
      const std::vector<int16> expected = ReconstructOracle<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected);
      const std::vector<int16> blocked = RunSweep<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected, k_FullWidthBudget);
      const std::vector<int16> tiled = RunSweep<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected, k_OverwideBudget);
      INFO("fullyConnected=" << fullyConnected << " op=" << static_cast<int>(op));
      REQUIRE(blocked == expected);
      REQUIRE(tiled == expected);

      const std::vector<int16> onePairExpected = ReconstructSweepPairOracle<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected);
      REQUIRE(RunSweep<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected, k_FullWidthBudget, 1) == onePairExpected);
      REQUIRE(RunSweep<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected, k_OverwideBudget, 1) == onePairExpected);
    }
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: validates dimensions and store sizes", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  DataStore<int8> markerStore(ShapeType{1}, ShapeType{1}, int8{7});
  DataStore<int8> maskStore(ShapeType{1}, ShapeType{1}, int8{8});
  DataStore<int8> outStore(ShapeType{1}, ShapeType{1}, int8{42});
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  SECTION("XY product overflow")
  {
    const Result<> result =
        ReconstructSweep<int8>{markerStore, maskStore, outStore, SizeVec3Type{std::numeric_limits<usize>::max(), 2, 1}, ReconstructOp::Dilation, false, shouldCancel, messageHandler}();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8640);
  }
  SECTION("XYZ product overflow")
  {
    const Result<> result =
        ReconstructSweep<int8>{markerStore, maskStore, outStore, SizeVec3Type{std::numeric_limits<usize>::max(), 1, 2}, ReconstructOp::Dilation, false, shouldCancel, messageHandler}();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8641);
  }
  SECTION("store size mismatch")
  {
    const Result<> result = ReconstructSweep<int8>{markerStore, maskStore, outStore, SizeVec3Type{2, 2, 2}, ReconstructOp::Dilation, false, shouldCancel, messageHandler}();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8642);
  }
  SECTION("zero dimension is an explicit no-op")
  {
    const Result<> result = ReconstructSweep<int8>{markerStore, maskStore, outStore, SizeVec3Type{0, 2, 3}, ReconstructOp::Dilation, false, shouldCancel, messageHandler}();
    REQUIRE(result.valid());
    REQUIRE(outStore.getValue(0) == int8{42});
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: multi-slab short tail preserves exact scan semantics", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  // Two planes per slab over seven planes forces 2 + 2 + 2 + 1 in both directions. The seeded markers require
  // propagation across every slab boundary, exercising both the carried boundary plane and the short tail.
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr usize k_SlabValueBudget = 2 * k_DimX * k_DimY;
  const std::vector<int16> mask = MakeMask<int16>(k_DimX, k_DimY, k_DimZ);

  for(bool fullyConnected : {false, true})
  {
    for(ReconstructOp op : {ReconstructOp::Dilation, ReconstructOp::Erosion})
    {
      const std::vector<int16> marker = (op == ReconstructOp::Dilation) ? SeedMarkerBelow<int16>(mask, k_DimX, k_DimY, k_DimZ) : SeedMarkerAbove<int16>(mask, k_DimX, k_DimY, k_DimZ);
      const std::vector<int16> oracle = ReconstructOracle<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected);
      const std::vector<int16> vincent = RunVincent<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected);
      const std::vector<int16> sweep = RunSweep<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected, k_SlabValueBudget);
      INFO("fullyConnected=" << fullyConnected << " op=" << static_cast<int>(op));
      REQUIRE(sweep == oracle);
      REQUIRE(sweep == vincent);
    }
  }
}

TEMPLATE_TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: Vincent == Sweep == oracle (3D + 2D + small)", "[NXImageProcessing][MorphologicalReconstructionEngine]", uint8, int16, int32,
                   float32)
{
  using T = TestType;
  SECTION("3D non-cubic 4x5x6")
  {
    CheckReconstruction<T>(k_X, k_Y, k_Z);
  }
  SECTION("2D single-plane 6x5x1")
  {
    CheckReconstruction<T>(6, 5, 1);
  }
  SECTION("small 3x3x3 (heavy clipping)")
  {
    CheckReconstruction<T>(3, 3, 3);
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: hand-computed dark-hole fill (erosion)", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  // 5x5x1 bright field (value 100) with a single dark hole (value 0) at the center, NOT touching the border.
  // Fillhole-style marker: max (100) in the interior, input on the border. Reconstruct-by-erosion fills the hole
  // to 100 (a dark minimum not connected to the border is removed).
  constexpr usize k_D = 5;
  std::vector<int32> mask(k_D * k_D, 100);
  mask[FlatIndex(2, 2, 0, k_D, k_D)] = 0; // interior hole

  std::vector<int32> marker(k_D * k_D, 100); // interior max
  // border = input (all 100 here, so border stays 100).
  const std::vector<int32> out = RunVincent<int32>(marker, mask, k_D, k_D, 1, ReconstructOp::Erosion, false);
  const std::vector<int32> sweep = RunSweep<int32>(marker, mask, k_D, k_D, 1, ReconstructOp::Erosion, false);
  for(usize i = 0; i < mask.size(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(out[i] == 100); // hole filled to the surrounding level
    REQUIRE(sweep[i] == 100);
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: hand-computed shallow vs deep maximum (dilation)", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  // 1D line (7x1x1), background 10, with a shallow bump (+3 -> 13) and a deep bump (+30 -> 40). HMaxima marker =
  // input - h with h = 5: bumps below h vanish (flattened to background), bumps >= h survive reduced by h.
  constexpr usize k_D = 7;
  std::vector<int32> mask(k_D, 10);
  mask[2] = 13; // shallow (+3 < h)
  mask[5] = 40; // deep (+30 >= h)
  const double h = 5.0;
  std::vector<int32> marker(k_D);
  for(usize i = 0; i < k_D; ++i)
  {
    marker[i] = mask[i] - static_cast<int32>(h);
  }
  const std::vector<int32> out = RunVincent<int32>(marker, mask, k_D, 1, 1, ReconstructOp::Dilation, false);
  const std::vector<int32> sweep = RunSweep<int32>(marker, mask, k_D, 1, 1, ReconstructOp::Dilation, false);
  REQUIRE(out == sweep);
  // Shallow bump is suppressed to background level; deep bump survives but reduced by h.
  REQUIRE(out[2] == 10); // 13 was < h above background -> flattened
  REQUIRE(out[5] == 35); // 40 -> reduced by h (=5)
  REQUIRE(out[0] == 10);
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: configured mask-derived height markers match provided markers", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 3;
  constexpr float64 k_Height = 9.0;
  const std::vector<int16> mask = MakeMask<int16>(k_DimX, k_DimY, k_DimZ);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  for(const ReconstructOp op : {ReconstructOp::Dilation, ReconstructOp::Erosion})
  {
    const bool dilation = op == ReconstructOp::Dilation;
    const std::vector<int16> marker = dilation ? MarkerBelow<int16>(mask, k_Height) : MarkerAbove<int16>(mask, k_Height);
    const std::vector<int16> expected = ReconstructOracle(marker, mask, k_DimX, k_DimY, k_DimZ, op, /*fullyConnected=*/true);
    const ReconstructionMarkerOptions options{.source = dilation ? ReconstructionMarkerSource::MaskMinusHeight : ReconstructionMarkerSource::MaskPlusHeight, .height = k_Height};

    DataStore<int16> maskStore = MakeStore(mask, k_DimX, k_DimY, k_DimZ);
    DataStore<int16> vincentOutput(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int16{0});
    DataStore<int16> sweepOutput(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int16{0});
    ReconstructVincent<int16> vincent{maskStore, maskStore, vincentOutput, SizeVec3Type{k_DimX, k_DimY, k_DimZ}, op, true, shouldCancel, messageHandler, options};
    ReconstructSweep<int16, true> sweep{maskStore,
                                        maskStore,
                                        sweepOutput,
                                        SizeVec3Type{k_DimX, k_DimY, k_DimZ},
                                        op,
                                        true,
                                        shouldCancel,
                                        messageHandler,
                                        ReconstructSweep<int16, true>::k_DefaultSlabValues,
                                        std::numeric_limits<usize>::max(),
                                        options};
    REQUIRE(vincent().valid());
    REQUIRE(sweep().valid());

    std::vector<int16> actual(expected.size());
    REQUIRE(vincentOutput.copyIntoBuffer(0, nonstd::span<int16>(actual.data(), actual.size())).valid());
    REQUIRE(actual == expected);
    REQUIRE(sweepOutput.copyIntoBuffer(0, nonstd::span<int16>(actual.data(), actual.size())).valid());
    REQUIRE(actual == expected);
  }
}

TEST_CASE("NXImageProcessing::MorphologicalReconstructionEngine: configured mask-derived border markers match provided markers", "[NXImageProcessing][MorphologicalReconstructionEngine]")
{
  constexpr usize k_DimX = 4;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 6;
  constexpr usize k_VolumeValues = k_DimX * k_DimY * k_DimZ;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int16> mask = MakeMask<int16>(k_DimX, k_DimY, k_DimZ);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  for(const bool fullyConnected : {false, true})
  {
    for(const ReconstructOp op : {ReconstructOp::Dilation, ReconstructOp::Erosion})
    {
      const bool dilation = op == ReconstructOp::Dilation;
      const int16 interiorValue = dilation ? std::numeric_limits<int16>::lowest() : std::numeric_limits<int16>::max();
      const ReconstructionMarkerSource source = dilation ? ReconstructionMarkerSource::MaskWithMinimumInterior : ReconstructionMarkerSource::MaskWithMaximumInterior;
      std::vector<int16> marker = mask;
      for(usize z = 0; z < k_DimZ; ++z)
      {
        for(usize y = 0; y < k_DimY; ++y)
        {
          for(usize x = 0; x < k_DimX; ++x)
          {
            const bool border = x == 0 || x + 1 == k_DimX || y == 0 || y + 1 == k_DimY || z == 0 || z + 1 == k_DimZ;
            if(!border)
            {
              marker[FlatIndex(x, y, z, k_DimX, k_DimY)] = interiorValue;
            }
          }
        }
      }
      const std::vector<int16> expected = ReconstructOracle<int16>(marker, mask, k_DimX, k_DimY, k_DimZ, op, fullyConnected);
      const ReconstructionMarkerOptions options{.source = source, .height = 0.0};

      DataStore<int16> maskStore = MakeStore(mask, k_DimX, k_DimY, k_DimZ);
      DataStore<int16> vincentOutput(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int16{0});
      CountingDataStore<int16> sweepMaskStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int16{0});
      DataStore<int16> sweepOutput(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int16{0});
      REQUIRE(sweepMaskStore.copyFromBuffer(0, nonstd::span<const int16>(mask.data(), mask.size())).valid());
      ReconstructVincent<int16> vincent{maskStore, maskStore, vincentOutput, dims, op, fullyConnected, shouldCancel, messageHandler, options};
      ReconstructSweep<int16, true> sweep{
          sweepMaskStore, sweepMaskStore, sweepOutput, dims, op, fullyConnected, shouldCancel, messageHandler, k_VolumeValues, std::numeric_limits<usize>::max(), options, 0, 0, true};
      REQUIRE(vincent().valid());
      REQUIRE(sweep().valid());

      std::vector<int16> actual(expected.size());
      REQUIRE(vincentOutput.copyIntoBuffer(0, nonstd::span<int16>(actual.data(), actual.size())).valid());
      INFO("Vincent fullyConnected=" << fullyConnected << " op=" << static_cast<int>(op));
      REQUIRE(actual == expected);
      REQUIRE(sweepOutput.copyIntoBuffer(0, nonstd::span<int16>(actual.data(), actual.size())).valid());
      INFO("Sweep fullyConnected=" << fullyConnected << " op=" << static_cast<int>(op));
      REQUIRE(actual == expected);
      REQUIRE(sweepMaskStore.readCallCount() == 1);
    }
  }
}
