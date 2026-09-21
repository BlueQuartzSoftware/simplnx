#include "simplnx/Utilities/ImageProcessing/BinaryThinningEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <limits>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
template <class T>
class TransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    m_ReadValues += buffer.size();
    m_MaximumReadValues = std::max(m_MaximumReadValues, buffer.size());
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++m_WriteCount;
    m_WrittenValues += buffer.size();
    m_MaximumWriteValues = std::max(m_MaximumWriteValues, buffer.size());
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

  usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

  usize readValues() const noexcept
  {
    return m_ReadValues;
  }

  usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

  usize maximumReadValues() const noexcept
  {
    return m_MaximumReadValues;
  }

  usize maximumWriteValues() const noexcept
  {
    return m_MaximumWriteValues;
  }

private:
  mutable usize m_ReadCount = 0;
  mutable usize m_ReadValues = 0;
  usize m_WriteCount = 0;
  usize m_WrittenValues = 0;
  mutable usize m_MaximumReadValues = 0;
  usize m_MaximumWriteValues = 0;
};

template <class T>
class CancelAfterFirstReadDataStore : public DataStore<T>
{
public:
  CancelAfterFirstReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue, std::atomic_bool& shouldCancel)
  : DataStore<T>(tupleShape, componentShape, initialValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    m_ShouldCancel = true;
    return result;
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  mutable usize m_ReadCount = 0;
};

template <class T>
class FailOnSecondReadDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    if(m_ReadCount == 2)
    {
      return MakeErrorResult(-8718, "Injected binary-thinning scratch read failure.");
    }
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

private:
  mutable usize m_ReadCount = 0;
};

template <class T>
class FailOnFirstWriteDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++m_WriteCount;
    if(m_WriteCount == 1)
    {
      return MakeErrorResult(-8719, "Injected binary-thinning output write failure.");
    }
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

private:
  usize m_WriteCount = 0;
};

template <class T>
std::vector<T> RunThinning(const std::vector<T>& mask, usize dx, usize dy, usize dz)
{
  DataStore<T> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, T{});
  for(usize i = 0; i < mask.size(); ++i)
  {
    inStore.setValue(i, mask[i]);
  }
  DataStore<T> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, T{});
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> r = ApplyBinaryThinning<T>(inStore, outStore, SizeVec3{dx, dy, dz}, shouldCancel, messageHandler);
  REQUIRE(r.valid());
  std::vector<T> out(mask.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}
} // namespace

