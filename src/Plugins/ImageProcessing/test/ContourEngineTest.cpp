#include "ContourFilterTestUtils.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/ContourEngine.hpp"

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
// Build an input DataStore laid out {Z, Y, X} (slowest -> fastest) and copy in the flat values.
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

template <class T>
class TransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_MaxReadValues = std::max(m_MaxReadValues, buffer.size());
    m_ReadCount++;
    m_ReadValues += buffer.size();
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_MaxWriteValues = std::max(m_MaxWriteValues, buffer.size());
    m_WrittenValues += buffer.size();
    m_WriteCount++;
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize maxReadValues() const noexcept
  {
    return m_MaxReadValues;
  }

  usize maxWriteValues() const noexcept
  {
    return m_MaxWriteValues;
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

  usize readValues() const noexcept
  {
    return m_ReadValues;
  }

  usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

  usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

private:
  mutable usize m_MaxReadValues = 0;
  mutable usize m_ReadCount = 0;
  mutable usize m_ReadValues = 0;
  usize m_MaxWriteValues = 0;
  usize m_WrittenValues = 0;
  usize m_WriteCount = 0;
};

template <class T>
class OutOfCoreTransferCountingDataStore : public TransferCountingDataStore<T>
{
public:
  using TransferCountingDataStore<T>::TransferCountingDataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }
};

template <class T>
class CancelAfterReadDataStore : public TransferCountingDataStore<T>
{
public:
  CancelAfterReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue, std::atomic_bool& shouldCancel, usize triggerRead)
  : TransferCountingDataStore<T>(tupleShape, componentShape, initialValue)
  , m_ShouldCancel(shouldCancel)
  , m_TriggerRead(triggerRead)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = TransferCountingDataStore<T>::copyIntoBuffer(startIndex, buffer);
    m_ReadCount++;
    if(result.valid() && m_ReadCount == m_TriggerRead)
    {
      m_ShouldCancel = true;
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  usize m_TriggerRead = 0;
  mutable usize m_ReadCount = 0;
};

template <class T>
class FailingReadDataStore : public TransferCountingDataStore<T>
{
public:
  using TransferCountingDataStore<T>::TransferCountingDataStore;

  Result<> copyIntoBuffer(usize, nonstd::span<T>) const override
  {
    return MakeErrorResult(-9750, "Contour test read failure.");
  }
};

class CacheBudgetScope
{
public:
  explicit CacheBudgetScope(uint64 budgetBytes)
  : m_Manager(CacheMemoryBudgetManager::instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(budgetBytes);
  }

  ~CacheBudgetScope()
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }

  CacheBudgetScope(const CacheBudgetScope&) = delete;
  CacheBudgetScope& operator=(const CacheBudgetScope&) = delete;

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget = 0;
};

// Run the contour engine directly on DataStores with a BinaryContourPredicate and return the flat output.
template <class T>
std::vector<T> RunBinaryContour(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, bool fullyConnected, T fg, T bg)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(fullyConnected);
  const BinaryContourPredicate<T> predicate{fg, bg};
  const Result<> result = ApplyContour<T>(inStore, outStore, SizeVec3{dimX, dimY, dimZ}, offsets, predicate, shouldCancel, messageHandler);
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// Compare the engine output to the independent oracle over both connectivities, both a binary and a
// multi-label input (to exercise the non-foreground passthrough), for a given image shape.
template <class T>
void CheckContour(usize dimX, usize dimY, usize dimZ)
{
  const T fg = static_cast<T>(1);
  const T bg = static_cast<T>(0);
  const std::vector<bool> connectivities = {false, true};

  const std::vector<T> binary = contour_test::MakeContourPattern<T>(dimX, dimY, dimZ, fg, bg);
  const std::vector<T> labels = contour_test::MakeMultiLabelContourPattern<T>(dimX, dimY, dimZ);

  for(bool fullyConnected : connectivities)
  {
    const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(fullyConnected);
    const BinaryContourPredicate<T> predicate{fg, bg};

    const std::vector<T> expectedBinary = contour_test::ContourGatherOracle<T>(binary, dimX, dimY, dimZ, offsets, predicate);
    const std::vector<T> actualBinary = RunBinaryContour<T>(binary, dimX, dimY, dimZ, fullyConnected, fg, bg);

    const std::vector<T> expectedLabels = contour_test::ContourGatherOracle<T>(labels, dimX, dimY, dimZ, offsets, predicate);
    const std::vector<T> actualLabels = RunBinaryContour<T>(labels, dimX, dimY, dimZ, fullyConnected, fg, bg);

    for(usize i = 0; i < binary.size(); ++i)
    {
      INFO("fullyConnected=" << (fullyConnected ? "true" : "false") << " index=" << i);
      REQUIRE(actualBinary[i] == expectedBinary[i]);
      REQUIRE(actualLabels[i] == expectedLabels[i]);
    }
  }
}

