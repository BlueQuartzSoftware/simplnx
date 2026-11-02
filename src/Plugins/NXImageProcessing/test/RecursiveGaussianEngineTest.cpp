#include "simplnx/Utilities/ImageProcessing/RecursiveGaussianEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/GaussianTemporaryStore.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <numeric>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
class TestTemporaryRecordStore : public ITemporaryRecordStore
{
public:
  TestTemporaryRecordStore(uint64 recordSize, uint64 recordCount, uint64 maxRecordsPerBatch)
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
  Result<uint64> read(uint64 recordOffset, uint64 requestedRecordCount, nonstd::span<std::byte> records, const std::atomic_bool&) const override
  {
    readCalls++;
    if(failRead)
    {
      return MakeErrorResult<uint64>(-7650, "Injected Gaussian scratch read failure");
    }
    const uint64 returnedRecords = shortRead && requestedRecordCount > 0 ? requestedRecordCount - 1 : requestedRecordCount;
    std::memcpy(records.data(), m_Bytes.data() + static_cast<usize>(recordOffset * m_RecordSize), static_cast<usize>(returnedRecords * m_RecordSize));
    return {returnedRecords};
  }
  Result<> write(uint64 recordOffset, uint64 recordCount, nonstd::span<const std::byte> records, const std::atomic_bool&) override
  {
    writeCalls++;
    if(failWrite)
    {
      return MakeErrorResult(-7651, "Injected Gaussian scratch write failure");
    }
    std::memcpy(m_Bytes.data() + static_cast<usize>(recordOffset * m_RecordSize), records.data(), static_cast<usize>(recordCount * m_RecordSize));
    return {};
  }
  Result<> fill(uint64, uint64, nonstd::span<const std::byte>, const std::atomic_bool&) override
  {
    return MakeErrorResult(-7652, "Unexpected Gaussian scratch fill");
  }
  Result<> resize(uint64, const std::atomic_bool&) override
  {
    return MakeErrorResult(-7653, "Unexpected Gaussian scratch resize");
  }

  mutable usize readCalls = 0;
  usize writeCalls = 0;
  bool failRead = false;
  bool failWrite = false;
  bool shortRead = false;

private:
  uint64 m_RecordSize = 0;
  uint64 m_RecordCount = 0;
  uint64 m_MaxRecordsPerBatch = 0;
  std::vector<std::byte> m_Bytes;
};

usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return (z * dimY + y) * dimX + x;
}

