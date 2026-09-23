#include "simplnx/Utilities/ImageProcessing/WatershedFromMarkersEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/WatershedExternalMemory.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"

#include <catch2/catch.hpp>

#include <atomic>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
struct TemporaryRecordStoreStats
{
  uint64 readBytes = 0;
  uint64 writeBytes = 0;
  std::vector<uint64> readByteCounts;
  std::vector<uint64> writeByteCounts;
};

class CountingTemporaryRecordStore final : public ITemporaryRecordStore
{
public:
  CountingTemporaryRecordStore(std::unique_ptr<ITemporaryRecordStore> store, std::shared_ptr<TemporaryRecordStoreStats> stats)
  : m_Store(std::move(store))
  , m_Stats(std::move(stats))
  {
  }

  uint64 recordSize() const override
  {
    return m_Store->recordSize();
  }

  uint64 recordCount() const override
  {
    return m_Store->recordCount();
  }

  uint64 maxRecordsPerBatch() const override
  {
    return m_Store->maxRecordsPerBatch();
  }

  bool isReadOnly() const override
  {
    return m_Store->isReadOnly();
  }

  Result<uint64> read(uint64 offset, uint64 count, nonstd::span<std::byte> bytes, const std::atomic_bool& shouldCancel) const override
  {
    auto result = m_Store->read(offset, count, bytes, shouldCancel);
    if(result.valid())
    {
      const uint64 transferredBytes = result.value() * recordSize();
      m_Stats->readBytes += transferredBytes;
      m_Stats->readByteCounts.push_back(transferredBytes);
    }
    return result;
  }

  Result<> write(uint64 offset, uint64 count, nonstd::span<const std::byte> bytes, const std::atomic_bool& shouldCancel) override
  {
    auto result = m_Store->write(offset, count, bytes, shouldCancel);
    if(result.valid())
    {
      const uint64 transferredBytes = count * recordSize();
      m_Stats->writeBytes += transferredBytes;
      m_Stats->writeByteCounts.push_back(transferredBytes);
    }
    return result;
  }

  Result<> fill(uint64 offset, uint64 count, nonstd::span<const std::byte> record, const std::atomic_bool& shouldCancel) override
  {
    return m_Store->fill(offset, count, record, shouldCancel);
  }

  Result<> resize(uint64 count, const std::atomic_bool& shouldCancel) override
  {
    return m_Store->resize(count, shouldCancel);
  }

private:
  std::unique_ptr<ITemporaryRecordStore> m_Store;
  std::shared_ptr<TemporaryRecordStoreStats> m_Stats;
};

