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
  Result<uint64> read(uint64 recordOffset, uint64 requestedRecordCount, nonstd::span<std::byte> records, const std::atomic_bool& /*shouldCancel*/) const override
  {
    read_calls++;
    if(fail_read)
    {
      return MakeErrorResult<uint64>(-7650, "Injected Gaussian scratch read failure");
    }
    const uint64 returnedRecords = short_read && requestedRecordCount > 0 ? requestedRecordCount - 1 : requestedRecordCount;
    std::memcpy(records.data(), m_Bytes.data() + static_cast<usize>(recordOffset * m_RecordSize), static_cast<usize>(returnedRecords * m_RecordSize));
    return {returnedRecords};
  }
  Result<> write(uint64 recordOffset, uint64 recordCount, nonstd::span<const std::byte> records, const std::atomic_bool& /*shouldCancel*/) override
  {
    write_calls++;
    if(fail_write)
    {
      return MakeErrorResult(-7651, "Injected Gaussian scratch write failure");
    }
    std::memcpy(m_Bytes.data() + static_cast<usize>(recordOffset * m_RecordSize), records.data(), static_cast<usize>(recordCount * m_RecordSize));
    return {};
  }
  Result<> fill(uint64 /*recordOffset*/, uint64 /*recordCount*/, nonstd::span<const std::byte> /*record*/, const std::atomic_bool& /*shouldCancel*/) override
  {
    return MakeErrorResult(-7652, "Unexpected Gaussian scratch fill");
  }
  Result<> resize(uint64 /*recordCount*/, const std::atomic_bool& /*shouldCancel*/) override
  {
    return MakeErrorResult(-7653, "Unexpected Gaussian scratch resize");
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
  const Result<> r = RecursiveGaussianAxisPass<float32>(inStore, outStore, SizeVec3Type{dx, dy, dz}, axis, sigma, spacing, order, normalize, shouldCancel, messageHandler, options);
  REQUIRE(r.valid());
  std::vector<float32> out(field.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

template <class T>
std::vector<float32> RunPlaneCascade(const std::vector<T>& field, const SizeVec3Type& dims, const RecursiveGaussianPlaneAxis& first, const RecursiveGaussianPlaneAxis& second)
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

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: order-0 preserves a constant image", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  const usize d = 16;
  const std::vector<float32> field(d * d * d, 5.0f);
  const std::vector<float32> out = RunPass(field, d, d, d, /*axis=*/0, /*sigma=*/2.0, /*spacing=*/1.0, /*order=*/0, false);
  for(float32 v : out)
  {
    REQUIRE(v == Approx(5.0f).margin(1e-3));
  }
}

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: order-1 and order-2 of a constant image are ~0", "[NXImageProcessing][RecursiveGaussianEngine]")
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

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: order-0 impulse response is symmetric and sums to ~1", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  // A 1D-along-x impulse embedded in a 3D volume (single non-zero voxel at the x-center of one row).
  const usize dx = 32;
  const usize dy = 4;
  const usize dz = 4;
  std::vector<float32> field(dx * dy * dz, 0.0f);
  const usize cx = dx / 2;
  const usize cy = dy / 2;
  const usize cz = dz / 2;
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

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: smoothing along X leaves a pure-Y ramp unchanged", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  const usize dx = 12;
  const usize dy = 12;
  const usize dz = 4;
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

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: deterministic + tall-Z streaming runs", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  const usize dx = 8;
  const usize dy = 8;
  const usize dz = 64; // dz large enough to exercise the z-slab path
  std::vector<float32> field(dx * dy * dz);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<float32>((i * 37) % 101);
  }
  const std::vector<float32> a = RunPass(field, dx, dy, dz, /*axis=*/2, 2.5, 1.0, 0, false);
  const std::vector<float32> b = RunPass(field, dx, dy, dz, /*axis=*/2, 2.5, 1.0, 0, false);
  REQUIRE(a == b); // byte-identical across runs
}

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: batched Z streaming matches direct memory", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 11;
  constexpr usize k_DimZ = 13;
  std::vector<float32> field(k_DimX * k_DimY * k_DimZ);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<float32>((i * 37 + i / 5) % 101);
  }

  const std::vector<float32> direct = RunPass(field, k_DimX, k_DimY, k_DimZ, /*axis=*/2, 2.5, 1.0, 0, false);
  RecursiveGaussianPassOptions streamingOptions;
  streamingOptions.staging_byte_budget = 1600; // Two Y rows per batch, leaving a one-row final batch.
  streamingOptions.use_in_memory_fast_path = false;
  const std::vector<float32> streamed = RunPass(field, k_DimX, k_DimY, k_DimZ, /*axis=*/2, 2.5, 1.0, 0, false, streamingOptions);
  REQUIRE(streamed == direct);
}

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: Z planner scales the supplied budget and keeps a one-row minimum", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  const SizeVec3Type benchmarkDims{512, 512, 128};
  REQUIRE(ImageProcessing::detail::ComputeRecursiveGaussianZBatchRowCount<float32>(benchmarkDims, RecursiveGaussianPassOptions::k_FallbackStagingBytes) == 128);
  REQUIRE(ImageProcessing::detail::ComputeRecursiveGaussianZBatchRowCount<float32>(benchmarkDims, RecursiveGaussianPassOptions::k_FallbackStagingBytes * 2) == 256);
  REQUIRE(ImageProcessing::detail::ComputeRecursiveGaussianZBatchRowCount<float32>(benchmarkDims, 0) == 1);
  REQUIRE(ImageProcessing::detail::ComputeRecursiveGaussianZBatchRowCount<float64>(SizeVec3Type{7, 11, 13}, 1000) == 1); // One row pair requires 7 * 13 * (8 + 4) = 1092 bytes.
}

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: true 2D planner bounds row blocks and overwide column tiles", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize k_ResidentLimit = 64ULL * 1024ULL * 1024ULL;
  const auto benchmarkPlan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(5824, 5824, k_ResidentLimit);
  REQUIRE(benchmarkPlan.valid);
  REQUIRE_FALSE(benchmarkPlan.overflow);
  REQUIRE(benchmarkPlan.core_cols == 5824);
  REQUIRE(benchmarkPlan.block_rows >= 8);
  REQUIRE(benchmarkPlan.block_rows < 5824);
  REQUIRE(benchmarkPlan.block_count > 1);
  REQUIRE(benchmarkPlan.resident_bytes <= k_ResidentLimit);

  const auto tiledPlan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(16385, 17, 65536);
  REQUIRE(tiledPlan.valid);
  REQUIRE_FALSE(tiledPlan.overflow);
  REQUIRE(tiledPlan.core_cols > 0);
  REQUIRE(tiledPlan.core_cols < 16385);
  REQUIRE(tiledPlan.block_rows >= 4);
  REQUIRE(tiledPlan.resident_bytes <= 65536);

  REQUIRE_FALSE(ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(16385, 17, 4096).valid);
  const auto overflowPlan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(std::numeric_limits<usize>::max(), 17, k_ResidentLimit);
  REQUIRE_FALSE(overflowPlan.valid);
  REQUIRE(overflowPlan.overflow);
}

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: checkpointed true 2D Y pass matches the resident recurrence", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 29;
  const SizeVec3Type dims{k_DimX, k_DimY, 1};
  std::vector<float32> field(k_DimX * k_DimY);
  for(usize index = 0; index < field.size(); ++index)
  {
    field[index] = static_cast<float32>(std::sin(static_cast<double>(index) * 0.19) * 17.0 + static_cast<double>((index * 13) % 31));
  }
  const std::vector<float32> expected = RunPass(field, k_DimX, k_DimY, 1, 1, 2.25, 0.75, 1, true);

  DataStore<float32> source(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
  REQUIRE(source.copyFromBuffer(0, field).valid());
  const auto plan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(k_DimX, k_DimY, 128ULL * 1024ULL, 1024);
  REQUIRE(plan.valid);
  REQUIRE(plan.block_count > 1);
  using CheckpointType = ImageProcessing::detail::RecursiveGaussian2DCheckpoint;
  auto checkpointBackend = std::make_unique<TestTemporaryRecordStore>(sizeof(CheckpointType), plan.block_count * plan.core_cols, plan.core_cols);
  std::atomic_bool shouldCancel{false};
  ImageProcessing::detail::GaussianTemporaryStore<CheckpointType> checkpoints(std::move(checkpointBackend), shouldCancel);
  std::vector<float32> actual(field.size(), 0.0f);
  const RecursiveGaussianPlaneAxis pass{.axis = 1, .sigma = 2.25, .spacing = 0.75, .order = 1, .normalize_across_scale = true};
  auto sink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> values) -> Result<> {
    for(usize row = 0; row < rowCount; ++row)
    {
      std::copy_n(values.data() + row * columnCount, columnCount, actual.data() + (yBegin + row) * k_DimX + xBegin);
    }
    return {};
  };
  REQUIRE(ImageProcessing::RecursiveGaussian2DYAxisToSink<float32>(source, dims, pass, plan, checkpoints, shouldCancel, {}, sink).valid());
  REQUIRE(actual == expected);
}

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: overwide true 2D XY and YX cascades are byte-exact", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize k_DimX = 100;
  constexpr usize k_DimY = 8;
  const SizeVec3Type dims{k_DimX, k_DimY, 1};
  std::vector<float32> field(k_DimX * k_DimY);
  for(usize index = 0; index < field.size(); ++index)
  {
    field[index] = static_cast<float32>(std::sin(static_cast<double>(index) * 0.11) * 23.0 + static_cast<double>((index * 7) % 19));
  }
  DataStore<float32> source(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
  REQUIRE(source.copyFromBuffer(0, field).valid());
  const auto plan = ImageProcessing::detail::BuildRecursiveGaussian2DBufferPlan(k_DimX, k_DimY, 5000);
  REQUIRE(plan.valid);
  REQUIRE(plan.core_cols == 1);
  const usize checkpointCount = std::max(plan.block_count * plan.core_cols, ImageProcessing::detail::RecursiveGaussian2DXBlockCount(k_DimX, plan.core_cols));

  SECTION("X then Y")
  {
    const RecursiveGaussianPlaneAxis first{.axis = 0, .sigma = 1.75, .spacing = 0.8, .order = 1, .normalize_across_scale = true};
    const RecursiveGaussianPlaneAxis second{.axis = 1, .sigma = 2.25, .spacing = 1.2, .order = 0, .normalize_across_scale = false};
    const std::vector<float32> expected = RunPlaneCascade(field, dims, first, second);
    DataStore<float32> work(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
    auto checkpointBackend = std::make_unique<TestTemporaryRecordStore>(sizeof(ImageProcessing::detail::RecursiveGaussian2DCheckpoint), checkpointCount, plan.core_cols);
    std::atomic_bool shouldCancel{false};
    ImageProcessing::detail::GaussianTemporaryStore<ImageProcessing::detail::RecursiveGaussian2DCheckpoint> checkpoints(std::move(checkpointBackend), shouldCancel);
    std::vector<float32> actual(field.size(), 0.0f);
    auto sink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> values) -> Result<> {
      for(usize row = 0; row < rowCount; ++row)
      {
        std::copy_n(values.data() + row * columnCount, columnCount, actual.data() + (yBegin + row) * k_DimX + xBegin);
      }
      return {};
    };
    REQUIRE(ImageProcessing::RecursiveGaussian2DXYCascadeToSink<float32>(source, work, checkpoints, dims, first, second, plan, shouldCancel, {}, sink).valid());
    REQUIRE(actual == expected);
  }

  SECTION("Y then X")
  {
    const RecursiveGaussianPlaneAxis first{.axis = 1, .sigma = 1.75, .spacing = 0.8, .order = 1, .normalize_across_scale = true};
    const RecursiveGaussianPlaneAxis second{.axis = 0, .sigma = 2.25, .spacing = 1.2, .order = 0, .normalize_across_scale = false};
    const std::vector<float32> expected = RunPlaneCascade(field, dims, first, second);
    DataStore<float32> work(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
    auto checkpointBackend = std::make_unique<TestTemporaryRecordStore>(sizeof(ImageProcessing::detail::RecursiveGaussian2DCheckpoint), checkpointCount, plan.core_cols);
    std::atomic_bool shouldCancel{false};
    ImageProcessing::detail::GaussianTemporaryStore<ImageProcessing::detail::RecursiveGaussian2DCheckpoint> checkpoints(std::move(checkpointBackend), shouldCancel);
    std::vector<float32> actual(field.size(), 0.0f);
    auto sink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> values, nonstd::span<float32>) -> Result<> {
      for(usize row = 0; row < rowCount; ++row)
      {
        std::copy_n(values.data() + row * columnCount, columnCount, actual.data() + (yBegin + row) * k_DimX + xBegin);
      }
      return {};
    };
    REQUIRE(ImageProcessing::RecursiveGaussian2DYXCascadeToSink<float32>(source, work, checkpoints, dims, first, second, plan, shouldCancel, {}, sink).valid());
    REQUIRE(actual == expected);
  }
}

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: working storage selection is endpoint aware", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  // The three filter facades use PreflightImageFilter, whose output deliberately inherits the input data format; an
  // OOC-input/resident-output filter execution therefore cannot be constructed through their public parameters. Keep
  // all four endpoint combinations covered here because the storage-neutral engine also defends callers that construct
  // stores directly or use adaptive output placement outside those facades.
  constexpr const char* k_InputOocFormat = "Input-OOC";
  constexpr const char* k_OutputOocFormat = "Output-OOC";
  REQUIRE_FALSE(ImageProcessing::detail::SelectRecursiveGaussianStoragePlan(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::InMemory, "Alternate-DataStore")
                    .use_temporary_record_store);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianStoragePlan(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::OutOfCore, k_OutputOocFormat)
              .use_temporary_record_store);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianStoragePlan(IDataStore::StoreType::OutOfCore, k_InputOocFormat, IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore)
              .use_temporary_record_store);
  REQUIRE(
      ImageProcessing::detail::SelectRecursiveGaussianStoragePlan(IDataStore::StoreType::OutOfCore, k_InputOocFormat, IDataStore::StoreType::OutOfCore, k_OutputOocFormat).use_temporary_record_store);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianWorkingDataFormat(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::InMemory, "Alternate-DataStore") ==
          DataStore<float32>::k_DataStore);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianWorkingDataFormat(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore, IDataStore::StoreType::OutOfCore, k_OutputOocFormat) ==
          k_OutputOocFormat);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianWorkingDataFormat(IDataStore::StoreType::OutOfCore, k_InputOocFormat, IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore) ==
          k_InputOocFormat);
  REQUIRE(ImageProcessing::detail::SelectRecursiveGaussianWorkingDataFormat(IDataStore::StoreType::OutOfCore, k_InputOocFormat, IDataStore::StoreType::OutOfCore, k_OutputOocFormat) ==
          k_InputOocFormat);
}