TEST_CASE("ImageProcessing::BinaryThinningEngine: output is binary and a skeleton is idempotent", "[ImageProcessing][BinaryThinningEngine]")
{
  // A solid 5x5 block thins to a thin skeleton; thinning the result again is a no-op.
  const usize dx = 7, dy = 7, dz = 1;
  std::vector<int32> mask(dx * dy, 0);
  for(usize y = 1; y <= 5; ++y)
  {
    for(usize x = 1; x <= 5; ++x)
    {
      mask[y * dx + x] = 1;
    }
  }
  const std::vector<int32> sk = RunThinning<int32>(mask, dx, dy, dz);
  for(int32 v : sk)
  {
    REQUIRE((v == 0 || v == 1)); // binary output
  }
  REQUIRE(RunThinning<int32>(sk, dx, dy, dz) == sk); // idempotent on an already-thinned image
  // The skeleton has strictly fewer foreground pixels than the solid block (thinning removed boundary).
  const auto count = [](const std::vector<int32>& v) {
    usize c = 0;
    for(int32 e : v)
    {
      c += (e != 0);
    }
    return c;
  };
  REQUIRE(count(sk) < count(mask));
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: uint8 solid block is binary and idempotent", "[ImageProcessing][BinaryThinningEngine]")
{
  // Guard the T{} / static_cast<T> conversions on an unsigned type: the solid-block skeleton is binary (0/1) and idempotent.
  const usize dx = 7, dy = 7, dz = 1;
  std::vector<uint8> mask(dx * dy, 0);
  for(usize y = 1; y <= 5; ++y)
  {
    for(usize x = 1; x <= 5; ++x)
    {
      mask[y * dx + x] = 1;
    }
  }
  const std::vector<uint8> sk = RunThinning<uint8>(mask, dx, dy, dz);
  for(uint8 v : sk)
  {
    REQUIRE((v == 0 || v == 1)); // binary output
  }
  REQUIRE(RunThinning<uint8>(sk, dx, dy, dz) == sk); // idempotent on an already-thinned image
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: a horizontal bar 1px tall is preserved (already thin)", "[ImageProcessing][BinaryThinningEngine]")
{
  const usize dx = 7, dy = 3, dz = 1;
  std::vector<int32> mask(dx * dy, 0);
  for(usize x = 1; x <= 5; ++x)
  {
    mask[1 * dx + x] = 1; // middle row bar
  }
  const std::vector<int32> sk = RunThinning<int32>(mask, dx, dy, dz);
  REQUIRE(sk == mask); // a 1px-thick bar is its own skeleton
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: degenerate inputs (empty, single pixel)", "[ImageProcessing][BinaryThinningEngine]")
{
  REQUIRE(RunThinning<int32>(std::vector<int32>(9, 0), 3, 3, 1) == std::vector<int32>(9, 0)); // empty stays empty
  std::vector<int32> single(9, 0);
  single[4] = 1;
  REQUIRE(RunThinning<int32>(single, 3, 3, 1) == single); // isolated pixel is preserved (endpoint)
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: 3D thins each z-slice independently", "[ImageProcessing][BinaryThinningEngine]")
{
  // Two identical solid blocks on two z-slices -> the two output slices are identical (no z-coupling).
  const usize dx = 7, dy = 7, dz = 2;
  std::vector<int32> mask(dx * dy * dz, 0);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 1; y <= 5; ++y)
    {
      for(usize x = 1; x <= 5; ++x)
      {
        mask[(z * dy + y) * dx + x] = 1;
      }
    }
  }
  const std::vector<int32> out = RunThinning<int32>(mask, dx, dy, dz);
  const usize slice = dx * dy;
  for(usize i = 0; i < slice; ++i)
  {
    REQUIRE(out[i] == out[slice + i]); // slice 0 == slice 1
  }
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: checked 3D slice-batch memory plan", "[ImageProcessing][BinaryThinningEngine][WorkingMemory]")
{
  using ImageProcessing::detail::CreateBinaryThinningSliceBatchPlan;

  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 9;
  const SizeVec3 dims{k_DimX, k_DimY, k_DimZ};
  constexpr usize k_SliceValues = k_DimX * k_DimY;
  constexpr usize k_MarkerWords = (k_SliceValues + 63) / 64;
  constexpr usize k_BytesPerWorker = k_SliceValues * (sizeof(int32) + sizeof(uint8)) + k_MarkerWords * sizeof(uint64);
  const auto planResult = CreateBinaryThinningSliceBatchPlan<int32>(dims, 3 * k_BytesPerWorker, /*maximumWorkers=*/8);
  REQUIRE(planResult.valid());
  const auto& plan = planResult.value();
  REQUIRE(plan.sliceValues == k_SliceValues);
  REQUIRE(plan.markerWordsPerSlice == k_MarkerWords);
  REQUIRE(plan.bytesPerWorker == k_BytesPerWorker);
  REQUIRE(plan.workerCount == 3);
  REQUIRE(plan.residentBytes == 3 * k_BytesPerWorker);

  REQUIRE(CreateBinaryThinningSliceBatchPlan<int32>(dims, k_BytesPerWorker - 1, /*maximumWorkers=*/8).invalid());
  REQUIRE(CreateBinaryThinningSliceBatchPlan<int32>(SizeVec3{0, 5, 3}, k_BytesPerWorker, /*maximumWorkers=*/1).invalid());
  REQUIRE(CreateBinaryThinningSliceBatchPlan<int32>(dims, k_BytesPerWorker, /*maximumWorkers=*/0).invalid());
  REQUIRE(CreateBinaryThinningSliceBatchPlan<int32>(SizeVec3{std::numeric_limits<usize>::max(), 2, 3}, std::numeric_limits<usize>::max(), /*maximumWorkers=*/8).invalid());
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: 3D slice batches match one-slice results and transfer each value once", "[ImageProcessing][BinaryThinningEngine][WorkingMemory]")
{
  using ImageProcessing::detail::ApplyBinaryThinningSliceBatches;
  using ImageProcessing::detail::CreateBinaryThinningSliceBatchPlan;

  constexpr usize k_DimX = 19;
  constexpr usize k_DimY = 17;
  constexpr usize k_DimZ = 6;
  constexpr usize k_SliceValues = k_DimX * k_DimY;
  std::vector<int32> input(k_SliceValues * k_DimZ, 0);
  for(usize z = 0; z < k_DimZ; ++z)
  {
    const usize xBegin = z % 4;
    const usize xEnd = std::min(k_DimX, xBegin + 8 + z);
    const usize yBegin = (2 * z) % 5;
    const usize yEnd = std::min(k_DimY, yBegin + 7 + z);
    for(usize y = yBegin; y < yEnd; ++y)
    {
      for(usize x = xBegin; x < xEnd; ++x)
      {
        input[z * k_SliceValues + y * k_DimX + x] = static_cast<int32>(z + 2);
      }
    }
  }

  std::vector<int32> expected(input.size());
  for(usize z = 0; z < k_DimZ; ++z)
  {
    const std::vector<int32> inputSlice(input.data() + z * k_SliceValues, input.data() + (z + 1) * k_SliceValues);
    const std::vector<int32> outputSlice = RunThinning(inputSlice, k_DimX, k_DimY, 1);
    std::copy(outputSlice.cbegin(), outputSlice.cend(), expected.data() + z * k_SliceValues);
  }

  constexpr usize k_MarkerWords = (k_SliceValues + 63) / 64;
  constexpr usize k_BytesPerWorker = k_SliceValues * (sizeof(int32) + sizeof(uint8)) + k_MarkerWords * sizeof(uint64);
  auto planResult = CreateBinaryThinningSliceBatchPlan<int32>(SizeVec3{k_DimX, k_DimY, k_DimZ}, 2 * k_BytesPerWorker, /*maximumWorkers=*/8);
  REQUIRE(planResult.valid());
  const auto& plan = planResult.value();
  REQUIRE(plan.workerCount == 2);

  TransferCountingDataStore<int32> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{0});
  for(usize index = 0; index < input.size(); ++index)
  {
    inputStore.setValue(index, input[index]);
  }
  TransferCountingDataStore<int32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{-7});
  std::atomic_bool shouldCancel{false};
  REQUIRE(ApplyBinaryThinningSliceBatches<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, shouldCancel, plan).valid());

  std::vector<int32> actual(input.size());
  for(usize index = 0; index < actual.size(); ++index)
  {
    actual[index] = outputStore.getValue(index);
  }
  REQUIRE(actual == expected);
  REQUIRE(inputStore.readCount() == 3);
  REQUIRE(inputStore.readValues() == input.size());
  REQUIRE(inputStore.maximumReadValues() == 2 * k_SliceValues);
  REQUIRE(outputStore.writeCount() == 3);
  REQUIRE(outputStore.writtenValues() == input.size());
  REQUIRE(outputStore.maximumWriteValues() == 2 * k_SliceValues);

  const usize readsBeforeInvalidPlan = inputStore.readCount();
  const usize writesBeforeInvalidPlan = outputStore.writeCount();
  auto invalidPlan = plan;
  --invalidPlan.bytesPerWorker;
  REQUIRE(ApplyBinaryThinningSliceBatches<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, shouldCancel, invalidPlan).invalid());
  REQUIRE(inputStore.readCount() == readsBeforeInvalidPlan);
  REQUIRE(outputStore.writeCount() == writesBeforeInvalidPlan);
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: 3D slice-batch cancellation and I-O failures stop before the next output batch", "[ImageProcessing][BinaryThinningEngine][WorkingMemory]")
{
  using ImageProcessing::detail::ApplyBinaryThinningSliceBatches;
  using ImageProcessing::detail::CreateBinaryThinningSliceBatchPlan;

  constexpr usize k_DimX = 9;
  constexpr usize k_DimY = 7;
  constexpr usize k_DimZ = 5;
  constexpr usize k_SliceValues = k_DimX * k_DimY;
  constexpr int32 k_Poison = -77;
  constexpr usize k_MarkerWords = (k_SliceValues + 63) / 64;
  constexpr usize k_BytesPerWorker = k_SliceValues * (sizeof(int32) + sizeof(uint8)) + k_MarkerWords * sizeof(uint64);
  const SizeVec3 dims{k_DimX, k_DimY, k_DimZ};
  auto planResult = CreateBinaryThinningSliceBatchPlan<int32>(dims, 2 * k_BytesPerWorker, /*maximumWorkers=*/8);
  REQUIRE(planResult.valid());
  const auto& plan = planResult.value();
  REQUIRE(plan.workerCount == 2);

  SECTION("cancellation after the first batch read preserves caller output")
  {
    std::atomic_bool shouldCancel{false};
    CancelAfterFirstReadDataStore<int32> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{1}, shouldCancel);
    DataStore<int32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = ApplyBinaryThinningSliceBatches<int32>(inputStore, outputStore, dims, shouldCancel, plan);
    REQUIRE(result.valid());
    REQUIRE(inputStore.readCount() == 1);
    for(usize index = 0; index < outputStore.getSize(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }

  SECTION("the second batch read failure preserves the completed output prefix")
  {
    std::atomic_bool shouldCancel{false};
    FailOnSecondReadDataStore<int32> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{1});
    TransferCountingDataStore<int32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = ApplyBinaryThinningSliceBatches<int32>(inputStore, outputStore, dims, shouldCancel, plan);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8718);
    REQUIRE(inputStore.readCount() == 2);
    REQUIRE(outputStore.writeCount() == 1);
    REQUIRE(outputStore.writtenValues() == 2 * k_SliceValues);
    for(usize index = 2 * k_SliceValues; index < outputStore.getSize(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }

  SECTION("the first output write failure is propagated")
  {
    std::atomic_bool shouldCancel{false};
    TransferCountingDataStore<int32> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{1});
    FailOnFirstWriteDataStore<int32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = ApplyBinaryThinningSliceBatches<int32>(inputStore, outputStore, dims, shouldCancel, plan);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8719);
    REQUIRE(inputStore.readCount() == 1);
    REQUIRE(outputStore.writeCount() == 1);
    for(usize index = 0; index < outputStore.getSize(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: checked true-2-D memory plans", "[ImageProcessing][BinaryThinningEngine]")
{
  using ImageProcessing::detail::CreateBinaryThinningExternal2DPlan;
  using ImageProcessing::detail::CreateBinaryThinningFixed2DPlan;

  constexpr usize k_TargetBytes = 1024 * 1024;
  const auto fixedResult = CreateBinaryThinningFixed2DPlan<int64>(700000, k_TargetBytes);
  REQUIRE(fixedResult.valid());
  const auto& fixedPlan = fixedResult.value();
  REQUIRE(fixedPlan.workCapacityValues == 700000);
  REQUIRE(fixedPlan.markerCapacityWords == (700000 + 63) / 64);
  REQUIRE(fixedPlan.transferValues > 0);
  REQUIRE(fixedPlan.transferValues <= fixedPlan.workCapacityValues);
  REQUIRE(fixedPlan.minimumFixedBytes ==
          ImageProcessing::detail::k_BinaryThinning2DMetadataHeadroomBytes + fixedPlan.workCapacityValues + fixedPlan.markerCapacityWords * sizeof(uint64) + sizeof(int64));
  REQUIRE(fixedPlan.residentBytes <= k_TargetBytes);
  REQUIRE(fixedPlan.useFixedCapacity);

  const auto spillResult = CreateBinaryThinningFixed2DPlan<int64>(k_TargetBytes, k_TargetBytes);
  REQUIRE(spillResult.valid());
  REQUIRE_FALSE(spillResult.value().useFixedCapacity);
  REQUIRE(spillResult.value().minimumFixedBytes > k_TargetBytes);

  const usize blockTargetBytes = ImageProcessing::detail::k_BinaryThinning2DMetadataHeadroomBytes + (2 * 2 + 2) * 9;
  const auto blockResult = CreateBinaryThinningExternal2DPlan(/*dimX=*/9, /*dimY=*/7, blockTargetBytes);
  REQUIRE(blockResult.valid());
  REQUIRE_FALSE(blockResult.value().useTiles);
  REQUIRE(blockResult.value().coreRows == 2);
  REQUIRE(blockResult.value().coreColumns == 9);
  REQUIRE(blockResult.value().residentBytes == blockTargetBytes);

  const usize tileTargetBytes = ImageProcessing::detail::k_BinaryThinning2DMetadataHeadroomBytes + 3 * (3 + 2) + 3;
  const auto tileResult = CreateBinaryThinningExternal2DPlan(/*dimX=*/13, /*dimY=*/5, tileTargetBytes);
  REQUIRE(tileResult.valid());
  REQUIRE(tileResult.value().useTiles);
  REQUIRE(tileResult.value().coreRows == 1);
  REQUIRE(tileResult.value().coreColumns == 3);
  REQUIRE(tileResult.value().residentBytes == tileTargetBytes);

  REQUIRE(CreateBinaryThinningFixed2DPlan<int64>(/*volumeValues=*/1, /*targetBytes=*/0).invalid());
  REQUIRE(CreateBinaryThinningFixed2DPlan<int64>(/*volumeValues=*/1, ImageProcessing::detail::k_BinaryThinning2DMetadataHeadroomBytes + 1).invalid());
  REQUIRE(CreateBinaryThinningExternal2DPlan(/*dimX=*/0, /*dimY=*/5, tileTargetBytes).invalid());
  REQUIRE(CreateBinaryThinningExternal2DPlan(/*dimX=*/13, /*dimY=*/0, tileTargetBytes).invalid());
  REQUIRE(CreateBinaryThinningExternal2DPlan(/*dimX=*/13, /*dimY=*/5, ImageProcessing::detail::k_BinaryThinning2DMetadataHeadroomBytes + 9).invalid());
  REQUIRE(CreateBinaryThinningExternal2DPlan(std::numeric_limits<usize>::max(), /*dimY=*/2, std::numeric_limits<usize>::max()).invalid());
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: work-store adapter supports abstract and fixed-record stores", "[ImageProcessing][BinaryThinningEngine]")
{
  using ImageProcessing::detail::BinaryThinningWorkStore;
  using ImageProcessing::detail::SweepTemporaryStore;

  constexpr std::array<uint8, 8> k_Expected = {3, 1, 4, 1, 5, 9, 2, 6};
  std::array<uint8, 8> actual = {};

  DataStore<uint8> dataStore(ShapeType{8}, ShapeType{1}, uint8{0});
  BinaryThinningWorkStore dataAdapter(dataStore);
  REQUIRE(dataAdapter.getSize() == k_Expected.size());
  REQUIRE(dataAdapter.copyFromBuffer(0, nonstd::span<const uint8>(k_Expected.data(), k_Expected.size())).valid());
  REQUIRE(dataAdapter.copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size())).valid());
  REQUIRE(actual == k_Expected);

  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(uint8);
  config.maxRecordsPerBatch = 3;
  config.initialRecordCount = k_Expected.size();
  auto recordResult = InMemoryTemporaryRecordStore::Create(config);
  REQUIRE(recordResult.valid());
  std::atomic_bool shouldCancel{false};
  SweepTemporaryStore<uint8> fixedRecords(std::move(recordResult.value()), shouldCancel, "Binary-thinning adapter test");
  BinaryThinningWorkStore fixedAdapter(fixedRecords);
  actual.fill(uint8{0});
  REQUIRE(fixedAdapter.getSize() == k_Expected.size());
  REQUIRE(fixedAdapter.copyFromBuffer(0, nonstd::span<const uint8>(k_Expected.data(), k_Expected.size())).valid());
  REQUIRE(fixedAdapter.copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size())).valid());
  REQUIRE(actual == k_Expected);
  REQUIRE_FALSE(fixedAdapter.getChunkShape().has_value());
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: fixed-capacity true-2-D path matches resident and bounds transfers", "[ImageProcessing][BinaryThinningEngine]")
{
  using ImageProcessing::detail::ApplyBinaryThinningFixed2D;
  using ImageProcessing::detail::CreateBinaryThinningFixed2DPlan;

  constexpr usize k_TargetBytes = 1024 * 1024;
  std::atomic_bool shouldCancel{false};

  const auto requireFixedMatches = [&](const std::vector<int32>& mask, usize dimX, usize dimY, const std::vector<int32>& expected) {
    const auto planResult = CreateBinaryThinningFixed2DPlan<int32>(mask.size(), k_TargetBytes);
    REQUIRE(planResult.valid());
    const auto& plan = planResult.value();
    REQUIRE(plan.useFixedCapacity);

    TransferCountingDataStore<int32> inputStore(ShapeType{1, dimY, dimX}, ShapeType{1}, int32{0});
    for(usize index = 0; index < mask.size(); ++index)
    {
      inputStore.setValue(index, mask[index]);
    }
    TransferCountingDataStore<int32> outputStore(ShapeType{1, dimY, dimX}, ShapeType{1}, int32{-7});
    const Result<> result = ApplyBinaryThinningFixed2D<int32>(inputStore, outputStore, SizeVec3{dimX, dimY, 1}, shouldCancel, plan);
    REQUIRE(result.valid());

    std::vector<int32> actual(mask.size());
    REQUIRE(outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size())).valid());
    REQUIRE(actual == expected);
    REQUIRE(inputStore.maximumReadValues() <= plan.transferValues);
    REQUIRE(outputStore.maximumWriteValues() <= plan.transferValues);
    REQUIRE(plan.residentBytes <= k_TargetBytes);
  };

  SECTION("multi-iteration interior and border shapes")
  {
    constexpr usize k_DimX = 19;
    constexpr usize k_DimY = 17;
    std::vector<int32> mask(k_DimX * k_DimY, 0);
    for(usize y = 2; y <= 14; ++y)
    {
      for(usize x = 4; x <= 14; ++x)
      {
        mask[y * k_DimX + x] = 5;
      }
    }
    for(usize y = 0; y <= 10; ++y)
    {
      for(usize x = 0; x <= 2; ++x)
      {
        mask[y * k_DimX + x] = -3;
      }
    }
    for(usize y = 0; y <= 2; ++y)
    {
      for(usize x = 0; x <= 9; ++x)
      {
        mask[y * k_DimX + x] = -3;
      }
    }
    for(usize x = 15; x <= 17; ++x)
    {
      mask[15 * k_DimX + x] = 9;
    }
    requireFixedMatches(mask, k_DimX, k_DimY, RunThinning(mask, k_DimX, k_DimY, 1));
  }

  SECTION("empty and already-thin images are literal fixed points")
  {
    constexpr usize k_DimX = 11;
    constexpr usize k_DimY = 7;
    const std::vector<int32> empty(k_DimX * k_DimY, 0);
    requireFixedMatches(empty, k_DimX, k_DimY, empty);

    std::vector<int32> thin(k_DimX * k_DimY, 0);
    for(usize y = 0; y < k_DimY; ++y)
    {
      thin[y * k_DimX + k_DimX / 2] = 1;
    }
    requireFixedMatches(thin, k_DimX, k_DimY, thin);
  }
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: external true-2-D row blocks match resident and bound scratch transfers", "[ImageProcessing][BinaryThinningEngine]")
{
  using ImageProcessing::detail::ApplyBinaryThinningExternal2D;
  using ImageProcessing::detail::ApplyBinaryThinningExternalSubstepBlocks;
  using ImageProcessing::detail::BinaryThinningWorkStore;
  using ImageProcessing::detail::CreateBinaryThinningExternal2DPlan;

  constexpr usize k_DimX = 9;
  constexpr usize k_DimY = 7;
  const usize targetBytes = ImageProcessing::detail::k_BinaryThinning2DMetadataHeadroomBytes + (2 * 2 + 2) * k_DimX;
  const auto planResult = CreateBinaryThinningExternal2DPlan(k_DimX, k_DimY, targetBytes);
  REQUIRE(planResult.valid());
  const auto& plan = planResult.value();
  REQUIRE_FALSE(plan.useTiles);
  REQUIRE(plan.coreRows == 2);

  std::vector<int32> mask(k_DimX * k_DimY, 0);
  for(usize y = 0; y <= 4; ++y)
  {
    for(usize x = 0; x <= 3; ++x)
    {
      mask[y * k_DimX + x] = 4;
    }
  }
  for(usize y = 2; y < k_DimY; ++y)
  {
    for(usize x = 5; x < k_DimX; ++x)
    {
      mask[y * k_DimX + x] = -2;
    }
  }
  const std::vector<int32> expected = RunThinning(mask, k_DimX, k_DimY, 1);
  std::atomic_bool shouldCancel{false};

  SECTION("complete external convergence")
  {
    DataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int32{0});
    for(usize index = 0; index < mask.size(); ++index)
    {
      inputStore.setValue(index, mask[index]);
    }
    DataStore<int32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int32{-7});
    REQUIRE(ApplyBinaryThinningExternal2D<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, shouldCancel, targetBytes).valid());

    std::vector<int32> actual(mask.size());
    REQUIRE(outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size())).valid());
    REQUIRE(actual == expected);
  }

  SECTION("offset execution changes only the selected source and output slice")
  {
    constexpr usize k_SliceCount = 3;
    constexpr int32 k_Poison = -77;
    const usize sliceValues = k_DimX * k_DimY;
    TransferCountingDataStore<int32> inputStore(ShapeType{k_SliceCount, k_DimY, k_DimX}, ShapeType{1}, int32{0});
    for(usize index = 0; index < mask.size(); ++index)
    {
      inputStore.setValue(sliceValues + index, mask[index]);
    }
    TransferCountingDataStore<int32> outputStore(ShapeType{k_SliceCount, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    REQUIRE(ApplyBinaryThinningExternal2D<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, shouldCancel, targetBytes, sliceValues, sliceValues).valid());

    for(usize index = 0; index < sliceValues; ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
      REQUIRE(outputStore.getValue(sliceValues + index) == expected[index]);
      REQUIRE(outputStore.getValue(2 * sliceValues + index) == k_Poison);
    }
    REQUIRE(inputStore.readValues() == sliceValues);
    REQUIRE(outputStore.writtenValues() == sliceValues);

    const usize readsBeforeInvalidOffset = inputStore.readCount();
    const usize writesBeforeInvalidOffset = outputStore.writeCount();
    REQUIRE(ApplyBinaryThinningExternal2D<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, shouldCancel, targetBytes, std::numeric_limits<usize>::max(), 0).invalid());
    REQUIRE(inputStore.readCount() == readsBeforeInvalidOffset);
    REQUIRE(outputStore.writeCount() == writesBeforeInvalidOffset);
  }

  SECTION("step-one scratch transfers")
  {
    TransferCountingDataStore<uint8> sourceStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
    TransferCountingDataStore<uint8> destinationStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, uint8{0xA5});
    for(usize index = 0; index < mask.size(); ++index)
    {
      sourceStore.setValue(index, mask[index] != 0 ? uint8{1} : uint8{0});
    }
    BinaryThinningWorkStore source(sourceStore);
    BinaryThinningWorkStore destination(destinationStore);
    const Result<bool> result = ApplyBinaryThinningExternalSubstepBlocks(source, destination, k_DimX, k_DimY, /*step=*/1, shouldCancel, plan);
    REQUIRE(result.valid());
    REQUIRE(result.value());
    REQUIRE(sourceStore.maximumReadValues() <= (plan.coreRows + 2) * k_DimX);
    REQUIRE(destinationStore.maximumWriteValues() <= plan.coreRows * k_DimX);
    bool differsFromSource = false;
    for(usize index = 0; index < mask.size(); ++index)
    {
      const uint8 value = destinationStore.getValue(index);
      REQUIRE((value == 0 || value == 1));
      differsFromSource = differsFromSource || value != sourceStore.getValue(index);
    }
    REQUIRE(differsFromSource);
  }
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: external true-2-D X tiles match resident and bound scratch transfers", "[ImageProcessing][BinaryThinningEngine]")
{
  using ImageProcessing::detail::ApplyBinaryThinningExternal2D;
  using ImageProcessing::detail::ApplyBinaryThinningExternalSubstepTiles;
  using ImageProcessing::detail::BinaryThinningWorkStore;
  using ImageProcessing::detail::CreateBinaryThinningExternal2DPlan;

  constexpr usize k_DimX = 13;
  constexpr usize k_DimY = 7;
  const usize targetBytes = ImageProcessing::detail::k_BinaryThinning2DMetadataHeadroomBytes + 3 * (3 + 2) + 3;
  const auto planResult = CreateBinaryThinningExternal2DPlan(k_DimX, k_DimY, targetBytes);
  REQUIRE(planResult.valid());
  const auto& plan = planResult.value();
  REQUIRE(plan.useTiles);
  REQUIRE(plan.coreColumns == 3);

  std::vector<int32> mask(k_DimX * k_DimY, 0);
  for(usize y = 0; y <= 3; ++y)
  {
    for(usize x = 0; x <= 7; ++x)
    {
      mask[y * k_DimX + x] = 6;
    }
  }
  for(usize y = 3; y < k_DimY; ++y)
  {
    for(usize x = 6; x < k_DimX; ++x)
    {
      mask[y * k_DimX + x] = -4;
    }
  }
  const std::vector<int32> expected = RunThinning(mask, k_DimX, k_DimY, 1);
  std::atomic_bool shouldCancel{false};

  SECTION("complete external convergence")
  {
    DataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int32{0});
    for(usize index = 0; index < mask.size(); ++index)
    {
      inputStore.setValue(index, mask[index]);
    }
    DataStore<int32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int32{-7});
    REQUIRE(ApplyBinaryThinningExternal2D<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, shouldCancel, targetBytes).valid());

    std::vector<int32> actual(mask.size());
    REQUIRE(outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size())).valid());
    REQUIRE(actual == expected);
  }

  SECTION("step-one scratch transfers")
  {
    TransferCountingDataStore<uint8> sourceStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
    TransferCountingDataStore<uint8> destinationStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, uint8{0xA5});
    for(usize index = 0; index < mask.size(); ++index)
    {
      sourceStore.setValue(index, mask[index] != 0 ? uint8{1} : uint8{0});
    }
    BinaryThinningWorkStore source(sourceStore);
    BinaryThinningWorkStore destination(destinationStore);
    const Result<bool> result = ApplyBinaryThinningExternalSubstepTiles(source, destination, k_DimX, k_DimY, /*step=*/1, shouldCancel, plan);
    REQUIRE(result.valid());
    REQUIRE(result.value());
    REQUIRE(sourceStore.maximumReadValues() <= plan.coreColumns + 2);
    REQUIRE(destinationStore.maximumWriteValues() <= plan.coreColumns);
    bool differsFromSource = false;
    for(usize index = 0; index < mask.size(); ++index)
    {
      const uint8 value = destinationStore.getValue(index);
      REQUIRE((value == 0 || value == 1));
      differsFromSource = differsFromSource || value != sourceStore.getValue(index);
    }
    REQUIRE(differsFromSource);
  }
}