// Run ONE order-0/1/2 pass along `axis` over an in-RAM float32 volume built from `field`. Returns the float32 result.
std::vector<float32> RunPass(const std::vector<float32>& field, usize dx, usize dy, usize dz, uint32 axis, double sigma, double spacing, int order, bool normalize,
                             const RecursiveGaussianPassOptions& options = {})
{
  DataStore<float32> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0.0f);
  for(usize i = 0; i < field.size(); ++i)
  {
    inStore.setValue(i, field[i]);
  }
  DataStore<float32> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0.0f);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> r = RecursiveGaussianAxisPass<float32>(inStore, outStore, SizeVec3{dx, dy, dz}, axis, sigma, spacing, order, normalize, shouldCancel, messageHandler, options);
  REQUIRE(r.valid());
  std::vector<float32> out(field.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

template <class T>
std::vector<float32> RunPlaneCascade(const std::vector<T>& field, const SizeVec3& dims, const RecursiveGaussianPlaneAxis& first, const RecursiveGaussianPlaneAxis& second)
{
  DataStore<T> inStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, T{});
  for(usize i = 0; i < field.size(); ++i)
  {
    inStore.setValue(i, field[i]);
  }
  std::vector<float32> output(field.size());
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  auto sink = [&](usize offset, nonstd::span<float32> plane) -> Result<> {
    std::copy(plane.begin(), plane.end(), output.begin() + static_cast<std::ptrdiff_t>(offset));
    return {};
  };
  const Result<> result = RecursiveGaussianXYPlaneCascadeToSink<T>(inStore, dims, first, second, shouldCancel, messageHandler, sink);
  REQUIRE(result.valid());
  return output;
}
} // namespace

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: order-0 preserves a constant image", "[ImageProcessing][RecursiveGaussianEngine]")
{
  const usize d = 16;
  const std::vector<float32> field(d * d * d, 5.0f);
  const std::vector<float32> out = RunPass(field, d, d, d, /*axis=*/0, /*sigma=*/2.0, /*spacing=*/1.0, /*order=*/0, false);
  for(float32 v : out)
  {
    REQUIRE(v == Approx(5.0f).margin(1e-3));
  }
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: order-1 and order-2 of a constant image are ~0", "[ImageProcessing][RecursiveGaussianEngine]")
{
  const usize d = 16;
  const std::vector<float32> field(d * d * d, 7.0f);
  const std::vector<float32> d1 = RunPass(field, d, d, d, 0, 2.0, 1.0, 1, false);
  const std::vector<float32> d2 = RunPass(field, d, d, d, 0, 2.0, 1.0, 2, false);
  for(usize i = 0; i < field.size(); ++i)
  {
    REQUIRE(std::abs(d1[i]) < 1e-2f);
    REQUIRE(std::abs(d2[i]) < 1e-2f);
  }
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: order-0 impulse response is symmetric and sums to ~1", "[ImageProcessing][RecursiveGaussianEngine]")
{
  // A 1D-along-x impulse embedded in a 3D volume (single non-zero voxel at the x-center of one row).
  const usize dx = 32, dy = 4, dz = 4;
  std::vector<float32> field(dx * dy * dz, 0.0f);
  const usize cx = dx / 2, cy = dy / 2, cz = dz / 2;
  field[FlatIndex(cx, cy, cz, dx, dy)] = 1.0f;
  const std::vector<float32> out = RunPass(field, dx, dy, dz, /*axis=*/0, /*sigma=*/3.0, /*spacing=*/1.0, /*order=*/0, false);

  double sum = 0.0;
  for(usize x = 0; x < dx; ++x)
  {
    sum += out[FlatIndex(x, cy, cz, dx, dy)];
  }
  REQUIRE(sum == Approx(1.0).margin(1e-2));                                                                       // normalized kernel
  REQUIRE(out[FlatIndex(cx, cy, cz, dx, dy)] > out[FlatIndex(cx + 3, cy, cz, dx, dy)]);                           // peak at center
  REQUIRE(out[FlatIndex(cx + 2, cy, cz, dx, dy)] == Approx(out[FlatIndex(cx - 2, cy, cz, dx, dy)]).margin(1e-4)); // symmetric
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: smoothing along X leaves a pure-Y ramp unchanged", "[ImageProcessing][RecursiveGaussianEngine]")
{
  const usize dx = 12, dy = 12, dz = 4;
  std::vector<float32> field(dx * dy * dz, 0.0f);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        field[FlatIndex(x, y, z, dx, dy)] = static_cast<float32>(y); // varies only along Y
      }
    }
  }
  const std::vector<float32> out = RunPass(field, dx, dy, dz, /*axis=*/0, 2.0, 1.0, 0, false);
  for(usize i = 0; i < field.size(); ++i)
  {
    REQUIRE(out[i] == Approx(field[i]).margin(1e-3)); // constant along the smoothed axis => unchanged
  }
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: deterministic + tall-Z streaming runs", "[ImageProcessing][RecursiveGaussianEngine]")
{
  const usize dx = 8, dy = 8, dz = 64; // dz large enough to exercise the z-slab path
  std::vector<float32> field(dx * dy * dz);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<float32>((i * 37) % 101);
  }
  const std::vector<float32> a = RunPass(field, dx, dy, dz, /*axis=*/2, 2.5, 1.0, 0, false);
  const std::vector<float32> b = RunPass(field, dx, dy, dz, /*axis=*/2, 2.5, 1.0, 0, false);
  REQUIRE(a == b); // byte-identical across runs
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: batched Z streaming matches direct memory", "[ImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize kDimX = 7;
  constexpr usize kDimY = 11;
  constexpr usize kDimZ = 13;
  std::vector<float32> field(kDimX * kDimY * kDimZ);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<float32>((i * 37 + i / 5) % 101);
  }

  const std::vector<float32> direct = RunPass(field, kDimX, kDimY, kDimZ, /*axis=*/2, 2.5, 1.0, 0, false);
  RecursiveGaussianPassOptions streamingOptions;
  streamingOptions.stagingByteBudget = 1600; // Two Y rows per batch, leaving a one-row final batch.
  streamingOptions.useInMemoryFastPath = false;
  const std::vector<float32> streamed = RunPass(field, kDimX, kDimY, kDimZ, /*axis=*/2, 2.5, 1.0, 0, false, streamingOptions);
  REQUIRE(streamed == direct);
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: Z planner scales the supplied budget and keeps a one-row minimum", "[ImageProcessing][RecursiveGaussianEngine]")
{
  const SizeVec3 benchmarkDims{512, 512, 128};
  REQUIRE(ImageProcessing::detail::ComputeRecursiveGaussianZBatchRowCount<float32>(benchmarkDims, RecursiveGaussianPassOptions::k_FallbackStagingBytes) == 128);
  REQUIRE(ImageProcessing::detail::ComputeRecursiveGaussianZBatchRowCount<float32>(benchmarkDims, RecursiveGaussianPassOptions::k_FallbackStagingBytes * 2) == 256);
  REQUIRE(ImageProcessing::detail::ComputeRecursiveGaussianZBatchRowCount<float32>(benchmarkDims, 0) == 1);
  REQUIRE(ImageProcessing::detail::ComputeRecursiveGaussianZBatchRowCount<float64>(SizeVec3{7, 11, 13}, 1000) == 1); // One row pair requires 7 * 13 * (8 + 4) = 1092 bytes.
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: true 2D planner bounds row blocks and overwide column tiles", "[ImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize kResidentLimit = 64ULL * 1024ULL * 1024ULL;
  const auto benchmarkPlan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(5824, 5824, kResidentLimit);
  REQUIRE(benchmarkPlan.valid);
  REQUIRE_FALSE(benchmarkPlan.overflow);
  REQUIRE(benchmarkPlan.coreCols == 5824);
  REQUIRE(benchmarkPlan.blockRows >= 8);
  REQUIRE(benchmarkPlan.blockRows < 5824);
  REQUIRE(benchmarkPlan.blockCount > 1);
  REQUIRE(benchmarkPlan.residentBytes <= kResidentLimit);

  const auto tiledPlan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(16385, 17, 65536);
  REQUIRE(tiledPlan.valid);
  REQUIRE_FALSE(tiledPlan.overflow);
  REQUIRE(tiledPlan.coreCols > 0);
  REQUIRE(tiledPlan.coreCols < 16385);
  REQUIRE(tiledPlan.blockRows >= 4);
  REQUIRE(tiledPlan.residentBytes <= 65536);

  REQUIRE_FALSE(ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(16385, 17, 4096).valid);
  const auto overflowPlan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(std::numeric_limits<usize>::max(), 17, kResidentLimit);
  REQUIRE_FALSE(overflowPlan.valid);
  REQUIRE(overflowPlan.overflow);
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: checkpointed true 2D Y pass matches the resident recurrence", "[ImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize kDimX = 7;
  constexpr usize kDimY = 29;
  const SizeVec3 dims{kDimX, kDimY, 1};
  std::vector<float32> field(kDimX * kDimY);
  for(usize index = 0; index < field.size(); ++index)
  {
    field[index] = static_cast<float32>(std::sin(static_cast<double>(index) * 0.19) * 17.0 + static_cast<double>((index * 13) % 31));
  }
  const std::vector<float32> expected = RunPass(field, kDimX, kDimY, 1, 1, 2.25, 0.75, 1, true);

  DataStore<float32> source(ShapeType{1, kDimY, kDimX}, ShapeType{1}, 0.0f);
  REQUIRE(source.copyFromBuffer(0, field).valid());
  const auto plan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(kDimX, kDimY, 128ULL * 1024ULL, 1024);
  REQUIRE(plan.valid);
  REQUIRE(plan.blockCount > 1);
  using Checkpoint = ImageProcessing::detail::RecursiveGaussian2DCheckpoint;
  auto checkpointBackend = std::make_unique<TestTemporaryRecordStore>(sizeof(Checkpoint), plan.blockCount * plan.coreCols, plan.coreCols);
  std::atomic_bool shouldCancel{false};
  ImageProcessing::detail::GaussianTemporaryStore<Checkpoint> checkpoints(std::move(checkpointBackend), shouldCancel);
  std::vector<float32> actual(field.size(), 0.0f);
  const RecursiveGaussianPlaneAxis pass{1, 2.25, 0.75, 1, true};
  auto sink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> values) -> Result<> {
    for(usize row = 0; row < rowCount; ++row)
    {
      std::copy_n(values.data() + row * columnCount, columnCount, actual.data() + (yBegin + row) * kDimX + xBegin);
    }
    return {};
  };
  REQUIRE(ImageProcessing::RecursiveGaussian2DYAxisToSink<float32>(source, dims, pass, plan, checkpoints, shouldCancel, {}, sink).valid());
  REQUIRE(actual == expected);
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: overwide true 2D XY and YX cascades are byte-exact", "[ImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize kDimX = 100;
  constexpr usize kDimY = 8;
  const SizeVec3 dims{kDimX, kDimY, 1};
  std::vector<float32> field(kDimX * kDimY);
  for(usize index = 0; index < field.size(); ++index)
  {
    field[index] = static_cast<float32>(std::sin(static_cast<double>(index) * 0.11) * 23.0 + static_cast<double>((index * 7) % 19));
  }
  DataStore<float32> source(ShapeType{1, kDimY, kDimX}, ShapeType{1}, 0.0f);
  REQUIRE(source.copyFromBuffer(0, field).valid());
  const auto plan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(kDimX, kDimY, 5000);
  REQUIRE(plan.valid);
  REQUIRE(plan.coreCols == 1);
  const usize checkpointCount = std::max(plan.blockCount * plan.coreCols, ImageProcessing::detail::RecursiveGaussian2DXBlockCount(kDimX, plan.coreCols));

  SECTION("X then Y")
  {
    const RecursiveGaussianPlaneAxis first{0, 1.75, 0.8, 1, true};
    const RecursiveGaussianPlaneAxis second{1, 2.25, 1.2, 0, false};
    const std::vector<float32> expected = RunPlaneCascade(field, dims, first, second);
    DataStore<float32> work(ShapeType{1, kDimY, kDimX}, ShapeType{1}, 0.0f);
    auto checkpointBackend = std::make_unique<TestTemporaryRecordStore>(sizeof(ImageProcessing::detail::RecursiveGaussian2DCheckpoint), checkpointCount, plan.coreCols);
    std::atomic_bool shouldCancel{false};
    ImageProcessing::detail::GaussianTemporaryStore<ImageProcessing::detail::RecursiveGaussian2DCheckpoint> checkpoints(std::move(checkpointBackend), shouldCancel);
    std::vector<float32> actual(field.size(), 0.0f);
    auto sink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> values) -> Result<> {
      for(usize row = 0; row < rowCount; ++row)
      {
        std::copy_n(values.data() + row * columnCount, columnCount, actual.data() + (yBegin + row) * kDimX + xBegin);
      }
      return {};
    };
    REQUIRE(ImageProcessing::RecursiveGaussian2DXYCascadeToSink<float32>(source, work, checkpoints, dims, first, second, plan, shouldCancel, {}, sink).valid());
    REQUIRE(actual == expected);
  }

  SECTION("Y then X")
  {
    const RecursiveGaussianPlaneAxis first{1, 1.75, 0.8, 1, true};
    const RecursiveGaussianPlaneAxis second{0, 2.25, 1.2, 0, false};
    const std::vector<float32> expected = RunPlaneCascade(field, dims, first, second);
    DataStore<float32> work(ShapeType{1, kDimY, kDimX}, ShapeType{1}, 0.0f);
    auto checkpointBackend = std::make_unique<TestTemporaryRecordStore>(sizeof(ImageProcessing::detail::RecursiveGaussian2DCheckpoint), checkpointCount, plan.coreCols);
    std::atomic_bool shouldCancel{false};
    ImageProcessing::detail::GaussianTemporaryStore<ImageProcessing::detail::RecursiveGaussian2DCheckpoint> checkpoints(std::move(checkpointBackend), shouldCancel);
    std::vector<float32> actual(field.size(), 0.0f);
    auto sink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> values, nonstd::span<float32>) -> Result<> {
      for(usize row = 0; row < rowCount; ++row)
      {
        std::copy_n(values.data() + row * columnCount, columnCount, actual.data() + (yBegin + row) * kDimX + xBegin);
      }
      return {};
    };
    REQUIRE(ImageProcessing::RecursiveGaussian2DYXCascadeToSink<float32>(source, work, checkpoints, dims, first, second, plan, shouldCancel, {}, sink).valid());
    REQUIRE(actual == expected);
  }
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: working storage selection is endpoint aware", "[ImageProcessing][RecursiveGaussianEngine]")
{
  // The three filter facades use PreflightImageFilter, whose output deliberately inherits the input data format; an
  // OOC-input/resident-output filter execution therefore cannot be constructed through their public parameters. Keep
  // all four endpoint combinations covered here because the storage-neutral engine also defends callers that construct
  // stores directly or use adaptive output placement outside those facades.
  constexpr const char* kInputOocFormat = "Input-OOC";
  constexpr const char* kOutputOocFormat = "Output-OOC";
  REQUIRE_FALSE(ImageProcessing::detail::SelectRecursiveGaussianStoragePlan(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::InMemory, "Alternate-DataStore")
                    .useTemporaryRecordStore);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianStoragePlan(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::OutOfCore, kOutputOocFormat)
              .useTemporaryRecordStore);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianStoragePlan(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore)
              .useTemporaryRecordStore);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianStoragePlan(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::OutOfCore, kOutputOocFormat).useTemporaryRecordStore);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianWorkingDataFormat(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::InMemory, "Alternate-DataStore") ==
          DataStore<float32>::k_DataStore);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianWorkingDataFormat(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::OutOfCore, kOutputOocFormat) ==
          kOutputOocFormat);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianWorkingDataFormat(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore) ==
          kInputOocFormat);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianWorkingDataFormat(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::OutOfCore, kOutputOocFormat) == kInputOocFormat);
}