TEST_CASE("NXImageProcessing::GaussianTemporaryStore: performs typed bulk I/O and rejects backend failures", "[NXImageProcessing][RecursiveGaussianEngine]")
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

TEMPLATE_TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: XY plane cascade matches sequential float32 axis passes", "[NXImageProcessing][RecursiveGaussianEngine]", int16, float32, float64)
{
  using T = TestType;
  const uint32 firstAxis = GENERATE(0u, 1u);
  const int firstOrder = GENERATE(0, 1, 2);
  const usize dimZ = GENERATE(1ULL, 3ULL);
  const SizeVec3Type dims{7, 9, dimZ};
  CAPTURE(firstAxis, firstOrder, dimZ);

  std::vector<T> field(dims[0] * dims[1] * dims[2]);
  for(usize i = 0; i < field.size(); ++i)
  {
    const double value = std::sin(static_cast<double>(i) * 0.37) * 31.0 + static_cast<double>((i * 13 + i / 7) % 19);
    field[i] = static_cast<T>(value);
  }

  const uint32 secondAxis = 1u - firstAxis;
  const RecursiveGaussianPlaneAxis first{.axis = firstAxis, .sigma = 1.75, .spacing = firstAxis == 0 ? 0.6 : 1.4, .order = firstOrder, .normalize_across_scale = true};
  const RecursiveGaussianPlaneAxis second{.axis = secondAxis, .sigma = 2.25, .spacing = secondAxis == 0 ? 0.8 : 1.7, .order = 0, .normalize_across_scale = false};

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
  streamedOptions.use_in_memory_fast_path = false;
  REQUIRE(RecursiveGaussianAxisPass<T>(source, intermediate, dims, first.axis, first.sigma, first.spacing, first.order, first.normalize_across_scale, shouldCancel, messageHandler, streamedOptions)
              .valid());
  REQUIRE(RecursiveGaussianAxisPass<float32>(intermediate, expectedStore, dims, second.axis, second.sigma, second.spacing, second.order, second.normalize_across_scale, shouldCancel, messageHandler,
                                             streamedOptions)
              .valid());

  const std::vector<float32> actual = RunPlaneCascade(field, dims, first, second);
  REQUIRE(actual.size() == expectedStore.getSize());
  for(usize i = 0; i < actual.size(); ++i)
  {
    REQUIRE(actual[i] == expectedStore.getValue(i));
  }
}

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: XY plane cascade honors cancellation and propagates sink errors", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  const SizeVec3Type dims{7, 9, 3};
  DataStore<float32> source(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 2.0f);
  const RecursiveGaussianPlaneAxis first{.axis = 0, .sigma = 1.5, .spacing = 0.75, .order = 1, .normalize_across_scale = false};
  const RecursiveGaussianPlaneAxis second{.axis = 1, .sigma = 2.0, .spacing = 1.25, .order = 0, .normalize_across_scale = false};
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

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: XY plane cascade materializes float32 between recurrences", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  const SizeVec3Type dims{7, 9, 1};
  std::vector<float64> field(dims[0] * dims[1]);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = std::sin(static_cast<double>(i) * 0.73) * 1000000.0 + static_cast<double>(i * i) * 0.01;
  }

  const RecursiveGaussianPlaneAxis first{.axis = 0, .sigma = 1.37, .spacing = 0.7, .order = 1, .normalize_across_scale = true};
  const RecursiveGaussianPlaneAxis second{.axis = 1, .sigma = 2.11, .spacing = 1.3, .order = 0, .normalize_across_scale = false};
  const std::vector<float32> materialized = RunPlaneCascade(field, dims, first, second);

  const auto firstCoefficients = ImageProcessing::detail::ComputeRecursiveGaussianCoefficients(first.sigma, first.spacing, first.order, first.normalize_across_scale);
  const auto secondCoefficients = ImageProcessing::detail::ComputeRecursiveGaussianCoefficients(second.sigma, second.spacing, second.order, second.normalize_across_scale);
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

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: XY plane cascade rejects short filtered axes", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  const SizeVec3Type dims = GENERATE(SizeVec3Type{3, 9, 1}, SizeVec3Type{7, 3, 2});
  DataStore<float32> source(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 2.0f);
  const RecursiveGaussianPlaneAxis first{.axis = 0, .sigma = 1.5, .spacing = 1.0, .order = 0, .normalize_across_scale = false};
  const RecursiveGaussianPlaneAxis second{.axis = 1, .sigma = 1.5, .spacing = 1.0, .order = 0, .normalize_across_scale = false};
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

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: postprocessing planner aligns irregular OOC volumes", "[NXImageProcessing][RecursiveGaussianEngine]")
{
  constexpr usize k_PlaneValues = 600 * 450;
  constexpr usize k_TotalValues = k_PlaneValues * 128;
  const ShapeType tupleShape{128, 450, 600};

  SECTION("one float32 buffer")
  {
    const Result<ImageProcessing::detail::PointwiseBatchPlan> result =
        ImageProcessing::detail::MakeRecursiveGaussianPostprocessingPlan(k_TotalValues, sizeof(float32), ImageProcessing::detail::k_PointwiseTargetScratchBytes,
                                                                         {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 1, .name = "input"},
                                                                          {.store_type = IDataStore::StoreType::OutOfCore,
                                                                           .tuple_shape = tupleShape,
                                                                           .num_components = 1,
                                                                           .name = "output",
                                                                           .access = ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(result.valid());
    REQUIRE(result.value().alignment_values == k_PlaneValues);
    REQUIRE(result.value().batch_values == 62 * k_PlaneValues);
    REQUIRE(result.value().total_batches == 3);
    REQUIRE((k_TotalValues - 2 * result.value().batch_values) == 4 * k_PlaneValues);
  }

  SECTION("two float32 buffers")
  {
    const Result<ImageProcessing::detail::PointwiseBatchPlan> result =
        ImageProcessing::detail::MakeRecursiveGaussianPostprocessingPlan(k_TotalValues, sizeof(float32) * 2, ImageProcessing::detail::k_PointwiseTargetScratchBytes,
                                                                         {{.store_type = IDataStore::StoreType::OutOfCore,
                                                                           .tuple_shape = tupleShape,
                                                                           .num_components = 1,
                                                                           .name = "accumulator",
                                                                           .access = ImageProcessing::detail::PointwiseEndpointLayout::Access::Write},
                                                                          {.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 1, .name = "derivative"}});
    REQUIRE(result.valid());
    REQUIRE(result.value().alignment_values == k_PlaneValues);
    REQUIRE(result.value().batch_values == 31 * k_PlaneValues);
    REQUIRE(result.value().total_batches == 5);
    REQUIRE((k_TotalValues - 4 * result.value().batch_values) == 4 * k_PlaneValues);
  }

  SECTION("one complete slab is the minimum")
  {
    const Result<ImageProcessing::detail::PointwiseBatchPlan> result =
        ImageProcessing::detail::MakeRecursiveGaussianPostprocessingPlan(k_TotalValues, sizeof(float32) * 2, 0,
                                                                         {{.store_type = IDataStore::StoreType::OutOfCore,
                                                                           .tuple_shape = tupleShape,
                                                                           .num_components = 1,
                                                                           .name = "accumulator",
                                                                           .access = ImageProcessing::detail::PointwiseEndpointLayout::Access::Write},
                                                                          {.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 1, .name = "derivative"}});
    REQUIRE(result.valid());
    REQUIRE(result.value().alignment_values == k_PlaneValues);
    REQUIRE(result.value().batch_values == k_PlaneValues);
    REQUIRE(result.value().total_batches == 128);
  }
}

TEST_CASE("NXImageProcessing::RecursiveGaussianEngine: forced buffered postprocessing matches scalar formulas", "[NXImageProcessing][RecursiveGaussianEngine]")
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
  options.force_buffered = true;
  options.target_scratch_bytes = 53;

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