TEST_CASE("ImageProcessing::BinaryThinningEngine: external true-2-D cancellation and I-O failures propagate", "[ImageProcessing][BinaryThinningEngine]")
{
  using ImageProcessing::detail::ApplyBinaryThinningExternal2D;
  using ImageProcessing::detail::ApplyBinaryThinningExternalSubstepBlocks;
  using ImageProcessing::detail::BinaryThinningWorkStore;
  using ImageProcessing::detail::CreateBinaryThinningExternal2DPlan;

  constexpr usize k_DimX = 9;
  constexpr usize k_DimY = 7;
  constexpr int32 k_Poison = -77;
  const usize targetBytes = ImageProcessing::detail::k_BinaryThinning2DMetadataHeadroomBytes + (2 * 2 + 2) * k_DimX;
  const auto planResult = CreateBinaryThinningExternal2DPlan(k_DimX, k_DimY, targetBytes);
  REQUIRE(planResult.valid());
  const auto& plan = planResult.value();

  SECTION("cancellation after the first import read preserves caller output")
  {
    std::atomic_bool shouldCancel{false};
    CancelAfterFirstReadDataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int32{1}, shouldCancel);
    DataStore<int32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = ApplyBinaryThinningExternal2D<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, shouldCancel, targetBytes);
    REQUIRE(result.valid());
    REQUIRE(inputStore.readCount() == 1);
    for(usize index = 0; index < outputStore.getSize(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }

  SECTION("the second scratch read failure is propagated")
  {
    std::atomic_bool shouldCancel{false};
    FailOnSecondReadDataStore<uint8> sourceStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, uint8{1});
    DataStore<uint8> destinationStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
    BinaryThinningWorkStore source(sourceStore);
    BinaryThinningWorkStore destination(destinationStore);
    const Result<bool> result = ApplyBinaryThinningExternalSubstepBlocks(source, destination, k_DimX, k_DimY, /*step=*/1, shouldCancel, plan);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8718);
    REQUIRE(sourceStore.readCount() == 2);
  }

  SECTION("the first caller-output write failure is propagated")
  {
    std::atomic_bool shouldCancel{false};
    DataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int32{1});
    FailOnFirstWriteDataStore<int32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = ApplyBinaryThinningExternal2D<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, shouldCancel, targetBytes);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8719);
    REQUIRE(outputStore.writeCount() == 1);
    for(usize index = 0; index < outputStore.getSize(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }
}