// Run the contour engine directly on DataStores with a LabelContourPredicate and return the flat output.
template <class T>
std::vector<T> RunLabelContour(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, bool fullyConnected, T bg)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(fullyConnected);
  const LabelContourPredicate<T> predicate{bg};
  const Result<> result = ApplyContour<T>(inStore, outStore, SizeVec3{dimX, dimY, dimZ}, offsets, predicate, shouldCancel, messageHandler);
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// Compare the engine output (LabelContourPredicate) to the independent oracle over both connectivities on a
// multi-label input (labels {0,1,2,3}, background = 0), for a given image shape.
template <class T>
void CheckLabelContour(usize dimX, usize dimY, usize dimZ)
{
  const T bg = static_cast<T>(0);
  const std::vector<T> labels = contour_test::MakeMultiLabelContourPattern<T>(dimX, dimY, dimZ);

  for(bool fullyConnected : {false, true})
  {
    const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(fullyConnected);
    const LabelContourPredicate<T> predicate{bg};

    const std::vector<T> expected = contour_test::ContourGatherOracle<T>(labels, dimX, dimY, dimZ, offsets, predicate);
    const std::vector<T> actual = RunLabelContour<T>(labels, dimX, dimY, dimZ, fullyConnected, bg);

    for(usize i = 0; i < labels.size(); ++i)
    {
      INFO("fullyConnected=" << (fullyConnected ? "true" : "false") << " index=" << i);
      REQUIRE(actual[i] == expected[i]);
    }
  }
}

template <class PredicateT>
void RequireBoundedContour(const std::vector<int32>& input, const std::vector<int32>& expected, const std::vector<SEOffset>& offsets, const PredicateT& predicate, usize targetBytes,
                           usize maximumReadValues, usize maximumWriteValues)
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 7;
  constexpr usize dimZ = 1;
  constexpr usize totalValues = dimX * dimY;
  TransferCountingDataStore<int32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
  TransferCountingDataStore<int32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
  auto copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  auto applyContourResult = ApplyContour<int32>(inputStore, outputStore, SizeVec3{dimX, dimY, dimZ}, offsets, predicate, shouldCancel, messageHandler, targetBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(applyContourResult);

  std::vector<int32> actual(totalValues);
  auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expected);
  CAPTURE(inputStore.maxReadValues(), outputStore.maxWriteValues(), outputStore.writtenValues());
  REQUIRE(inputStore.maxReadValues() == maximumReadValues);
  REQUIRE(outputStore.maxWriteValues() == maximumWriteValues);
  REQUIRE(outputStore.writtenValues() == totalValues);
}
} // namespace