template <class TInput>
std::vector<uint32> RunFlood(const std::vector<TInput>& gray, const std::vector<uint32>& markers, usize dx, usize dy, usize dz, bool markWatershedLine, bool fullyConnected)
{
  DataStore<TInput> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0);
  DataStore<uint32> markerStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0u);
  DataStore<uint32> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0u);
  for(usize i = 0; i < gray.size(); ++i)
  {
    inStore.setValue(i, gray[i]);
    markerStore.setValue(i, markers[i]);
  }
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const uint32 borderSentinel = std::numeric_limits<uint32>::max();
  const Result<> r = ApplyWatershedFromMarkers<TInput>(inStore, markerStore, outStore, SizeVec3{dx, dy, dz}, markWatershedLine, fullyConnected, borderSentinel, shouldCancel, messageHandler);
  REQUIRE(r.valid());
  std::vector<uint32> out(gray.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

template <class TInput>
std::vector<uint32> RunExternalFlood(const std::vector<TInput>& gray, const std::vector<uint32>& markers, usize dimX, usize dimY, usize dimZ, bool markWatershedLine, bool fullyConnected,
                                     const std::shared_ptr<TemporaryRecordStoreStats>& voxelStats = {}, usize targetBytes = ImageProcessing::detail::k_WatershedExternalTargetBytes)
{
  DataStore<TInput> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0);
  DataStore<uint32> markerStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0u);
  DataStore<uint32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0u);
  for(usize index = 0; index < gray.size(); ++index)
  {
    inputStore.setValue(index, gray[index]);
    markerStore.setValue(index, markers[index]);
  }

  auto planResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<TInput>(gray.size(), targetBytes, markWatershedLine);
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);
  const auto& plan = planResult.value();
  TemporaryRecordStoreConfig voxelConfig;
  voxelConfig.recordSize = plan.voxelRecordBytes;
  voxelConfig.maxRecordsPerBatch = std::max(plan.voxelRecordsPerPage, plan.transferRecords);
  voxelConfig.initialRecordCount = gray.size();
  if(plan.useTiledVoxelLayout)
  {
    auto layoutResult = ImageProcessing::detail::CreateWatershedTiledRecordLayout(SizeVec3{dimX, dimY, dimZ}, plan.voxelRecordsPerPage);
    SIMPLNX_RESULT_REQUIRE_VALID(layoutResult);
    voxelConfig.initialRecordCount = layoutResult.value().recordCount;
  }
  auto voxelStoreResult = InMemoryTemporaryRecordStore::Create(voxelConfig);
  SIMPLNX_RESULT_REQUIRE_VALID(voxelStoreResult);

  TemporaryRecordStoreConfig queueConfig;
  queueConfig.recordSize = plan.useBucketQueue ? sizeof(ImageProcessing::detail::WatershedQueueBlock) : sizeof(ImageProcessing::detail::WatershedHeapRecord<TInput>);
  queueConfig.maxRecordsPerBatch = plan.useBucketQueue ? plan.bucketBlocksPerPage : plan.heapRecordsPerPage;
  queueConfig.initialRecordCount = plan.useBucketQueue ? plan.bucketBlockCount : gray.size();
  auto queueStoreResult = InMemoryTemporaryRecordStore::Create(queueConfig);
  SIMPLNX_RESULT_REQUIRE_VALID(queueStoreResult);

  std::unique_ptr<ITemporaryRecordStore> voxelStore = std::move(voxelStoreResult.value());
  if(voxelStats != nullptr)
  {
    voxelStore = std::make_unique<CountingTemporaryRecordStore>(std::move(voxelStore), voxelStats);
  }

  std::atomic_bool shouldCancel{false};
  const Result<> result = ApplyWatershedFromMarkersExternal<TInput>(inputStore, markerStore, outputStore, SizeVec3{dimX, dimY, dimZ}, markWatershedLine, fullyConnected,
                                                                    std::numeric_limits<uint32>::max(), shouldCancel, std::move(voxelStore), std::move(queueStoreResult.value()), plan);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  std::vector<uint32> output(gray.size());
  auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<uint32>(output.data(), output.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  return output;
}
} // namespace

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: 1D double-well, one marker per well, watershed line at the ridge", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  // gray: 0 1 2 3 2 1 0  (a ridge at x=3). markers: label 1 at x=0, label 2 at x=6.
  const std::vector<int32> gray = {0, 1, 2, 3, 2, 1, 0};
  std::vector<uint32> markers(7, 0u);
  markers[0] = 1u;
  markers[6] = 2u;
  const std::vector<uint32> withLine = RunFlood(gray, markers, 7, 1, 1, /*markWatershedLine=*/true, /*fullyConnected=*/false);
  // basins fill from each marker up the slopes; the ridge x=3 borders both labels -> watershed line (0).
  REQUIRE(withLine[0] == 1u);
  REQUIRE(withLine[1] == 1u);
  REQUIRE(withLine[2] == 1u);
  REQUIRE(withLine[3] == 0u); // ridge = watershed line
  REQUIRE(withLine[4] == 2u);
  REQUIRE(withLine[5] == 2u);
  REQUIRE(withLine[6] == 2u);
  // Without watershed lines, the ridge is assigned to one basin (no 0s among the labeled region).
  const std::vector<uint32> noLine = RunFlood(gray, markers, 7, 1, 1, /*markWatershedLine=*/false, /*fullyConnected=*/false);
  for(uint32 v : noLine)
  {
    REQUIRE(v != 0u); // every pixel labeled (Beucher: no watershed line)
  }
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: single marker floods the whole basin", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  const usize dx = 5, dy = 5, dz = 1;
  std::vector<int32> gray(dx * dy);
  for(usize y = 0; y < dy; ++y)
  {
    for(usize x = 0; x < dx; ++x)
    {
      gray[y * dx + x] = static_cast<int32>(std::abs(static_cast<int64>(x) - 2) + std::abs(static_cast<int64>(y) - 2)); // bowl, min at center
    }
  }
  std::vector<uint32> markers(dx * dy, 0u);
  markers[2 * dx + 2] = 7u; // one marker at the basin bottom
  const std::vector<uint32> out = RunFlood(gray, markers, dx, dy, dz, true, false);
  for(uint32 v : out)
  {
    REQUIRE(v == 7u); // the whole bowl is one basin
  }
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: determinism", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  const usize dx = 8, dy = 8, dz = 4;
  std::vector<int32> gray(dx * dy * dz);
  std::vector<uint32> markers(dx * dy * dz, 0u);
  for(usize i = 0; i < gray.size(); ++i)
  {
    gray[i] = static_cast<int32>((i * 31) % 17);
  }
  markers[0] = 1u;
  markers[gray.size() - 1] = 2u;
  const std::vector<uint32> a = RunFlood(gray, markers, dx, dy, dz, true, false);
  const std::vector<uint32> b = RunFlood(gray, markers, dx, dy, dz, true, false);
  REQUIRE(a == b);
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: fullyConnected offset set floods a single bowl", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  // A single marker always fills the whole (connected) basin regardless of connectivity: there is only ever one real
  // label among a voxel's neighbors, so no collision/watershed line ever forms. This exercises the fullyConnected=true
  // 8-neighbor offset generation and confirms it does not break the flood. A true-vs-false split where the watershed
  // LINE differs is a non-unique-plateau case gated by the direct-ITK grid in Task 2, not asserted analytically here.
  const usize dx = 5, dy = 5, dz = 1;
  std::vector<int32> gray(dx * dy);
  for(usize y = 0; y < dy; ++y)
  {
    for(usize x = 0; x < dx; ++x)
    {
      gray[y * dx + x] = static_cast<int32>(std::abs(static_cast<int64>(x) - 2) + std::abs(static_cast<int64>(y) - 2)); // bowl
    }
  }
  std::vector<uint32> markers(dx * dy, 0u);
  markers[2 * dx + 2] = 3u;
  const std::vector<uint32> outFull = RunFlood(gray, markers, dx, dy, dz, /*markWatershedLine=*/true, /*fullyConnected=*/true);
  const std::vector<uint32> outFace = RunFlood(gray, markers, dx, dy, dz, /*markWatershedLine=*/true, /*fullyConnected=*/false);
  for(usize i = 0; i < gray.size(); ++i)
  {
    REQUIRE(outFull[i] == 3u); // whole bowl is one basin under 8-connectivity
    REQUIRE(outFace[i] == 3u); // ... and under 4-connectivity
  }
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: a marker valued at the border sentinel is a normal marker", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  // A real in-bounds marker whose label == borderSentinel (uint32 max) must be treated as an ordinary label, not as an
  // out-of-image boundary. Place it at a border-touching corner so its OOB neighbors also read as the sentinel: with a
  // single marker the whole grid still floods to that label (OOB reads are neutral and never collide).
  const uint32 sentinel = std::numeric_limits<uint32>::max();
  const usize dx = 4, dy = 4, dz = 1;
  std::vector<int32> gray(dx * dy);
  for(usize y = 0; y < dy; ++y)
  {
    for(usize x = 0; x < dx; ++x)
    {
      gray[y * dx + x] = static_cast<int32>(x + y); // gentle ramp
    }
  }
  std::vector<uint32> markers(dx * dy, 0u);
  markers[0] = sentinel; // corner marker (touches the image border), labeled with the sentinel value
  const std::vector<uint32> out = RunFlood(gray, markers, dx, dy, dz, /*markWatershedLine=*/true, /*fullyConnected=*/false);
  for(uint32 v : out)
  {
    REQUIRE(v == sentinel);
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::WatershedExternalMemory: checked plan respects different granted targets", "[ImageProcessing][WatershedFromMarkersEngine]", uint8, int8, uint16, int16, uint32,
                   int32, uint64, int64, float32, float64)
{
  using T = TestType;
  const usize targetBytes = GENERATE(32ULL * 1024ULL * 1024ULL, 64ULL * 1024ULL * 1024ULL, 256ULL * 1024ULL * 1024ULL);
  CAPTURE(targetBytes);
  const auto planResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<T>(33'554'432, targetBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);
  const auto& plan = planResult.value();
  REQUIRE(plan.voxelRecordsPerPage > 0);
  REQUIRE(plan.voxelCachePages > 0);
  REQUIRE(plan.heapRecordsPerPage > 0);
  REQUIRE((plan.heapCachePages > 0 || plan.useBucketQueue));
  REQUIRE(plan.useBucketQueue == (std::is_integral_v<T> && sizeof(T) <= sizeof(uint16)));
  REQUIRE(plan.transferRecords > 0);
  REQUIRE(plan.residentBytes <= targetBytes);
  if constexpr(ImageProcessing::detail::k_UseWatershedBucketQueue<T>)
  {
    REQUIRE(plan.bucketBlockCount > 0);
    REQUIRE(plan.bucketBlocksPerPage > 0);
    REQUIRE(plan.bucketCachePages > 0);
  }

  auto createWatershedExternalMemoryPlanResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<T>(0);
  SIMPLNX_RESULT_REQUIRE_INVALID(createWatershedExternalMemoryPlanResult);
}