TEST_CASE("ImageProcessing::GaussianTemporaryStore: performs typed bulk I/O and rejects backend failures", "[ImageProcessing][RecursiveGaussianEngine]")
{
  std::atomic_bool shouldCancel{false};
  auto backend = std::make_unique<TestTemporaryRecordStore>(sizeof(float32), 12, 4);
  auto* backendPtr = backend.get();
  ImageProcessing::detail::GaussianTemporaryStore<float32> store(std::move(backend), shouldCancel);
  REQUIRE(store.getSize() == 12);

  const std::array<float32, 6> written = {1.25f, -2.5f, 3.75f, 9.0f, -11.0f, 13.5f};
  REQUIRE(store.copyFromBuffer(3, nonstd::span<const float32>(written.data(), written.size())).valid());
  std::array<float32, 6> read{};
  REQUIRE(store.copyIntoBuffer(3, nonstd::span<float32>(read.data(), read.size())).valid());
  REQUIRE(read == written);
  REQUIRE(backendPtr->writeCalls == 2);
  REQUIRE(backendPtr->readCalls == 2);

  REQUIRE(store.copyIntoBuffer(10, nonstd::span<float32>(read.data(), 3)).invalid());
  REQUIRE(backendPtr->readCalls == 2);

  backendPtr->failWrite = true;
  REQUIRE(store.copyFromBuffer(3, nonstd::span<const float32>(written.data(), written.size())).invalid());
  backendPtr->failWrite = false;
  backendPtr->failRead = true;
  REQUIRE(store.copyIntoBuffer(3, nonstd::span<float32>(read.data(), read.size())).invalid());
  backendPtr->failRead = false;
  backendPtr->shortRead = true;
  REQUIRE(store.copyIntoBuffer(3, nonstd::span<float32>(read.data(), read.size())).invalid());
}