TEST_CASE("ImageProcessing::ContourEngine: resident state requires a complete dataset-scaled reservation", "[ImageProcessing][ContourEngine][WorkingMemory]")
{
  constexpr usize dimX = 512;
  constexpr usize dimY = 512;
  constexpr usize dimZ = 128;
  constexpr usize valueCount = dimX * dimY * dimZ;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{dimX, dimY, dimZ};

  auto uint8Result = ImageProcessing::detail::CalculateContourResidentWorkingMemoryBytes<uint8>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(uint8Result);
  REQUIRE(uint8Result.value() == valueCount * 2 * sizeof(uint8));
  REQUIRE(uint8Result.value() == 64 * k_MiB);
  auto int32Result = ImageProcessing::detail::CalculateContourResidentWorkingMemoryBytes<int32>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(int32Result);
  REQUIRE(int32Result.value() == valueCount * 2 * sizeof(int32));
  auto calculateContourResidentWorkingMemoryBytesResult = ImageProcessing::detail::CalculateContourResidentWorkingMemoryBytes<int32>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(calculateContourResidentWorkingMemoryBytesResult);

  REQUIRE(ImageProcessing::detail::ShouldUseContourResidentState(dims));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseContourResidentState(SizeVec3{dimX, dimY, 1}));

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(128 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveContourResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 32 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(512 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveContourResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == uint8Result.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::ContourEngine: real-OOC selector uses resident state only after a complete grant", "[ImageProcessing][ContourEngine][WorkingMemory]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  constexpr uint64 k_PartialBudgetBytes = 1024;
  constexpr uint64 k_CompleteBudgetBytes = 4096;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const std::vector<uint8> input = contour_test::MakeMultiLabelContourPattern<uint8>(dimX, dimY, dimZ);
  const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(true);
  const LabelContourPredicate<uint8> predicate{/*background=*/0};
  const std::vector<uint8> expected = contour_test::ContourGatherOracle<uint8>(input, dimX, dimY, dimZ, offsets, predicate);

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();

  auto run = [&](uint64 budgetBytes) {
    manager.clear();
    manager.setBudgetBytes(budgetBytes);
    OutOfCoreTransferCountingDataStore<uint8> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{0});
    OutOfCoreTransferCountingDataStore<uint8> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{0});
    for(usize index = 0; index < input.size(); ++index)
    {
      inputStore.setValue(index, input[index]);
    }

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    auto applyContourResult = ApplyContour(inputStore, outputStore, dims, offsets, predicate, shouldCancel, messageHandler);
    SIMPLNX_RESULT_REQUIRE_VALID(applyContourResult);
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == expected[index]);
    }
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    return std::array<usize, 5>{inputStore.readCount(), inputStore.readValues(), inputStore.maxReadValues(), outputStore.writeCount(), outputStore.maxWriteValues()};
  };

  const auto partialTransfers = run(k_PartialBudgetBytes);
  REQUIRE(partialTransfers[0] == dimZ);
  REQUIRE(partialTransfers[1] == input.size());
  REQUIRE(partialTransfers[2] == dimX * dimY);
  REQUIRE(partialTransfers[3] == dimZ);
  REQUIRE(partialTransfers[4] == dimX * dimY);
  const auto completeTransfers = run(k_CompleteBudgetBytes);
  REQUIRE(completeTransfers[0] == 1);
  REQUIRE(completeTransfers[1] == input.size());
  REQUIRE(completeTransfers[2] == input.size());
  REQUIRE(completeTransfers[3] == 1);
  REQUIRE(completeTransfers[4] == input.size());
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::ContourEngine: rolling 3D body omits unavailable boundary planes", "[ImageProcessing][ContourEngine]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 4;
  constexpr usize dimZ = 3;
  constexpr usize sliceValues = dimX * dimY;
  const std::vector<int32> input = contour_test::MakeMultiLabelContourPattern<int32>(dimX, dimY, dimZ);
  const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(/*fullyConnected=*/true);
  const LabelContourPredicate<int32> predicate{/*background=*/0};
  const std::vector<int32> expected = contour_test::ContourGatherOracle<int32>(input, dimX, dimY, dimZ, offsets, predicate);

  std::vector<int32> firstOutput(sliceValues, std::numeric_limits<int32>::lowest());
  nx::core::ImageProcessing::detail::ContourRollingPlaneBody<int32, LabelContourPredicate<int32>>{.previousPlane = nullptr,
                                                                                                  .currentPlane = input.data(),
                                                                                                  .nextPlane = input.data() + sliceValues,
                                                                                                  .outPlane = firstOutput.data(),
                                                                                                  .offsets = offsets.data(),
                                                                                                  .numOffsets = offsets.size(),
                                                                                                  .dimX = dimX,
                                                                                                  .dimY = dimY,
                                                                                                  .hasPrevious = false,
                                                                                                  .hasNext = true,
                                                                                                  .predicate = predicate}(Range{0, sliceValues});
  REQUIRE(std::equal(firstOutput.cbegin(), firstOutput.cend(), expected.cbegin()));

  std::vector<int32> lastOutput(sliceValues, std::numeric_limits<int32>::lowest());
  nx::core::ImageProcessing::detail::ContourRollingPlaneBody<int32, LabelContourPredicate<int32>>{.previousPlane = input.data() + sliceValues,
                                                                                                  .currentPlane = input.data() + 2 * sliceValues,
                                                                                                  .nextPlane = nullptr,
                                                                                                  .outPlane = lastOutput.data(),
                                                                                                  .offsets = offsets.data(),
                                                                                                  .numOffsets = offsets.size(),
                                                                                                  .dimX = dimX,
                                                                                                  .dimY = dimY,
                                                                                                  .hasPrevious = true,
                                                                                                  .hasNext = false,
                                                                                                  .predicate = predicate}(Range{0, sliceValues});
  REQUIRE(std::equal(lastOutput.cbegin(), lastOutput.cend(), expected.cbegin() + 2 * sliceValues));
}

TEMPLATE_TEST_CASE("ImageProcessing::ContourEngine: rolling 3D path reads each plane once for Binary and Label predicates", "[ImageProcessing][ContourEngine]", int8, uint8, int32)
{
  using T = TestType;
  const CacheBudgetScope budgetScope(64);
  constexpr usize dimX = 5;
  constexpr usize dimY = 4;
  for(const usize dimZ : {usize{2}, usize{7}})
  {
    const SizeVec3 dims{dimX, dimY, dimZ};
    const usize sliceValues = dimX * dimY;
    const std::vector<T> binary = contour_test::MakeContourPattern<T>(dimX, dimY, dimZ, T{1}, T{0});
    const std::vector<T> labels = contour_test::MakeMultiLabelContourPattern<T>(dimX, dimY, dimZ);
    for(const bool fullyConnected : {false, true})
    {
      const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(fullyConnected);
      const auto requireRolling = [&](const std::vector<T>& source, const auto& predicate) {
        OutOfCoreTransferCountingDataStore<T> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, T{});
        OutOfCoreTransferCountingDataStore<T> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, std::numeric_limits<T>::lowest());
        auto copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const T>(source.data(), source.size()));
        SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
        std::atomic_bool shouldCancel{false};
        IFilter::MessageHandler messageHandler{};
        auto applyContourResult = ApplyContour<T>(inputStore, outputStore, dims, offsets, predicate, shouldCancel, messageHandler);
        SIMPLNX_RESULT_REQUIRE_VALID(applyContourResult);

        std::vector<T> actual(source.size());
        auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<T>(actual.data(), actual.size()));
        SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
        REQUIRE(actual == contour_test::ContourGatherOracle<T>(source, dimX, dimY, dimZ, offsets, predicate));
        REQUIRE(inputStore.readCount() == dimZ);
        REQUIRE(inputStore.readValues() == source.size());
        REQUIRE(inputStore.maxReadValues() == sliceValues);
        REQUIRE(outputStore.writeCount() == dimZ);
        REQUIRE(outputStore.writtenValues() == source.size());
        REQUIRE(outputStore.maxWriteValues() == sliceValues);
      };

      CAPTURE(dimZ, fullyConnected);
      requireRolling(binary, BinaryContourPredicate<T>{T{1}, T{0}});
      requireRolling(labels, LabelContourPredicate<T>{T{0}});
    }
  }
}