TEMPLATE_TEST_CASE("ImageProcessing::WatershedExternalMemory: larger working-memory targets retain more fixed-size pages", "[ImageProcessing][WatershedFromMarkersEngine]", uint8, float32)
{
  using T = TestType;
  constexpr usize k_ValueCount = 33'554'432;
  constexpr usize k_BaseTargetBytes = 64ULL * 1024ULL * 1024ULL;
  constexpr usize k_LargerTargetBytes = 512ULL * 1024ULL * 1024ULL;

  auto baseResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<T>(k_ValueCount, k_BaseTargetBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(baseResult);
  const auto& base = baseResult.value();

  auto largerResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<T>(k_ValueCount, k_LargerTargetBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(largerResult);
  const auto& larger = largerResult.value();

  REQUIRE(larger.heapRecordsPerPage == base.heapRecordsPerPage);
  REQUIRE(larger.voxelCachePages > base.voxelCachePages);
  if constexpr(ImageProcessing::detail::k_UseWatershedBucketQueue<T>)
  {
    REQUIRE(base.usePackedResidentStatusState);
    REQUIRE(larger.usePackedCombinedState);
    REQUIRE(base.voxelRecordBytes == sizeof(ImageProcessing::detail::WatershedPackedResidentStatusVoxelRecord<T>));
    REQUIRE(larger.voxelRecordBytes == sizeof(ImageProcessing::detail::WatershedPackedVoxelRecord<T>));
    REQUIRE(base.useTiledVoxelLayout);
    REQUIRE(larger.completePrimaryStateCache);
    REQUIRE_FALSE(larger.useTiledVoxelLayout);
    REQUIRE(larger.voxelCachePages * larger.voxelRecordsPerPage > base.voxelCachePages * base.voxelRecordsPerPage);
    REQUIRE(larger.bucketCachePages > base.bucketCachePages);
  }
  else
  {
    REQUIRE(larger.voxelRecordsPerPage == base.voxelRecordsPerPage);
    REQUIRE(larger.heapCachePages > base.heapCachePages);
  }
  REQUIRE(base.residentBytes <= k_BaseTargetBytes);
  REQUIRE(larger.residentBytes <= k_LargerTargetBytes);
  REQUIRE(larger.residentBytes > base.residentBytes);

  auto createWatershedExternalMemoryPlanResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<T>(k_ValueCount, /*targetBytes=*/1);
  SIMPLNX_RESULT_REQUIRE_INVALID(createWatershedExternalMemoryPlanResult);
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: packed combined voxel record preserves every field", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  using Uint8Record = ImageProcessing::detail::WatershedVoxelRecord<uint8>;
  using Uint16Record = ImageProcessing::detail::WatershedVoxelRecord<uint16>;
  static_assert(sizeof(ImageProcessing::detail::WatershedPackedVoxelRecord<uint8>) == 6);
  static_assert(sizeof(ImageProcessing::detail::WatershedPackedVoxelRecord<uint16>) == 7);

  for(const Uint8Record record :
      {Uint8Record{0, 0, std::numeric_limits<uint8>::lowest()}, Uint8Record{std::numeric_limits<uint32>::max(), std::numeric_limits<uint8>::max(), std::numeric_limits<uint8>::max()}})
  {
    const auto decoded = ImageProcessing::detail::DecodeWatershedPackedVoxelRecord<uint8>(ImageProcessing::detail::EncodeWatershedPackedVoxelRecord(record));
    REQUIRE(decoded.output == record.output);
    REQUIRE(decoded.status == record.status);
    REQUIRE(decoded.grayValue == record.grayValue);
  }
  for(const Uint16Record record :
      {Uint16Record{0, 0, std::numeric_limits<uint16>::lowest()}, Uint16Record{std::numeric_limits<uint32>::max(), std::numeric_limits<uint8>::max(), std::numeric_limits<uint16>::max()}})
  {
    const auto decoded = ImageProcessing::detail::DecodeWatershedPackedVoxelRecord<uint16>(ImageProcessing::detail::EncodeWatershedPackedVoxelRecord(record));
    REQUIRE(decoded.output == record.output);
    REQUIRE(decoded.status == record.status);
    REQUIRE(decoded.grayValue == record.grayValue);
  }
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: packed resident-status voxel record preserves output and gray value", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  using Uint8Record = ImageProcessing::detail::WatershedVoxelRecord<uint8>;
  using Uint16Record = ImageProcessing::detail::WatershedVoxelRecord<uint16>;
  static_assert(sizeof(ImageProcessing::detail::WatershedPackedResidentStatusVoxelRecord<uint8>) == 5);
  static_assert(sizeof(ImageProcessing::detail::WatershedPackedResidentStatusVoxelRecord<uint16>) == 6);

  for(const Uint8Record record :
      {Uint8Record{0, 0, std::numeric_limits<uint8>::lowest()}, Uint8Record{std::numeric_limits<uint32>::max(), std::numeric_limits<uint8>::max(), std::numeric_limits<uint8>::max()}})
  {
    const auto decoded = ImageProcessing::detail::DecodeWatershedPackedResidentStatusVoxelRecord<uint8>(ImageProcessing::detail::EncodeWatershedPackedResidentStatusVoxelRecord(record));
    REQUIRE(decoded.output == record.output);
    REQUIRE(decoded.status == 0);
    REQUIRE(decoded.grayValue == record.grayValue);
  }
  for(const Uint16Record record :
      {Uint16Record{0, 0, std::numeric_limits<uint16>::lowest()}, Uint16Record{std::numeric_limits<uint32>::max(), std::numeric_limits<uint8>::max(), std::numeric_limits<uint16>::max()}})
  {
    const auto decoded = ImageProcessing::detail::DecodeWatershedPackedResidentStatusVoxelRecord<uint16>(ImageProcessing::detail::EncodeWatershedPackedResidentStatusVoxelRecord(record));
    REQUIRE(decoded.output == record.output);
    REQUIRE(decoded.status == 0);
    REQUIRE(decoded.grayValue == record.grayValue);
  }
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: packed resident-status tiled pages retain complete 32 by 32 tiles", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  constexpr usize k_ValueCount = 268'435'456;
  constexpr usize k_TargetBytes = 512ULL * 1024ULL * 1024ULL;
  auto planResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_ValueCount, k_TargetBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);
  const auto& plan = planResult.value();
  REQUIRE(plan.usePackedResidentStatusState);
  REQUIRE_FALSE(plan.useSplitBucketState);
  REQUIRE(plan.useTiledVoxelLayout);
  REQUIRE(plan.voxelRecordBytes == 5);
  REQUIRE(plan.voxelRecordsPerPage == 12'288);
  REQUIRE(plan.voxelRecordsPerPage % (32 * 32) == 0);
  REQUIRE(plan.voxelRecordsPerPage * plan.voxelRecordBytes == 61'440);
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: 16 MiB route retains packed resident status and one full initialization page", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  constexpr usize k_ValueCount = 33'554'432;
  constexpr usize k_TargetBytes = 16ULL * 1024ULL * 1024ULL;
  auto planResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_ValueCount, k_TargetBytes, /*markWatershedLine=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);
  const auto& plan = planResult.value();
  REQUIRE(plan.usePackedResidentStatusState);
  REQUIRE(plan.useResidentStatus);
  REQUIRE_FALSE(plan.completePrimaryStateCache);
  REQUIRE(plan.useTiledVoxelLayout);
  REQUIRE(plan.voxelRecordBytes == sizeof(ImageProcessing::detail::WatershedPackedResidentStatusVoxelRecord<uint8>));
  REQUIRE(plan.transferRecords >= plan.voxelRecordsPerPage);
  REQUIRE(plan.residentBytes <= k_TargetBytes);
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: packed bucket state matches resident ordering", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  struct Geometry
  {
    usize dimX;
    usize dimY;
    usize dimZ;
  };
  const std::array<Geometry, 2> geometries = {{{9, 7, 1}, {8, 7, 3}}};
  for(const Geometry& geometry : geometries)
  {
    const usize count = geometry.dimX * geometry.dimY * geometry.dimZ;
    std::vector<uint8> gray(count);
    std::vector<uint32> markers(count, 0u);
    for(usize index = 0; index < count; ++index)
    {
      gray[index] = static_cast<uint8>((index * 31) % 19);
    }
    markers[0] = std::numeric_limits<uint32>::max();
    markers[geometry.dimX + 1] = 1u;
    markers[count - 1] = 2u;
    for(const bool markWatershedLine : {false, true})
    {
      for(const bool fullyConnected : {false, true})
      {
        CAPTURE(geometry.dimX, geometry.dimY, geometry.dimZ, markWatershedLine, fullyConnected);
        const auto resident = RunFlood(gray, markers, geometry.dimX, geometry.dimY, geometry.dimZ, markWatershedLine, fullyConnected);
        const auto external = RunExternalFlood(gray, markers, geometry.dimX, geometry.dimY, geometry.dimZ, markWatershedLine, fullyConnected);
        REQUIRE(external == resident);
      }
    }
  }
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: packed bucket state uses six-byte raw transfers", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  constexpr usize k_DimX = 64;
  constexpr usize k_DimY = 64;
  constexpr usize k_ValueCount = k_DimX * k_DimY;
  const std::vector<uint8> gray(k_ValueCount, uint8{7});
  const std::vector<uint32> markers(k_ValueCount, 0);
  const auto stats = std::make_shared<TemporaryRecordStoreStats>();

  const auto output = RunExternalFlood(gray, markers, k_DimX, k_DimY, 1, /*markWatershedLine=*/false, /*fullyConnected=*/false, stats);
  REQUIRE(output == markers);
  REQUIRE(stats->writeBytes == 6 * k_ValueCount);
  REQUIRE(stats->readBytes == 12 * k_ValueCount);
  REQUIRE(stats->writeByteCounts == std::vector<uint64>{6 * k_ValueCount});
  REQUIRE(stats->readByteCounts == std::vector<uint64>{6 * k_ValueCount, 6 * k_ValueCount});
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: packed resident-status bucket state uses five-byte raw transfers", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  constexpr usize k_DimX = 1024;
  constexpr usize k_DimY = 1024;
  constexpr usize k_ValueCount = k_DimX * k_DimY;
  constexpr usize k_TargetBytes = 5ULL * 1024ULL * 1024ULL;
  const std::vector<uint8> gray(k_ValueCount, uint8{7});
  const std::vector<uint32> markers(k_ValueCount, 0);
  const auto stats = std::make_shared<TemporaryRecordStoreStats>();

  auto planResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_ValueCount, k_TargetBytes, /*markWatershedLine=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);
  const auto& plan = planResult.value();
  REQUIRE(plan.usePackedResidentStatusState);
  REQUIRE(plan.useTiledVoxelLayout);
  auto layoutResult = ImageProcessing::detail::CreateWatershedTiledRecordLayout(SizeVec3{k_DimX, k_DimY, 1}, plan.voxelRecordsPerPage);
  SIMPLNX_RESULT_REQUIRE_VALID(layoutResult);
  const uint64 expectedPageBytes = static_cast<uint64>(plan.voxelRecordsPerPage) * sizeof(ImageProcessing::detail::WatershedPackedResidentStatusVoxelRecord<uint8>);
  const usize expectedPageCount = layoutResult.value().recordCount / plan.voxelRecordsPerPage;
  const uint64 expectedPhysicalBytes = static_cast<uint64>(expectedPageCount) * expectedPageBytes;

  const auto output = RunExternalFlood(gray, markers, k_DimX, k_DimY, 1, /*markWatershedLine=*/true, /*fullyConnected=*/false, stats, k_TargetBytes);
  REQUIRE(output == markers);
  REQUIRE(stats->writeBytes == expectedPhysicalBytes);
  REQUIRE(stats->readBytes == 2 * expectedPhysicalBytes);
  REQUIRE(stats->writeByteCounts == std::vector<uint64>(expectedPageCount, expectedPageBytes));
  REQUIRE(stats->readByteCounts == std::vector<uint64>(2 * expectedPageCount, expectedPageBytes));
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: useful endpoint retains complete primary state before extra queue pages", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  constexpr usize k_ValueCount = 33'554'432;
  auto usefulBytesResult = ImageProcessing::detail::CalculateWatershedUsefulWorkingMemoryBytes<uint8>(k_ValueCount);
  SIMPLNX_RESULT_REQUIRE_VALID(usefulBytesResult);
  auto planResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_ValueCount, usefulBytesResult.value());
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);
  const auto& plan = planResult.value();

  const usize expectedVoxelPages = k_ValueCount / plan.voxelRecordsPerPage + static_cast<usize>(k_ValueCount % plan.voxelRecordsPerPage != 0);
  const usize completeBucketPages = plan.bucketBlockCount / plan.bucketBlocksPerPage + static_cast<usize>(plan.bucketBlockCount % plan.bucketBlocksPerPage != 0);
  REQUIRE(plan.completePrimaryStateCache);
  REQUIRE(plan.voxelCachePages == expectedVoxelPages);
  REQUIRE(plan.bucketCachePages > 2 * plan.bucketCount);
  REQUIRE(plan.bucketCachePages <= completeBucketPages);
  REQUIRE(plan.residentBytes <= usefulBytesResult.value());
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: resident status selection follows data size and grant", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  constexpr usize k_TargetBytes = 512ULL * 1024ULL * 1024ULL;
  constexpr usize k_FittingValueCount = 33'554'432;
  auto fittingResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_FittingValueCount, k_TargetBytes, /*markWatershedLine=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(fittingResult);
  const auto& fitting = fittingResult.value();
  REQUIRE_FALSE(fitting.useSplitBucketState);
  REQUIRE(fitting.usePackedCombinedState);
  REQUIRE(fitting.useResidentStatus);
  REQUIRE(fitting.voxelRecordBytes == sizeof(ImageProcessing::detail::WatershedPackedVoxelRecord<uint8>));
  REQUIRE(fitting.inputRecordsPerPage == 0);
  REQUIRE(fitting.inputCachePages == 0);
  REQUIRE(fitting.residentStatusWordCount == (k_FittingValueCount + 63) / 64);
  REQUIRE(fitting.residentStatusBytes == fitting.residentStatusWordCount * sizeof(uint64));
  REQUIRE(fitting.residentBytes <= k_TargetBytes);

  constexpr usize k_LargerCompleteValueCount = 67'108'864;
  auto largerCompleteResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_LargerCompleteValueCount, k_TargetBytes, /*markWatershedLine=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(largerCompleteResult);
  const auto& largerComplete = largerCompleteResult.value();
  REQUIRE_FALSE(largerComplete.useSplitBucketState);
  REQUIRE(largerComplete.usePackedCombinedState);
  REQUIRE(largerComplete.useResidentStatus);
  REQUIRE(largerComplete.voxelRecordBytes == sizeof(ImageProcessing::detail::WatershedPackedVoxelRecord<uint8>));
  REQUIRE(largerComplete.inputRecordsPerPage == 0);
  REQUIRE(largerComplete.inputCachePages == 0);

  constexpr usize k_PartialValueCount = 268'435'456;
  auto partialResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_PartialValueCount, k_TargetBytes, /*markWatershedLine=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(partialResult);
  REQUIRE_FALSE(partialResult.value().useSplitBucketState);
  REQUIRE(partialResult.value().usePackedResidentStatusState);
  REQUIRE(partialResult.value().useResidentStatus);
  REQUIRE_FALSE(partialResult.value().completePrimaryStateCache);
  REQUIRE(partialResult.value().useTiledVoxelLayout);
  REQUIRE(partialResult.value().bucketCachePages <= 2 * partialResult.value().bucketCount);

  constexpr usize k_TooLargeValueCount = 1'073'741'824;
  auto fallbackResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_TooLargeValueCount, 64ULL * 1024ULL * 1024ULL, /*markWatershedLine=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(fallbackResult);
  REQUIRE_FALSE(fallbackResult.value().useSplitBucketState);
  REQUIRE_FALSE(fallbackResult.value().useResidentStatus);

  auto noLineResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_FittingValueCount, k_TargetBytes, /*markWatershedLine=*/false);
  SIMPLNX_RESULT_REQUIRE_VALID(noLineResult);
  REQUIRE_FALSE(noLineResult.value().useResidentStatus);
  REQUIRE_FALSE(noLineResult.value().usePackedResidentStatusState);
  REQUIRE(noLineResult.value().usePackedCombinedState);
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: tiled voxel layout is a checked 64-bit bijection", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  const SizeVec3 k_Dims = {65, 34, 9};
  constexpr usize k_RecordsPerPage = 8192;
  auto layoutResult = ImageProcessing::detail::CreateWatershedTiledRecordLayout(k_Dims, k_RecordsPerPage);
  SIMPLNX_RESULT_REQUIRE_VALID(layoutResult);
  const auto& layout = layoutResult.value();
  REQUIRE(layout.tileDimensions == SizeVec3{32, 32, 8});
  REQUIRE(layout.recordCount >= k_Dims[0] * k_Dims[1] * k_Dims[2]);

  std::vector<uint8> seen(layout.recordCount, uint8{0});
  const usize logicalCount = k_Dims[0] * k_Dims[1] * k_Dims[2];
  for(usize logicalIndex = 0; logicalIndex < logicalCount; ++logicalIndex)
  {
    auto physicalResult = layout.physicalIndex(logicalIndex);
    SIMPLNX_RESULT_REQUIRE_VALID(physicalResult);
    REQUIRE(physicalResult.value() < layout.recordCount);
    REQUIRE(seen[physicalResult.value()] == uint8{0});
    seen[physicalResult.value()] = uint8{1};
  }

  const usize center = (4 * k_Dims[1] + 10) * k_Dims[0] + 10;
  auto centerResult = layout.physicalIndex(center);
  auto xResult = layout.physicalIndex(center + 1);
  auto yResult = layout.physicalIndex(center + k_Dims[0]);
  auto zResult = layout.physicalIndex(center + k_Dims[0] * k_Dims[1]);
  SIMPLNX_RESULT_REQUIRE_VALID(centerResult);
  SIMPLNX_RESULT_REQUIRE_VALID(xResult);
  SIMPLNX_RESULT_REQUIRE_VALID(yResult);
  SIMPLNX_RESULT_REQUIRE_VALID(zResult);
  REQUIRE(centerResult.value() / k_RecordsPerPage == xResult.value() / k_RecordsPerPage);
  REQUIRE(centerResult.value() / k_RecordsPerPage == yResult.value() / k_RecordsPerPage);
  REQUIRE(centerResult.value() / k_RecordsPerPage == zResult.value() / k_RecordsPerPage);

  auto createWatershedTiledRecordLayoutResult = ImageProcessing::detail::CreateWatershedTiledRecordLayout(k_Dims, 0);
  SIMPLNX_RESULT_REQUIRE_INVALID(createWatershedTiledRecordLayoutResult);
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: reserved plan honors the shared quarter-budget ceiling", "[ImageProcessing][WatershedFromMarkersEngine][WorkingMemory]")
{
  constexpr usize k_ValueCount = 33'554'432;
  constexpr uint64 k_OneGiB = 1024ULL * 1024ULL * 1024ULL;
  constexpr uint64 k_RequestedBytes = 512ULL * 1024ULL * 1024ULL;
  constexpr uint64 k_ExpectedGrantBytes = 256ULL * 1024ULL * 1024ULL;

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(k_OneGiB);

  {
    ImageProcessing::ScopedWorkingMemoryTuningOverride override(k_RequestedBytes);
    auto allocationResult = ImageProcessing::detail::ReserveWatershedExternalMemoryPlan<uint8>(k_ValueCount);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    const auto& allocation = allocationResult.value();
    REQUIRE(allocation.reservation.sizeBytes() == k_ExpectedGrantBytes);
    REQUIRE(manager.reservedWorkingMemoryBytes() == k_ExpectedGrantBytes);
    REQUIRE(allocation.plan.residentBytes <= k_ExpectedGrantBytes);

    auto baselineResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<uint8>(k_ValueCount);
    SIMPLNX_RESULT_REQUIRE_VALID(baselineResult);
    REQUIRE(allocation.plan.voxelCachePages > baselineResult.value().voxelCachePages);
  }

  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: production request scales across live cache budgets", "[ImageProcessing][WatershedFromMarkersEngine][WorkingMemory]")
{
  constexpr usize k_ValueCount = 33'554'432;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const std::array<uint64, 3> cacheBudgets = {256 * k_MiB, 512 * k_MiB, 1024 * k_MiB};

  // Cache budgets of at most 1 GiB avoid the machine-dependent upper cap.
  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  uint64 previousGrant = 0;
  for(const uint64 cacheBudget : cacheBudgets)
  {
    REQUIRE_FALSE(manager.setBudgetBytes(cacheBudget));
    {
      auto allocationResult = ImageProcessing::detail::ReserveWatershedExternalMemoryPlan<uint8>(k_ValueCount);
      SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
      const auto& allocation = allocationResult.value();
      REQUIRE(allocation.reservation.sizeBytes() > previousGrant);
      REQUIRE(allocation.reservation.sizeBytes() <= cacheBudget / 4);
      REQUIRE(allocation.plan.residentBytes <= allocation.reservation.sizeBytes());
      previousGrant = allocation.reservation.sizeBytes();
    }
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  }
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: resident bucket working state is dataset-scaled and requires a complete reservation",
          "[ImageProcessing][WatershedFromMarkersEngine][WorkingMemory]")
{
  constexpr usize k_ValueCount = 4'194'304;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  constexpr usize k_ExpectedBytesPerValue = sizeof(uint8) + 2 * sizeof(uint32) + sizeof(uint8) + ImageProcessing::detail::k_WatershedResidentQueueBytesPerValue;
  constexpr usize k_ExpectedBucketBytes = (usize{1} << 8) * ImageProcessing::detail::k_WatershedResidentBucketHeadroomBytes;

  auto requiredResult = ImageProcessing::detail::CalculateWatershedResidentWorkingMemoryBytes<uint8>(k_ValueCount, /*markWatershedLine=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == k_ValueCount * k_ExpectedBytesPerValue + k_ExpectedBucketBytes);

  auto halfDatasetResult = ImageProcessing::detail::CalculateWatershedResidentWorkingMemoryBytes<uint8>(k_ValueCount / 2, /*markWatershedLine=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(halfDatasetResult);
  REQUIRE(halfDatasetResult.value() < requiredResult.value());
  auto noLineResult = ImageProcessing::detail::CalculateWatershedResidentWorkingMemoryBytes<uint8>(k_ValueCount, /*markWatershedLine=*/false);
  SIMPLNX_RESULT_REQUIRE_VALID(noLineResult);
  REQUIRE(noLineResult.value() < requiredResult.value());
  auto calculateWatershedResidentWorkingMemoryBytesResult = ImageProcessing::detail::CalculateWatershedResidentWorkingMemoryBytes<float32>(k_ValueCount, /*markWatershedLine=*/true);
  SIMPLNX_RESULT_REQUIRE_INVALID(calculateWatershedResidentWorkingMemoryBytesResult);

  // Cache budgets of at most 1 GiB avoid the machine-dependent upper cap.
  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  REQUIRE_FALSE(manager.setBudgetBytes(512 * k_MiB));
  {
    auto allocationResult = ImageProcessing::detail::ReserveWatershedResidentWorkingMemory<uint8>(k_ValueCount, /*markWatershedLine=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 128 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  REQUIRE_FALSE(manager.setBudgetBytes(1024 * k_MiB));
  {
    auto allocationResult = ImageProcessing::detail::ReserveWatershedResidentWorkingMemory<uint8>(k_ValueCount, /*markWatershedLine=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: external min-heap preserves level and FIFO order", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  using Heap = ImageProcessing::detail::WatershedExternalMinHeap<int32>;
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(ImageProcessing::detail::WatershedHeapRecord<int32>);
  config.maxRecordsPerBatch = 4;
  config.initialRecordCount = 8;
  auto storeResult = InMemoryTemporaryRecordStore::Create(config);
  SIMPLNX_RESULT_REQUIRE_VALID(storeResult);
  std::atomic_bool shouldCancel{false};
  auto heapResult = Heap::Create(std::move(storeResult.value()), /*recordsPerPage=*/2, /*maximumPages=*/2, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(heapResult);
  auto& heap = *heapResult.value();

  auto pushResult = heap.push(5, 50);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult);
  auto pushResult2 = heap.push(1, 10);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult2);
  auto pushResult3 = heap.push(5, 51);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult3);
  auto pushResult4 = heap.push(-2, 20);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult4);
  auto pushResult5 = heap.push(ImageProcessing::detail::WatershedEffectiveLevel<int32>(3, 5), 52);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult5);
  auto pushResult6 = heap.push(ImageProcessing::detail::WatershedEffectiveLevel<int32>(8, 5), 80);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult6);

  const std::array<std::pair<int32, int64>, 6> expected = {{{-2, 20}, {1, 10}, {5, 50}, {5, 51}, {5, 52}, {8, 80}}};
  for(const auto& [level, flatIndex] : expected)
  {
    auto popResult = heap.pop();
    SIMPLNX_RESULT_REQUIRE_VALID(popResult);
    REQUIRE(popResult.value().level == level);
    REQUIRE(popResult.value().flatIndex == flatIndex);
  }
  REQUIRE(heap.empty());
  auto popResult2 = heap.pop();
  SIMPLNX_RESULT_REQUIRE_INVALID(popResult2);
  auto flushResult = heap.flush();
  SIMPLNX_RESULT_REQUIRE_VALID(flushResult);

  shouldCancel = true;
  auto pushResult7 = heap.push(1, 1);
  SIMPLNX_RESULT_REQUIRE_INVALID(pushResult7);
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: external min-heap rejects capacity and provider metadata", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  using Heap = ImageProcessing::detail::WatershedExternalMinHeap<float32>;
  std::atomic_bool shouldCancel{false};

  TemporaryRecordStoreConfig smallConfig;
  smallConfig.recordSize = sizeof(ImageProcessing::detail::WatershedHeapRecord<float32>);
  smallConfig.maxRecordsPerBatch = 2;
  smallConfig.initialRecordCount = 2;
  auto smallStore = InMemoryTemporaryRecordStore::Create(smallConfig);
  SIMPLNX_RESULT_REQUIRE_VALID(smallStore);
  auto heapResult = Heap::Create(std::move(smallStore.value()), /*recordsPerPage=*/2, /*maximumPages=*/1, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(heapResult);
  auto pushResult = heapResult.value()->push(1.5f, 1);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult);
  auto pushResult2 = heapResult.value()->push(1.5f, 2);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult2);
  auto pushResult3 = heapResult.value()->push(1.5f, 3);
  SIMPLNX_RESULT_REQUIRE_INVALID(pushResult3);

  TemporaryRecordStoreConfig wrongConfig = smallConfig;
  wrongConfig.recordSize = sizeof(uint8);
  auto wrongStore = InMemoryTemporaryRecordStore::Create(wrongConfig);
  SIMPLNX_RESULT_REQUIRE_VALID(wrongStore);
  auto createResult = Heap::Create(std::move(wrongStore.value()), /*recordsPerPage=*/2, /*maximumPages=*/1, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(createResult);
}