TEMPLATE_TEST_CASE("ImageProcessing::RecursiveGaussianEngine: XY plane cascade matches sequential float32 axis passes", "[ImageProcessing][RecursiveGaussianEngine]", int16, float32, float64)
{
  using T = TestType;
  const uint32 firstAxis = GENERATE(0u, 1u);
  const int firstOrder = GENERATE(0, 1, 2);
  const usize dimZ = GENERATE(1ULL, 3ULL);
  const SizeVec3 dims{7, 9, dimZ};
  CAPTURE(firstAxis, firstOrder, dimZ);

  std::vector<T> field(dims[0] * dims[1] * dims[2]);
  for(usize i = 0; i < field.size(); ++i)
  {
    const double value = std::sin(static_cast<double>(i) * 0.37) * 31.0 + static_cast<double>((i * 13 + i / 7) % 19);
    field[i] = static_cast<T>(value);
  }

  const uint32 secondAxis = 1u - firstAxis;
  const RecursiveGaussianPlaneAxis first{firstAxis, 1.75, firstAxis == 0 ? 0.6 : 1.4, firstOrder, true};
  const RecursiveGaussianPlaneAxis second{secondAxis, 2.25, secondAxis == 0 ? 0.8 : 1.7, 0, false};

  DataStore<T> source(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, T{});
  for(usize i = 0; i < field.size(); ++i)
  {
    source.setValue(i, field[i]);
  }
  DataStore<float32> intermediate(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 0.0f);
  DataStore<float32> expectedStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 0.0f);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  RecursiveGaussianPassOptions streamedOptions;
  streamedOptions.useInMemoryFastPath = false;
  REQUIRE(
      RecursiveGaussianAxisPass<T>(source, intermediate, dims, first.axis, first.sigma, first.spacing, first.order, first.normalizeAcrossScale, shouldCancel, messageHandler, streamedOptions).valid());
  REQUIRE(RecursiveGaussianAxisPass<float32>(intermediate, expectedStore, dims, second.axis, second.sigma, second.spacing, second.order, second.normalizeAcrossScale, shouldCancel, messageHandler,
                                             streamedOptions)
              .valid());

  const std::vector<float32> actual = RunPlaneCascade(field, dims, first, second);
  REQUIRE(actual.size() == expectedStore.getSize());
  for(usize i = 0; i < actual.size(); ++i)
  {
    REQUIRE(actual[i] == expectedStore.getValue(i));
  }
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: XY plane cascade honors cancellation and propagates sink errors", "[ImageProcessing][RecursiveGaussianEngine]")
{
  const SizeVec3 dims{7, 9, 3};
  DataStore<float32> source(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 2.0f);
  const RecursiveGaussianPlaneAxis first{0, 1.5, 0.75, 1, false};
  const RecursiveGaussianPlaneAxis second{1, 2.0, 1.25, 0, false};
  IFilter::MessageHandler messageHandler{};

  SECTION("pre-cancelled")
  {
    std::atomic_bool shouldCancel{true};
    usize sinkCalls = 0;
    auto sink = [&](usize, nonstd::span<float32>) -> Result<> {
      ++sinkCalls;
      return {};
    };
    REQUIRE(RecursiveGaussianXYPlaneCascadeToSink<float32>(source, dims, first, second, shouldCancel, messageHandler, sink).valid());
    REQUIRE(sinkCalls == 0);
  }

  SECTION("sink error")
  {
    std::atomic_bool shouldCancel{false};
    usize sinkCalls = 0;
    auto sink = [&](usize, nonstd::span<float32>) -> Result<> {
      ++sinkCalls;
      return MakeErrorResult(-23610, "Injected recursive-Gaussian sink failure");
    };
    const Result<> result = RecursiveGaussianXYPlaneCascadeToSink<float32>(source, dims, first, second, shouldCancel, messageHandler, sink);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -23610);
    REQUIRE(sinkCalls == 1);
  }
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: XY plane cascade materializes float32 between recurrences", "[ImageProcessing][RecursiveGaussianEngine]")
{
  const SizeVec3 dims{7, 9, 1};
  std::vector<float64> field(dims[0] * dims[1]);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = std::sin(static_cast<double>(i) * 0.73) * 1000000.0 + static_cast<double>(i * i) * 0.01;
  }

  const RecursiveGaussianPlaneAxis first{0, 1.37, 0.7, 1, true};
  const RecursiveGaussianPlaneAxis second{1, 2.11, 1.3, 0, false};
  const std::vector<float32> materialized = RunPlaneCascade(field, dims, first, second);

  const auto firstCoefficients = ImageProcessing::detail::ComputeRecursiveGaussianCoefficients(first.sigma, first.spacing, first.order, first.normalizeAcrossScale);
  const auto secondCoefficients = ImageProcessing::detail::ComputeRecursiveGaussianCoefficients(second.sigma, second.spacing, second.order, second.normalizeAcrossScale);
  std::vector<double> firstDouble(field.size());
  std::vector<double> secondDouble(field.size());
  std::vector<double> lineData(std::max(dims[0], dims[1]));
  std::vector<double> lineOutput(lineData.size());

  for(usize y = 0; y < dims[1]; ++y)
  {
    const usize offset = y * dims[0];
    std::copy_n(field.data() + offset, dims[0], lineData.data());
    ImageProcessing::detail::FilterDataArray(lineOutput.data(), lineData.data(), dims[0], firstCoefficients);
    std::copy_n(lineOutput.data(), dims[0], firstDouble.data() + offset);
  }
  for(usize x = 0; x < dims[0]; ++x)
  {
    for(usize y = 0; y < dims[1]; ++y)
    {
      lineData[y] = firstDouble[y * dims[0] + x];
    }
    ImageProcessing::detail::FilterDataArray(lineOutput.data(), lineData.data(), dims[1], secondCoefficients);
    for(usize y = 0; y < dims[1]; ++y)
    {
      secondDouble[y * dims[0] + x] = lineOutput[y];
    }
  }

  bool differsFromDoubleIntermediate = false;
  for(usize i = 0; i < materialized.size(); ++i)
  {
    differsFromDoubleIntermediate = differsFromDoubleIntermediate || materialized[i] != static_cast<float32>(secondDouble[i]);
  }
  REQUIRE(differsFromDoubleIntermediate);
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: XY plane cascade rejects short filtered axes", "[ImageProcessing][RecursiveGaussianEngine]")
{
  const SizeVec3 dims = GENERATE(SizeVec3{3, 9, 1}, SizeVec3{7, 3, 2});
  DataStore<float32> source(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 2.0f);
  const RecursiveGaussianPlaneAxis first{0, 1.5, 1.0, 0, false};
  const RecursiveGaussianPlaneAxis second{1, 1.5, 1.0, 0, false};
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  usize sinkCalls = 0;
  auto sink = [&](usize, nonstd::span<float32>) -> Result<> {
    ++sinkCalls;
    return {};
  };

  const Result<> result = RecursiveGaussianXYPlaneCascadeToSink<float32>(source, dims, first, second, shouldCancel, messageHandler, sink);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().front().code == -23603);
  REQUIRE(sinkCalls == 0);
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: postprocessing planner aligns irregular OOC volumes", "[ImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize k_PlaneValues = 600 * 450;
  constexpr usize k_TotalValues = k_PlaneValues * 128;
  const ShapeType tupleShape{128, 450, 600};

  SECTION("one float32 buffer")
  {
    const Result<ImageProcessing::detail::PointwiseBatchPlan> result = ImageProcessing::detail::MakeRecursiveGaussianPostprocessingPlan(
        k_TotalValues, sizeof(float32), ImageProcessing::detail::k_PointwiseTargetScratchBytes,
        {{IDataStore::StoreType::OutOfCore, tupleShape, 1, "input"}, {IDataStore::StoreType::OutOfCore, tupleShape, 1, "output", ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(result.valid());
    REQUIRE(result.value().alignmentValues == k_PlaneValues);
    REQUIRE(result.value().batchValues == 62 * k_PlaneValues);
    REQUIRE(result.value().totalBatches == 3);
    REQUIRE((k_TotalValues - 2 * result.value().batchValues) == 4 * k_PlaneValues);
  }

  SECTION("two float32 buffers")
  {
    const Result<ImageProcessing::detail::PointwiseBatchPlan> result = ImageProcessing::detail::MakeRecursiveGaussianPostprocessingPlan(
        k_TotalValues, sizeof(float32) * 2, ImageProcessing::detail::k_PointwiseTargetScratchBytes,
        {{IDataStore::StoreType::OutOfCore, tupleShape, 1, "accumulator", ImageProcessing::detail::PointwiseEndpointLayout::Access::Write},
         {IDataStore::StoreType::OutOfCore, tupleShape, 1, "derivative"}});
    REQUIRE(result.valid());
    REQUIRE(result.value().alignmentValues == k_PlaneValues);
    REQUIRE(result.value().batchValues == 31 * k_PlaneValues);
    REQUIRE(result.value().totalBatches == 5);
    REQUIRE((k_TotalValues - 4 * result.value().batchValues) == 4 * k_PlaneValues);
  }

  SECTION("one complete slab is the minimum")
  {
    const Result<ImageProcessing::detail::PointwiseBatchPlan> result = ImageProcessing::detail::MakeRecursiveGaussianPostprocessingPlan(
        k_TotalValues, sizeof(float32) * 2, 0,
        {{IDataStore::StoreType::OutOfCore, tupleShape, 1, "accumulator", ImageProcessing::detail::PointwiseEndpointLayout::Access::Write},
         {IDataStore::StoreType::OutOfCore, tupleShape, 1, "derivative"}});
    REQUIRE(result.valid());
    REQUIRE(result.value().alignmentValues == k_PlaneValues);
    REQUIRE(result.value().batchValues == k_PlaneValues);
    REQUIRE(result.value().totalBatches == 128);
  }
}

TEST_CASE("ImageProcessing::RecursiveGaussianEngine: forced buffered postprocessing matches scalar formulas", "[ImageProcessing][RecursiveGaussianEngine]")
{
  const ShapeType tupleShape{3, 7, 9};
  constexpr usize k_TotalValues = 3 * 7 * 9;
  DataStore<float32> source(tupleShape, ShapeType{1}, 0.0f);
  for(usize i = 0; i < k_TotalValues; ++i)
  {
    source.setValue(i, static_cast<float32>(i % 17) - 8.0f);
  }
  std::atomic_bool shouldCancel{false};
  ImageProcessing::detail::PointwiseExecutionOptions options;
  options.forceBuffered = true;
  options.targetScratchBytes = 53;

  DataStore<float32> squared(tupleShape, ShapeType{1}, -1.0f);
  REQUIRE(SetSquaredOverSpacing(squared, source, 1.25, shouldCancel, options).valid());
  for(usize i = 0; i < k_TotalValues; ++i)
  {
    const double value = static_cast<double>(source.getValue(i)) / 1.25;
    REQUIRE(squared.getValue(i) == static_cast<float32>(value * value));
  }

  DataStore<float32> accumulatedSquared(tupleShape, ShapeType{1}, 2.0f);
  REQUIRE(AccumulateSquaredOverSpacing(accumulatedSquared, source, 1.25, shouldCancel, options).valid());
  for(usize i = 0; i < k_TotalValues; ++i)
  {
    const double value = static_cast<double>(source.getValue(i)) / 1.25;
    REQUIRE(accumulatedSquared.getValue(i) == static_cast<float32>(2.0 + value * value));
  }

  DataStore<float32> scaled(tupleShape, ShapeType{1}, -1.0f);
  REQUIRE(SetScaledDerivative(scaled, source, 0.375, shouldCancel, options).valid());
  DataStore<float32> accumulatedScaled(tupleShape, ShapeType{1}, 2.0f);
  REQUIRE(AccumulateScaledDerivative(accumulatedScaled, source, 0.375, shouldCancel, options).valid());
  for(usize i = 0; i < k_TotalValues; ++i)
  {
    REQUIRE(scaled.getValue(i) == static_cast<float32>(static_cast<double>(source.getValue(i)) * 0.375));
    REQUIRE(accumulatedScaled.getValue(i) == static_cast<float32>(2.0 + static_cast<double>(source.getValue(i)) * 0.375));
  }

  REQUIRE(SqrtStoreInPlace(squared, shouldCancel, options).valid());
  DataStore<int16> cast(tupleShape, ShapeType{1}, int16{});
  REQUIRE(CastFloat32StoreTo<int16>(scaled, cast, shouldCancel, options).valid());
  for(usize i = 0; i < k_TotalValues; ++i)
  {
    const double value = static_cast<double>(source.getValue(i)) / 1.25;
    REQUIRE(squared.getValue(i) == static_cast<float32>(std::sqrt(static_cast<double>(static_cast<float32>(value * value)))));
    REQUIRE(cast.getValue(i) == static_cast<int16>(scaled.getValue(i)));
  }

  std::atomic_bool cancelled{true};
  DataStore<float32> untouched(tupleShape, ShapeType{1}, 17.0f);
  REQUIRE(SetSquaredOverSpacing(untouched, source, 1.25, cancelled, options).valid());
  for(usize i = 0; i < k_TotalValues; ++i)
  {
    REQUIRE(untouched.getValue(i) == 17.0f);
  }
}