TEST_CASE("ImageProcessing::ContourEngine: rolling 3D cancellation after the first plane preserves output", "[ImageProcessing][ContourEngine]")
{
  const CacheBudgetScope budgetScope(64);
  constexpr usize dimX = 5;
  constexpr usize dimY = 4;
  constexpr usize dimZ = 7;
  constexpr int32 poison = 73;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const std::vector<int32> input = contour_test::MakeMultiLabelContourPattern<int32>(dimX, dimY, dimZ);
  const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(/*fullyConnected=*/true);
  const LabelContourPredicate<int32> predicate{/*background=*/0};
  std::atomic_bool shouldCancel{false};
  CancelAfterReadDataStore<int32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{}, shouldCancel, /*triggerRead=*/1);
  OutOfCoreTransferCountingDataStore<int32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, poison);
  auto copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  IFilter::MessageHandler messageHandler{};
  auto applyContourResult = ApplyContour<int32>(inputStore, outputStore, dims, offsets, predicate, shouldCancel, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(applyContourResult);
  REQUIRE(shouldCancel);
  REQUIRE(outputStore.writtenValues() == 0);
  std::vector<int32> actual(input.size());
  auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(std::all_of(actual.cbegin(), actual.cend(), [](int32 value) { return value == poison; }));
}

TEST_CASE("ImageProcessing::ContourEngine: rolling 3D propagates an input read failure", "[ImageProcessing][ContourEngine]")
{
  const CacheBudgetScope budgetScope(64);
  constexpr usize dimX = 5;
  constexpr usize dimY = 4;
  constexpr usize dimZ = 7;
  const SizeVec3 dims{dimX, dimY, dimZ};
  FailingReadDataStore<int32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{});
  OutOfCoreTransferCountingDataStore<int32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{73});
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> result =
      ApplyContour<int32>(inputStore, outputStore, dims, MakeContourNeighborOffsets(/*fullyConnected=*/false), LabelContourPredicate<int32>{/*background=*/0}, shouldCancel, messageHandler);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().front().code == -9750);
  REQUIRE(outputStore.writtenValues() == 0);
}

TEMPLATE_TEST_CASE("ImageProcessing::ContourEngine: BinaryContour matches independent gather oracle (3D + 2D)", "[ImageProcessing][ContourEngine]", int32, uint8, int16)
{
  using T = TestType;
  SECTION("3D non-cubic 5x4x3")
  {
    CheckContour<T>(5, 4, 3);
  }
  SECTION("3D non-cubic 7x5x4")
  {
    CheckContour<T>(7, 5, 4);
  }
  SECTION("2D single-plane 8x6x1 (Z-neighbors clipped to plane)")
  {
    CheckContour<T>(8, 6, 1);
  }
}