TEST_CASE("ImageProcessing::WatershedExternalMemory: fixed-level external buckets preserve numeric and FIFO order", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  using Block = ImageProcessing::detail::WatershedQueueBlock;
  using Queue = ImageProcessing::detail::WatershedExternalBucketQueue<int16>;
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(Block);
  config.maxRecordsPerBatch = 2;
  config.initialRecordCount = 8;
  auto storeResult = InMemoryTemporaryRecordStore::Create(config);
  SIMPLNX_RESULT_REQUIRE_VALID(storeResult);
  std::atomic_bool shouldCancel{false};
  constexpr uint64 k_Capacity = static_cast<uint64>(std::numeric_limits<uint32>::max()) + 16;
  auto queueResult = Queue::Create(std::move(storeResult.value()), /*recordsPerPage=*/2, /*maximumPages=*/2, k_Capacity, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(queueResult);
  auto& queue = *queueResult.value();

  auto pushResult = queue.push(5, 0);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult);
  auto pushResult2 = queue.push(-2, 1);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult2);
  auto pushResult3 = queue.push(5, 2);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult3);
  auto pushResult4 = queue.push(std::numeric_limits<int16>::lowest(), 3);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult4);
  auto pushResult5 = queue.push(std::numeric_limits<int16>::max(), 4);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult5);
  const int64 largeFlatIndex = static_cast<int64>(std::numeric_limits<uint32>::max()) + 7;
  auto pushResult6 = queue.push(ImageProcessing::detail::WatershedEffectiveLevel<int16>(3, 5), largeFlatIndex);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult6);

  const std::array<std::pair<int16, int64>, 6> expected = {{{std::numeric_limits<int16>::lowest(), 3}, {-2, 1}, {5, 0}, {5, 2}, {5, largeFlatIndex}, {std::numeric_limits<int16>::max(), 4}}};
  for(const auto& [level, flatIndex] : expected)
  {
    auto popResult = queue.pop();
    SIMPLNX_RESULT_REQUIRE_VALID(popResult);
    REQUIRE(popResult.value().level == level);
    REQUIRE(popResult.value().flatIndex == flatIndex);
  }
  REQUIRE(queue.empty());
  auto popResult2 = queue.pop();
  SIMPLNX_RESULT_REQUIRE_INVALID(popResult2);

  auto pushResult7 = queue.push(7, 6);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult7);
  auto pushResult8 = queue.push(7, 7);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult8);
  auto firstInterleaved = queue.pop();
  SIMPLNX_RESULT_REQUIRE_VALID(firstInterleaved);
  REQUIRE(firstInterleaved.value().flatIndex == 6);
  auto pushResult9 = queue.push(7, largeFlatIndex);
  SIMPLNX_RESULT_REQUIRE_VALID(pushResult9);
  auto secondInterleaved = queue.pop();
  auto thirdInterleaved = queue.pop();
  SIMPLNX_RESULT_REQUIRE_VALID(secondInterleaved);
  SIMPLNX_RESULT_REQUIRE_VALID(thirdInterleaved);
  REQUIRE(secondInterleaved.value().flatIndex == 7);
  REQUIRE(thirdInterleaved.value().flatIndex == largeFlatIndex);
  REQUIRE(queue.empty());
  auto flushResult = queue.flush();
  SIMPLNX_RESULT_REQUIRE_VALID(flushResult);
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: injected external flood matches resident ordering", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  struct Geometry
  {
    usize dimX;
    usize dimY;
    usize dimZ;
  };
  const std::array<Geometry, 2> geometries = {{{9, 7, 1}, {8, 7, 3}}};
  for(const Geometry& geometry : geometries)
  {
    const usize count = geometry.dimX * geometry.dimY * geometry.dimZ;
    std::vector<int32> gray(count);
    std::vector<uint32> markers(count, 0u);
    for(usize index = 0; index < count; ++index)
    {
      gray[index] = static_cast<int32>((index * 31) % 19) - 9;
    }
    markers[0] = std::numeric_limits<uint32>::max();
    markers[geometry.dimX + 1] = 1u;
    markers[count - 1] = 2u;

    for(const bool markWatershedLine : {false, true})
    {
      for(const bool fullyConnected : {false, true})
      {
        CAPTURE(geometry.dimX, geometry.dimY, geometry.dimZ, markWatershedLine, fullyConnected);
        const std::vector<uint32> resident = RunFlood(gray, markers, geometry.dimX, geometry.dimY, geometry.dimZ, markWatershedLine, fullyConnected);
        const std::vector<uint32> external = RunExternalFlood(gray, markers, geometry.dimX, geometry.dimY, geometry.dimZ, markWatershedLine, fullyConnected);
        REQUIRE(external == resident);
      }
    }

    std::fill(markers.begin(), markers.end(), 0u);
    REQUIRE(RunExternalFlood(gray, markers, geometry.dimX, geometry.dimY, geometry.dimZ, true, false) == RunFlood(gray, markers, geometry.dimX, geometry.dimY, geometry.dimZ, true, false));
  }
}