TEST_CASE("ImageProcessing::ContourEngine: hand-computed border rule (out-of-bounds neighbors ignored)", "[ImageProcessing][ContourEngine]")
{
  // A 4x3x1 image whose foreground (1) fills the x in {0,1} slab; x in {2,3} is background (0). fg=1, bg=0.
  //   x: 0 1 2 3
  //      F F . .
  //      F F . .   (all y, single z-plane)
  //      F F . .
  // Face connectivity (Fully Connected off): the in-bounds neighbors of a voxel at x=0 are its right neighbor
  // (x=1, foreground) and its y-neighbors (foreground); the x=-1 neighbor is OUT OF BOUNDS and IGNORED. So an
  // x=0 voxel has no differing in-bounds neighbor -> it is NOT a contour -> background. This is the crux of the
  // border rule: an edge voxel is a contour only if a REAL in-bounds neighbor differs, never because the image
  // simply ends. The x=1 voxels border the x=2 background, so they ARE contour (foreground). The x=2/x=3
  // background voxels pass their own value (0) through.
  constexpr usize dimX = 4;
  constexpr usize dimY = 3;
  constexpr usize dimZ = 1;
  std::vector<int32> input(dimX * dimY * dimZ, 0);
  for(usize y = 0; y < dimY; ++y)
  {
    input[contour_test::FlatIndex(0, y, 0, dimX, dimY)] = 1;
    input[contour_test::FlatIndex(1, y, 0, dimX, dimY)] = 1;
  }

  const std::vector<int32> out = RunBinaryContour<int32>(input, dimX, dimY, dimZ, /*fullyConnected=*/false, /*fg=*/1, /*bg=*/0);

  for(usize y = 0; y < dimY; ++y)
  {
    INFO("y=" << y);
    REQUIRE(out[contour_test::FlatIndex(0, y, 0, dimX, dimY)] == 0); // x=0 fg interior-at-edge -> background (OOB ignored)
    REQUIRE(out[contour_test::FlatIndex(1, y, 0, dimX, dimY)] == 1); // x=1 fg borders bg -> contour foreground
    REQUIRE(out[contour_test::FlatIndex(2, y, 0, dimX, dimY)] == 0); // background passes its own value through
    REQUIRE(out[contour_test::FlatIndex(3, y, 0, dimX, dimY)] == 0);
  }
}