TEST_CASE("ImageProcessing::WatershedFromMarkersEngine: external initialization failure preserves output", "[ImageProcessing][WatershedFromMarkersEngine]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 4;
  constexpr usize valueCount = dimX * dimY;
  constexpr uint32 poison = 73u;
  DataStore<int32> inputStore(ShapeType{1, dimY, dimX}, ShapeType{1}, 2);
  DataStore<uint32> markerStore(ShapeType{1, dimY, dimX}, ShapeType{1}, 0u);
  markerStore.setValue(0, 1u);
  DataStore<uint32> outputStore(ShapeType{1, dimY, dimX}, ShapeType{1}, poison);
  auto planResult = ImageProcessing::detail::CreateWatershedExternalMemoryPlan<int32>(valueCount);
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);
  const auto& plan = planResult.value();

  TemporaryRecordStoreConfig voxelConfig;
  voxelConfig.recordSize = plan.voxelRecordBytes;
  voxelConfig.maxRecordsPerBatch = std::max(plan.voxelRecordsPerPage, plan.transferRecords);
  voxelConfig.initialRecordCount = valueCount;
  voxelConfig.readOnly = true;
  auto voxelStore = InMemoryTemporaryRecordStore::Create(voxelConfig);
  SIMPLNX_RESULT_REQUIRE_VALID(voxelStore);

  TemporaryRecordStoreConfig heapConfig;
  heapConfig.recordSize = sizeof(ImageProcessing::detail::WatershedHeapRecord<int32>);
  heapConfig.maxRecordsPerBatch = plan.heapRecordsPerPage;
  heapConfig.initialRecordCount = valueCount;
  auto heapStore = InMemoryTemporaryRecordStore::Create(heapConfig);
  SIMPLNX_RESULT_REQUIRE_VALID(heapStore);

  std::atomic_bool shouldCancel{false};
  const Result<> result = ApplyWatershedFromMarkersExternal<int32>(inputStore, markerStore, outputStore, SizeVec3{dimX, dimY, 1}, /*markWatershedLine=*/true, /*fullyConnected=*/false,
                                                                   std::numeric_limits<uint32>::max(), shouldCancel, std::move(voxelStore.value()), std::move(heapStore.value()), plan);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == -79057);
  for(const uint32 value : outputStore)
  {
    REQUIRE(value == poison);
  }
}