TEST_CASE("ImageProcessing::ContourEngine: full-image foreground yields no contour", "[ImageProcessing][ContourEngine]")
{
  // A region filling the ENTIRE image has no in-bounds neighbor that differs anywhere, so it produces NO
  // contour (every foreground voxel becomes background). This is the ITK "no boundary condition" behavior.
  constexpr usize dimX = 5;
  constexpr usize dimY = 4;
  constexpr usize dimZ = 3;
  const std::vector<int32> input(dimX * dimY * dimZ, 7); // fg everywhere
  for(bool fullyConnected : {false, true})
  {
    const std::vector<int32> out = RunBinaryContour<int32>(input, dimX, dimY, dimZ, fullyConnected, /*fg=*/7, /*bg=*/0);
    for(usize i = 0; i < out.size(); ++i)
    {
      INFO("fullyConnected=" << (fullyConnected ? "true" : "false") << " index=" << i);
      REQUIRE(out[i] == 0);
    }
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::ContourEngine: LabelContour matches independent gather oracle (3D + 2D)", "[ImageProcessing][ContourEngine]", int32, uint8, int16)
{
  using T = TestType;
  SECTION("3D non-cubic 5x4x3")
  {
    CheckLabelContour<T>(5, 4, 3);
  }
  SECTION("3D non-cubic 7x5x4")
  {
    CheckLabelContour<T>(7, 5, 4);
  }
  SECTION("2D single-plane 8x6x1 (Z-neighbors clipped to plane)")
  {
    CheckLabelContour<T>(8, 6, 1);
  }
}

TEST_CASE("ImageProcessing::ContourEngine: over-capacity neighbor offsets are rejected (no buffer overflow)", "[ImageProcessing][ContourEngine]")
{
  // ApplyContour gathers in-bounds neighbors into a fixed k_MaxRadius1Neighbors-slot stack buffer. A caller that
  // passes more than a radius-1 box's worth of offsets would overflow it in a release build; the engine must
  // reject it with a clean (invalid) Result instead. MakeContourNeighborOffsets never does this, so this
  // exercises the defensive guard directly on the public template.
  DataStore<int32> inStore(ShapeType{2, 2, 2}, ShapeType{1}, 0);
  DataStore<int32> outStore(ShapeType{2, 2, 2}, ShapeType{1}, 0);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const std::vector<SEOffset> tooMany(k_MaxRadius1Neighbors + 1, SEOffset{1, 0, 0});
  const BinaryContourPredicate<int32> predicate{1, 0};
  const Result<> result = ApplyContour<int32>(inStore, outStore, SizeVec3{2, 2, 2}, tooMany, predicate, shouldCancel, messageHandler);
  REQUIRE(result.invalid());
}

TEST_CASE("ImageProcessing::ContourEngine: single voxel (empty neighbor span) is handled", "[ImageProcessing][ContourEngine]")
{
  // A 1x1x1 image has NO in-bounds neighbors for its lone voxel, so the predicate is invoked with an empty
  // neighbor span. A foreground voxel then has no differing neighbor -> not a contour -> background; a
  // non-foreground voxel passes its own value through. Both connectivities exercise the same empty-span path.
  for(bool fullyConnected : {false, true})
  {
    const std::vector<int32> outFg = RunBinaryContour<int32>({1}, 1, 1, 1, fullyConnected, /*fg=*/1, /*bg=*/0);
    REQUIRE(outFg[0] == 0); // fg with no differing in-bounds neighbor -> bg
    const std::vector<int32> outBg = RunBinaryContour<int32>({5}, 1, 1, 1, fullyConnected, /*fg=*/1, /*bg=*/0);
    REQUIRE(outBg[0] == 5); // non-fg passes its own value through
  }
}

TEST_CASE("ImageProcessing::ContourEngine: LabelContour hand-computed (background never a contour; labels differ)", "[ImageProcessing][ContourEngine]")
{
  // A 4x1x1 line: label 5, label 5, background 0, label 8. background = 0. Face connectivity.
  //   x=0 (5): in-bounds neighbor x=1 (5) same; x=-1 OOB ignored -> no differing neighbor -> background.
  //   x=1 (5): neighbors x=0 (5) same, x=2 (0) differs -> contour -> keeps label 5.
  //   x=2 (0): center == background -> ALWAYS background (even though neighbor x=1=5 and x=3=8 differ).
  //   x=3 (8): neighbor x=2 (0) differs -> contour -> keeps label 8 (edge neighbor x=4 OOB ignored).
  constexpr usize dimX = 4;
  constexpr usize dimY = 1;
  constexpr usize dimZ = 1;
  const std::vector<int32> input = {5, 5, 0, 8};

  const std::vector<int32> out = RunLabelContour<int32>(input, dimX, dimY, dimZ, /*fullyConnected=*/false, /*bg=*/0);

  REQUIRE(out[0] == 0); // label 5 interior-at-edge (only same-label in-bounds neighbor) -> background
  REQUIRE(out[1] == 5); // label 5 borders background -> contour, keeps its label
  REQUIRE(out[2] == 0); // background is never a contour
  REQUIRE(out[3] == 8); // label 8 borders background -> contour, keeps its label
}

TEST_CASE("ImageProcessing::RadiusOneStencil2D: planner bounds full-width and overwide typed buffers", "[ImageProcessing][ContourEngine]")
{
  using ImageProcessing::detail::CreateRadiusOneStencil2DPlan;

  SECTION("full-width row blocks")
  {
    const auto result = CreateRadiusOneStencil2DPlan(/*dimX=*/9, /*dimY=*/7, /*inputValueBytes=*/4, /*outputValueBytes=*/4, /*targetBytes=*/216, /*preferFixedInput=*/false);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    const auto& plan = result.value();
    REQUIRE(plan.fullWidth);
    REQUIRE_FALSE(plan.fixedInput);
    REQUIRE(plan.coreRows == 2);
    REQUIRE(plan.coreColumns == 9);
    REQUIRE(plan.inputBufferValues == 36);
    REQUIRE(plan.outputBufferValues == 18);
    REQUIRE(plan.residentBytes == 216);
  }

  SECTION("overwide X tiles")
  {
    const auto result = CreateRadiusOneStencil2DPlan(/*dimX=*/9, /*dimY=*/7, /*inputValueBytes=*/4, /*outputValueBytes=*/4, /*targetBytes=*/56, /*preferFixedInput=*/false);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    const auto& plan = result.value();
    REQUIRE_FALSE(plan.fullWidth);
    REQUIRE_FALSE(plan.fixedInput);
    REQUIRE(plan.coreRows == 1);
    REQUIRE(plan.coreColumns == 2);
    REQUIRE(plan.inputBufferValues == 12);
    REQUIRE(plan.outputBufferValues == 2);
    REQUIRE(plan.residentBytes == 56);
  }

  SECTION("fixed input capacity")
  {
    const auto result =
        CreateRadiusOneStencil2DPlan(/*dimX=*/9, /*dimY=*/7, /*inputValueBytes=*/4, /*outputValueBytes=*/4, ImageProcessing::detail::k_RadiusOneStencil2DTargetBytes, /*preferFixedInput=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    const auto& plan = result.value();
    REQUIRE(plan.fullWidth);
    REQUIRE(plan.fixedInput);
    REQUIRE(plan.inputBufferValues == ImageProcessing::detail::k_RadiusOneStencil2DFixedInputBytes / sizeof(int32));
    REQUIRE(plan.residentBytes <= ImageProcessing::detail::k_RadiusOneStencil2DTargetBytes);
  }

  SECTION("fixed output plane")
  {
    const auto result = CreateRadiusOneStencil2DPlan(/*dimX=*/9, /*dimY=*/7, /*inputValueBytes=*/4, /*outputValueBytes=*/4, ImageProcessing::detail::k_RadiusOneStencil2DTargetBytes,
                                                     /*preferFixedInput=*/false, /*preferFixedOutput=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    const auto& plan = result.value();
    REQUIRE(plan.fullWidth);
    REQUIRE_FALSE(plan.fixedInput);
    REQUIRE(plan.fixedOutput);
    REQUIRE(plan.outputBufferValues == ImageProcessing::detail::k_RadiusOneStencil2DFixedPlaneBytes / sizeof(int32));
    REQUIRE(plan.residentBytes <= ImageProcessing::detail::k_RadiusOneStencil2DTargetBytes);
  }

  SECTION("invalid and overflowing layouts")
  {
    auto createRadiusOneStencil2DPlanResult = CreateRadiusOneStencil2DPlan(/*dimX=*/0, /*dimY=*/7, /*inputValueBytes=*/4, /*outputValueBytes=*/4, /*targetBytes=*/216, false);
    SIMPLNX_RESULT_REQUIRE_INVALID(createRadiusOneStencil2DPlanResult);
    auto createRadiusOneStencil2DPlanResult2 = CreateRadiusOneStencil2DPlan(std::numeric_limits<usize>::max(), /*dimY=*/7, /*inputValueBytes=*/4, /*outputValueBytes=*/4, /*targetBytes=*/216, false);
    SIMPLNX_RESULT_REQUIRE_INVALID(createRadiusOneStencil2DPlanResult2);
    auto createRadiusOneStencil2DPlanResult3 = CreateRadiusOneStencil2DPlan(/*dimX=*/9, /*dimY=*/7, /*inputValueBytes=*/4, /*outputValueBytes=*/4, /*targetBytes=*/39, false);
    SIMPLNX_RESULT_REQUIRE_INVALID(createRadiusOneStencil2DPlanResult3);
  }
}

TEST_CASE("ImageProcessing::ContourEngine: bounded 2D blocks and tiles preserve both contour predicates", "[ImageProcessing][ContourEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 7;
  constexpr usize dimZ = 1;
  const std::vector<int32> binary = contour_test::MakeContourPattern<int32>(dimX, dimY, dimZ, /*foreground=*/1, /*background=*/0);
  const std::vector<int32> labels = contour_test::MakeMultiLabelContourPattern<int32>(dimX, dimY, dimZ);
  const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(/*fullyConnected=*/true);
  const BinaryContourPredicate<int32> binaryPredicate{/*foreground=*/1, /*background=*/0};
  const LabelContourPredicate<int32> labelPredicate{/*background=*/0};
  const std::vector<int32> expectedBinary = contour_test::ContourGatherOracle<int32>(binary, dimX, dimY, dimZ, offsets, binaryPredicate);
  const std::vector<int32> expectedLabels = contour_test::ContourGatherOracle<int32>(labels, dimX, dimY, dimZ, offsets, labelPredicate);

  struct TransferCase
  {
    const char* label;
    usize targetBytes;
    usize maximumReadValues;
    usize maximumWriteValues;
  };
  const std::array<TransferCase, 2> transferCases = {{{"full-width row blocks", 216, 36, 18}, {"overwide X tiles", 56, 4, 2}}};

  for(const TransferCase& transferCase : transferCases)
  {
    const usize workerCount = ImageProcessing::detail::Contour2DWorkerCount();
    const usize targetBytes = transferCase.targetBytes + ImageProcessing::detail::Contour2DWorkerScratchBytes<int32>(workerCount);
    DYNAMIC_SECTION(transferCase.label << " binary")
    {
      RequireBoundedContour(binary, expectedBinary, offsets, binaryPredicate, targetBytes, transferCase.maximumReadValues, transferCase.maximumWriteValues);
    }
    DYNAMIC_SECTION(transferCase.label << " label")
    {
      RequireBoundedContour(labels, expectedLabels, offsets, labelPredicate, targetBytes, transferCase.maximumReadValues, transferCase.maximumWriteValues);
    }
  }
}

TEST_CASE("ImageProcessing::RadiusOneStencil2D: fixed output preserves block offsets and writes once", "[ImageProcessing][ContourEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 7;
  constexpr usize dimZ = 1;
  constexpr usize totalValues = dimX * dimY;
  const std::vector<int32> input = contour_test::MakeMultiLabelContourPattern<int32>(dimX, dimY, dimZ);
  const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(/*fullyConnected=*/true);
  const LabelContourPredicate<int32> predicate{/*background=*/0};
  const std::vector<int32> expected = contour_test::ContourGatherOracle<int32>(input, dimX, dimY, dimZ, offsets, predicate);

  struct FixedOutputCase
  {
    const char* label;
    usize inputAllowanceBytes;
    bool fullWidth;
    usize maximumReadValues;
  };
  const std::array<FixedOutputCase, 2> fixedOutputCases = {{{"full-width row blocks", 216, true, 45}, {"overwide X tiles", 56, false, 4}}};

  for(const FixedOutputCase& fixedOutputCase : fixedOutputCases)
  {
    DYNAMIC_SECTION(fixedOutputCase.label)
    {
      TransferCountingDataStore<int32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
      OutOfCoreTransferCountingDataStore<int32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
      auto copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);

      const usize workerScratchBytes = ImageProcessing::detail::k_RadiusOneStencil2DTargetBytes - ImageProcessing::detail::k_RadiusOneStencil2DFixedPlaneBytes - fixedOutputCase.inputAllowanceBytes;
      const auto planResult = ImageProcessing::detail::CreateRadiusOneStencil2DPlan(dimX, dimY, sizeof(int32), sizeof(int32), ImageProcessing::detail::k_RadiusOneStencil2DTargetBytes,
                                                                                    /*preferFixedInput=*/false, /*preferFixedOutput=*/true, workerScratchBytes);
      SIMPLNX_RESULT_REQUIRE_VALID(planResult);
      REQUIRE(planResult.value().fixedOutput);
      REQUIRE(planResult.value().fullWidth == fixedOutputCase.fullWidth);

      std::atomic_bool shouldCancel{false};
      const Result<> executionResult = ImageProcessing::detail::ExecuteRadiusOneStencil2D<int32, int32>(
          inputStore, outputStore, SizeVec3{dimX, dimY, dimZ}, shouldCancel, ImageProcessing::detail::k_RadiusOneStencil2DTargetBytes, workerScratchBytes, /*maximumWorkers=*/1,
          [&](const int32* blockInput, int32* blockOutput, usize inputXBegin, usize inputYBegin, usize inputWidth, usize outputXBegin, usize outputYBegin, usize outputWidth) {
            return ImageProcessing::detail::Contour2DBlockBody<int32, LabelContourPredicate<int32>>{.input = blockInput,
                                                                                                    .output = blockOutput,
                                                                                                    .offsets = offsets.data(),
                                                                                                    .numOffsets = offsets.size(),
                                                                                                    .dimX = dimX,
                                                                                                    .dimY = dimY,
                                                                                                    .inputXBegin = inputXBegin,
                                                                                                    .inputYBegin = inputYBegin,
                                                                                                    .inputWidth = inputWidth,
                                                                                                    .outputXBegin = outputXBegin,
                                                                                                    .outputYBegin = outputYBegin,
                                                                                                    .outputWidth = outputWidth,
                                                                                                    .predicate = predicate};
          });
      SIMPLNX_RESULT_REQUIRE_VALID(executionResult);

      std::vector<int32> actual(totalValues);
      auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
      REQUIRE(actual == expected);
      CAPTURE(inputStore.maxReadValues(), outputStore.maxWriteValues(), outputStore.writtenValues());
      REQUIRE(inputStore.maxReadValues() == fixedOutputCase.maximumReadValues);
      REQUIRE(outputStore.maxWriteValues() == totalValues);
      REQUIRE(outputStore.writtenValues() == totalValues);
    }
  }
}

TEST_CASE("ImageProcessing::RadiusOneStencil2D: cancellation before the fixed output write preserves output", "[ImageProcessing][ContourEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 7;
  constexpr usize dimZ = 1;
  constexpr usize totalValues = dimX * dimY;
  constexpr int32 poison = 73;
  const std::vector<int32> input = contour_test::MakeMultiLabelContourPattern<int32>(dimX, dimY, dimZ);
  const std::vector<SEOffset> offsets = MakeContourNeighborOffsets(/*fullyConnected=*/true);
  const LabelContourPredicate<int32> predicate{/*background=*/0};
  std::atomic_bool shouldCancel{false};
  CancelAfterReadDataStore<int32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0}, shouldCancel, /*triggerRead=*/2);
  OutOfCoreTransferCountingDataStore<int32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, poison);
  auto copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);

  constexpr usize inputAllowanceBytes = 216;
  constexpr usize workerScratchBytes = ImageProcessing::detail::k_RadiusOneStencil2DTargetBytes - ImageProcessing::detail::k_RadiusOneStencil2DFixedPlaneBytes - inputAllowanceBytes;
  const Result<> executionResult = ImageProcessing::detail::ExecuteRadiusOneStencil2D<int32, int32>(
      inputStore, outputStore, SizeVec3{dimX, dimY, dimZ}, shouldCancel, ImageProcessing::detail::k_RadiusOneStencil2DTargetBytes, workerScratchBytes, /*maximumWorkers=*/1,
      [&](const int32* blockInput, int32* blockOutput, usize inputXBegin, usize inputYBegin, usize inputWidth, usize outputXBegin, usize outputYBegin, usize outputWidth) {
        return ImageProcessing::detail::Contour2DBlockBody<int32, LabelContourPredicate<int32>>{.input = blockInput,
                                                                                                .output = blockOutput,
                                                                                                .offsets = offsets.data(),
                                                                                                .numOffsets = offsets.size(),
                                                                                                .dimX = dimX,
                                                                                                .dimY = dimY,
                                                                                                .inputXBegin = inputXBegin,
                                                                                                .inputYBegin = inputYBegin,
                                                                                                .inputWidth = inputWidth,
                                                                                                .outputXBegin = outputXBegin,
                                                                                                .outputYBegin = outputYBegin,
                                                                                                .outputWidth = outputWidth,
                                                                                                .predicate = predicate};
      });
  SIMPLNX_RESULT_REQUIRE_VALID(executionResult);
  REQUIRE(shouldCancel);
  REQUIRE(outputStore.writtenValues() == 0);
  std::vector<int32> actual(totalValues);
  auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(std::all_of(actual.cbegin(), actual.cend(), [](int32 value) { return value == poison; }));
}
