#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/PointwiseEngine.hpp"

#include <atomic>
#include <initializer_list>
#include <limits>
#include <vector>

using namespace nx::core;

namespace
{
template <class T>
class CountingDataStore : public DataStore<T>
{
public:
  CountingDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue)
  : DataStore<T>(tupleShape, componentShape, initialValue)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_ReadStarts.push_back(startIndex);
    m_ReadCounts.push_back(buffer.size());
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_WriteStarts.push_back(startIndex);
    m_WriteCounts.push_back(buffer.size());
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  const std::vector<usize>& readStarts() const noexcept
  {
    return m_ReadStarts;
  }

  const std::vector<usize>& readCounts() const noexcept
  {
    return m_ReadCounts;
  }

  const std::vector<usize>& writeStarts() const noexcept
  {
    return m_WriteStarts;
  }

  const std::vector<usize>& writeCounts() const noexcept
  {
    return m_WriteCounts;
  }

private:
  mutable std::vector<usize> m_ReadStarts;
  mutable std::vector<usize> m_ReadCounts;
  std::vector<usize> m_WriteStarts;
  std::vector<usize> m_WriteCounts;
};

ImageProcessing::detail::PointwiseBatchPlan RequirePlan(usize totalValues, usize targetValues, std::initializer_list<ImageProcessing::detail::PointwiseEndpointLayout> endpoints)
{
  const Result<ImageProcessing::detail::PointwiseBatchPlan> result = ImageProcessing::detail::MakePointwiseBatchPlan(totalValues, targetValues, endpoints);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  return result.value();
}
} // namespace

TEST_CASE("ImageProcessing::ApplyPointwise plans rectangular OOC batches", "[ImageProcessing][ApplyPointwise]")
{
  constexpr usize k_DefaultScratchBytes = ImageProcessing::detail::k_PointwiseTargetScratchBytes;

  SECTION("Not 600 by 450 by 128 rounds to 124 complete planes")
  {
    constexpr usize k_PlaneValues = 600 * 450;
    constexpr usize k_TotalValues = k_PlaneValues * 128;
    constexpr usize k_TargetValues = k_DefaultScratchBytes / (sizeof(uint8) + sizeof(uint8));
    const ShapeType tupleShape{128, 450, 600};
    const auto plan = RequirePlan(
        k_TotalValues, k_TargetValues,
        {{IDataStore::StoreType::OutOfCore, tupleShape, 1, "input"}, {IDataStore::StoreType::OutOfCore, tupleShape, 1, "output", ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(plan.batchValues == 33'480'000);
    REQUIRE(plan.totalBatches == 2);
    REQUIRE(plan.alignmentValues == k_PlaneValues);
    REQUIRE_FALSE(plan.usedMismatchedLayoutFallback);
    REQUIRE(k_TotalValues - plan.batchValues == 4 * k_PlaneValues);
  }

  SECTION("512 by 512 planes already divide the uint8 budget exactly")
  {
    constexpr usize k_PlaneValues = 512 * 512;
    constexpr usize k_TotalValues = k_PlaneValues * 129;
    constexpr usize k_TargetValues = k_DefaultScratchBytes / (sizeof(uint8) + sizeof(uint8));
    const ShapeType tupleShape{129, 512, 512};
    const auto plan = RequirePlan(k_TotalValues, k_TargetValues, {{IDataStore::StoreType::OutOfCore, tupleShape, 1, "input"}});
    REQUIRE(plan.batchValues == k_TargetValues);
    REQUIRE(plan.totalBatches == 2);
    REQUIRE(k_TotalValues - plan.batchValues == k_PlaneValues);
  }

  SECTION("int16 to uint8 uses the combined three-byte scratch cost")
  {
    constexpr usize k_PlaneValues = 600 * 450;
    constexpr usize k_TotalValues = k_PlaneValues * 128;
    constexpr usize k_TargetValues = k_DefaultScratchBytes / (sizeof(int16) + sizeof(uint8));
    const ShapeType tupleShape{128, 450, 600};
    const auto plan = RequirePlan(
        k_TotalValues, k_TargetValues,
        {{IDataStore::StoreType::OutOfCore, tupleShape, 1, "input"}, {IDataStore::StoreType::OutOfCore, tupleShape, 1, "output", ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(k_TargetValues == 22'369'621);
    REQUIRE(plan.batchValues == 22'140'000);
    REQUIRE(plan.batchValues == 82 * k_PlaneValues);
    REQUIRE(plan.totalBatches == 2);
  }

  SECTION("rank-one multi-component stores align to complete tuples")
  {
    const ShapeType tupleShape{100};
    const auto plan = RequirePlan(300, 100, {{IDataStore::StoreType::OutOfCore, tupleShape, 3, "input"}});
    REQUIRE(plan.batchValues == 99);
    REQUIRE(plan.totalBatches == 4);
  }

  SECTION("small volumes use one complete batch")
  {
    const ShapeType tupleShape{2, 3, 5};
    const auto plan = RequirePlan(30, 100, {{IDataStore::StoreType::OutOfCore, tupleShape, 1, "input"}});
    REQUIRE(plan.batchValues == 30);
    REQUIRE(plan.totalBatches == 1);
  }

  SECTION("mixed endpoints use only the OOC alignment")
  {
    const ShapeType inMemoryShape{60};
    const ShapeType oocShape{2, 3, 10};
    const auto plan = RequirePlan(
        60, 31,
        {{IDataStore::StoreType::InMemory, inMemoryShape, 1, "input"}, {IDataStore::StoreType::OutOfCore, oocShape, 1, "output", ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(plan.batchValues == 30);
    REQUIRE(plan.totalBatches == 2);
  }

  SECTION("differing OOC layouts use their checked least common multiple")
  {
    const ShapeType inputShape{2, 3, 10}; // 30-value slabs
    const ShapeType outputShape{3, 4, 5}; // 20-value slabs
    const auto plan = RequirePlan(
        60, 60,
        {{IDataStore::StoreType::OutOfCore, inputShape, 1, "input"}, {IDataStore::StoreType::OutOfCore, outputShape, 1, "output", ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(plan.batchValues == 60);
    REQUIRE(plan.totalBatches == 1);
    REQUIRE(plan.alignmentValues == 60);
    REQUIRE_FALSE(plan.usedMismatchedLayoutFallback);
  }

  SECTION("large coprime OOC layouts fall back to the bounded output alignment")
  {
    constexpr usize k_InputSlabValues = 1009;
    constexpr usize k_OutputSlabValues = 1013;
    constexpr usize k_TotalValues = k_InputSlabValues * k_OutputSlabValues;
    const ShapeType inputShape{k_OutputSlabValues, k_InputSlabValues};
    const ShapeType outputShape{k_InputSlabValues, k_OutputSlabValues};
    const auto plan = RequirePlan(
        k_TotalValues, 1000,
        {{IDataStore::StoreType::OutOfCore, inputShape, 1, "input"}, {IDataStore::StoreType::OutOfCore, outputShape, 1, "output", ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(plan.usedMismatchedLayoutFallback);
    REQUIRE(plan.alignmentValues == k_OutputSlabValues);
    REQUIRE(plan.batchValues == k_OutputSlabValues);
    REQUIRE(plan.batchValues <= std::max<usize>(1000, k_OutputSlabValues));
    REQUIRE(plan.batchValues < k_TotalValues);
    REQUIRE(plan.totalBatches == k_InputSlabValues);
  }

  SECTION("an exact target cap is retained")
  {
    const ShapeType tupleShape{2, 4, 5};
    const auto plan = RequirePlan(40, 40, {{IDataStore::StoreType::OutOfCore, tupleShape, 1, "input"}});
    REQUIRE(plan.batchValues == 40);
    REQUIRE(plan.totalBatches == 1);
  }

  SECTION("one selected alignment unit is the minimum when it exceeds the target")
  {
    const ShapeType tupleShape{2, 10, 10};
    const auto plan = RequirePlan(200, 64, {{IDataStore::StoreType::OutOfCore, tupleShape, 1, "input"}});
    REQUIRE(plan.batchValues == 100);
    REQUIRE(plan.totalBatches == 2);
  }

  SECTION("zero values require no batches and do not inspect layouts")
  {
    const ShapeType emptyShape;
    const auto plan = RequirePlan(0, 0, {{IDataStore::StoreType::OutOfCore, emptyShape, 0, "input"}});
    REQUIRE(plan.batchValues == 0);
    REQUIRE(plan.totalBatches == 0);
  }
}

TEST_CASE("ImageProcessing::ApplyPointwise rejects malformed OOC batch layouts", "[ImageProcessing][ApplyPointwise]")
{
  auto requireError = [](usize totalValues, const ShapeType& tupleShape, usize numComponents, int32 expectedCode) {
    const Result<ImageProcessing::detail::PointwiseBatchPlan> result =
        ImageProcessing::detail::MakePointwiseBatchPlan(totalValues, 100, {{IDataStore::StoreType::OutOfCore, tupleShape, numComponents, "input"}});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == expectedCode);
  };

  SECTION("zero components")
  {
    requireError(1, ShapeType{1}, 0, -8690);
  }
  SECTION("empty tuple shape")
  {
    requireError(1, ShapeType{}, 1, -8691);
  }
  SECTION("zero tuple dimension")
  {
    requireError(1, ShapeType{1, 0}, 1, -8692);
  }
  SECTION("shape product overflow")
  {
    requireError(1, ShapeType{2, std::numeric_limits<usize>::max(), 2}, 1, -8693);
  }
  SECTION("slab multiplication overflow")
  {
    requireError(1, ShapeType{1, std::numeric_limits<usize>::max()}, 2, -8694);
  }
  SECTION("layout component multiplication overflow")
  {
    requireError(1, ShapeType{std::numeric_limits<usize>::max()}, 2, -8695);
  }
  SECTION("non-divisible incompatible endpoint shape")
  {
    requireError(31, ShapeType{2, 3, 5}, 1, -8696);
  }
}

TEST_CASE("ImageProcessing::ApplyPointwise doubles every value", "[ImageProcessing]")
{
  UnitTest::LoadPlugins();
  // Keep the basic transform check pinned in memory; the dedicated non-power-of-two test below covers real OOC stores.
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize kDim = 10;
  const ShapeType tupleShape = {kDim, kDim, kDim};
  DataStructure ds;

  const DataPath inputPath({"input"});
  const DataPath outputPath({"output"});
  auto inStore = DataStoreUtilities::CreateDataStore<int32>(ds, inputPath, tupleShape, {1});
  auto* inArray = DataArray<int32>::Create(ds, "input", inStore, {});
  auto outStore = DataStoreUtilities::CreateDataStore<int32>(ds, outputPath, tupleShape, {1});
  auto* outArray = DataArray<int32>::Create(ds, "output", outStore, {});

  // Fill input with 0..N-1
  auto& inRef = inArray->getDataStoreRef();
  const usize n = inRef.getSize();
  for(usize i = 0; i < n; ++i)
  {
    inRef.setValue(i, static_cast<int32>(i));
  }

  const auto doubler = [](int32 v) -> int32 { return static_cast<int32>(v * 2); };
  Result<> result =
      ImageProcessing::ApplyPointwise<int32, int32>(inArray->getDataStoreRef(), outArray->getDataStoreRef(), doubler, /*shouldCancel=*/std::atomic_bool(false), IFilter::MessageHandler{});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  auto& outRef = outArray->getDataStoreRef();
  for(usize i = 0; i < n; ++i)
  {
    REQUIRE(outRef.getValue(i) == static_cast<int32>(i * 2));
  }
}

TEST_CASE("ImageProcessing::ApplyPointwise validates stores and cancellation", "[ImageProcessing][ApplyPointwise]")
{
  const auto doubler = [](int32 value) -> int32 { return value * 2; };

  SECTION("mismatched store sizes")
  {
    DataStore<int32> inputStore(ShapeType{16}, ShapeType{1}, 1);
    DataStore<int32> outputStore(ShapeType{15}, ShapeType{1}, 0);
    std::atomic_bool shouldCancel{false};
    const Result<> result = ImageProcessing::ApplyPointwise<int32, int32>(inputStore, outputStore, doubler, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8355);
  }

  SECTION("pre-cancel preserves output")
  {
    constexpr int32 k_Poison = -12345;
    DataStore<int32> inputStore(ShapeType{1000}, ShapeType{1}, 1);
    DataStore<int32> outputStore(ShapeType{1000}, ShapeType{1}, k_Poison);
    std::atomic_bool shouldCancel{true};
    const Result<> result = ImageProcessing::ApplyPointwise<int32, int32>(inputStore, outputStore, doubler, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    for(const int32 value : outputStore)
    {
      REQUIRE(value == k_Poison);
    }
  }
}

TEST_CASE("ImageProcessing::ApplyPointwise forced buffered execution is exact and cancel-safe", "[ImageProcessing][ApplyPointwise]")
{
  const auto doubler = [](int32 value) -> int32 { return value * 2; };
  constexpr usize k_ValueCount = 23;
  constexpr usize k_TargetValues = 5;
  constexpr usize k_TargetScratchBytes = k_TargetValues * sizeof(int32);
  const ImageProcessing::detail::PointwiseExecutionOptions options{k_TargetScratchBytes, true};

  SECTION("short tail preserves exact output")
  {
    CountingDataStore<int32> inputStore({k_ValueCount}, {1}, 0);
    CountingDataStore<int32> outputStore({k_ValueCount}, {1}, -1);
    for(usize index = 0; index < k_ValueCount; ++index)
    {
      inputStore.setValue(index, static_cast<int32>(index) - 7);
    }

    std::atomic_bool shouldCancel{false};
    const Result<> result = ImageProcessing::detail::ApplyPointwiseImpl(inputStore, outputStore, doubler, shouldCancel, {}, options);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    REQUIRE(inputStore.readStarts() == std::vector<usize>{0, 5, 10, 15, 20});
    REQUIRE(inputStore.readCounts() == std::vector<usize>{5, 5, 5, 5, 3});
    REQUIRE(outputStore.writeStarts() == inputStore.readStarts());
    REQUIRE(outputStore.writeCounts() == inputStore.readCounts());
    for(usize index = 0; index < k_ValueCount; ++index)
    {
      REQUIRE(outputStore.getValue(index) == (static_cast<int32>(index) - 7) * 2);
    }
  }

  SECTION("pre-cancel performs no I/O and preserves output")
  {
    constexpr int32 k_Poison = -12345;
    CountingDataStore<int32> inputStore({k_ValueCount}, {1}, 1);
    CountingDataStore<int32> outputStore({k_ValueCount}, {1}, k_Poison);
    std::atomic_bool shouldCancel{true};
    const Result<> result = ImageProcessing::detail::ApplyPointwiseImpl(inputStore, outputStore, doubler, shouldCancel, {}, options);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(inputStore.readCounts().empty());
    REQUIRE(outputStore.writeCounts().empty());
    for(usize index = 0; index < k_ValueCount; ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }
}

TEST_CASE("ImageProcessing::ApplyPointwise same-type buffered transfer reuses one target-sized buffer", "[ImageProcessing][ApplyPointwise]")
{
  constexpr usize k_ValueCount = 10;
  constexpr usize k_TargetScratchBytes = k_ValueCount * sizeof(int32);
  CountingDataStore<int32> inputStore({k_ValueCount}, {1}, 0);
  CountingDataStore<int32> outputStore({k_ValueCount}, {1}, -1);
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    inputStore.setValue(index, static_cast<int32>(index) - 4);
  }

  const auto doubler = [](int32 value) -> int32 { return value * 2; };
  const ImageProcessing::detail::PointwiseExecutionOptions options{k_TargetScratchBytes, true};
  std::atomic_bool shouldCancel{false};
  const Result<> result = ImageProcessing::detail::ApplyPointwiseImpl(inputStore, outputStore, doubler, shouldCancel, {}, options);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  REQUIRE(inputStore.readStarts() == std::vector<usize>{0});
  REQUIRE(inputStore.readCounts() == std::vector<usize>{k_ValueCount});
  REQUIRE(outputStore.writeStarts() == inputStore.readStarts());
  REQUIRE(outputStore.writeCounts() == inputStore.readCounts());
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    REQUIRE(outputStore.getValue(index) == (static_cast<int32>(index) - 4) * 2);
  }
}

TEST_CASE("ImageProcessing::ApplyPointwise different-type buffered transfer retains two-buffer batches", "[ImageProcessing][ApplyPointwise]")
{
  constexpr usize k_ValueCount = 10;
  constexpr usize k_TargetValues = 3;
  CountingDataStore<int32> inputStore({k_ValueCount}, {1}, 0);
  CountingDataStore<float64> outputStore({k_ValueCount}, {1}, -1.0);
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    inputStore.setValue(index, static_cast<int32>(index) - 3);
  }
  const auto cast = [](int32 value) -> float64 { return static_cast<float64>(value) + 0.5; };
  const ImageProcessing::detail::PointwiseExecutionOptions options{k_TargetValues * (sizeof(int32) + sizeof(float64)), true};
  std::atomic_bool shouldCancel{false};
  Result<> applyPointwiseResult = ImageProcessing::detail::ApplyPointwiseImpl(inputStore, outputStore, cast, shouldCancel, {}, options);
  SIMPLNX_RESULT_REQUIRE_VALID(applyPointwiseResult);
  REQUIRE(inputStore.readStarts() == std::vector<usize>{0, 3, 6, 9});
  REQUIRE(inputStore.readCounts() == std::vector<usize>{3, 3, 3, 1});
  REQUIRE(outputStore.writeStarts() == inputStore.readStarts());
  REQUIRE(outputStore.writeCounts() == inputStore.readCounts());
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    REQUIRE(outputStore.getValue(index) == static_cast<float64>(static_cast<int32>(index) - 3) + 0.5);
  }
}

TEST_CASE("ImageProcessing::ApplyPointwise same-type buffered execution supports store aliasing", "[ImageProcessing][ApplyPointwise]")
{
  CountingDataStore<int32> store({4}, {1}, 0);
  for(usize index = 0; index < store.getSize(); ++index)
  {
    store.setValue(index, static_cast<int32>(index) + 1);
  }
  const auto doubler = [](int32 value) -> int32 { return value * 2; };
  std::atomic_bool shouldCancel{false};
  Result<> applyPointwiseResult = ImageProcessing::detail::ApplyPointwiseImpl(store, store, doubler, shouldCancel, {}, {2 * sizeof(int32), true});
  SIMPLNX_RESULT_REQUIRE_VALID(applyPointwiseResult);
  REQUIRE(store.readStarts() == std::vector<usize>{0, 2});
  REQUIRE(store.readCounts() == std::vector<usize>{2, 2});
  REQUIRE(store.writeStarts() == std::vector<usize>{0, 2});
  REQUIRE(store.writeCounts() == std::vector<usize>{2, 2});
  REQUIRE(store.getValue(0) == 2);
  REQUIRE(store.getValue(1) == 4);
  REQUIRE(store.getValue(2) == 6);
  REQUIRE(store.getValue(3) == 8);
}

TEST_CASE("ImageProcessing::ApplyPointwise buffered target smaller than one value uses one-value batches", "[ImageProcessing][ApplyPointwise]")
{
  CountingDataStore<int32> inputStore({3}, {1}, 2);
  CountingDataStore<int32> outputStore({3}, {1}, -1);
  const auto increment = [](int32 value) -> int32 { return value + 1; };
  std::atomic_bool shouldCancel{false};
  Result<> applyPointwiseResult = ImageProcessing::detail::ApplyPointwiseImpl(inputStore, outputStore, increment, shouldCancel, {}, {1, true});
  SIMPLNX_RESULT_REQUIRE_VALID(applyPointwiseResult);
  REQUIRE(inputStore.readStarts() == std::vector<usize>{0, 1, 2});
  REQUIRE(inputStore.readCounts() == std::vector<usize>{1, 1, 1});
  REQUIRE(outputStore.writeStarts() == inputStore.readStarts());
  REQUIRE(outputStore.writeCounts() == inputStore.readCounts());
  REQUIRE(outputStore.getValue(0) == 3);
  REQUIRE(outputStore.getValue(1) == 3);
  REQUIRE(outputStore.getValue(2) == 3);
}

TEST_CASE("ImageProcessing::ApplyPointwise preserves bytes on non-power-of-two OOC stores", "[ImageProcessing][ApplyPointwise][OOC]")
{
  UnitTest::LoadPlugins();
  const auto application = Application::Instance();
  REQUIRE(application != nullptr);
  if(application->getIOManager("HDF5-OOC") == nullptr)
  {
    SUCCEED("The HDF5-OOC backend is not present in this build");
    return;
  }

  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceOutOfCore, 0);
  constexpr usize k_DimZ = 7;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimX = 13;
  constexpr usize k_PlaneValues = k_DimY * k_DimX;
  constexpr usize k_TotalValues = k_DimZ * k_PlaneValues;
  constexpr usize k_PlanesPerBatch = 2;
  constexpr usize k_TargetScratchBytes = k_PlanesPerBatch * k_PlaneValues * (sizeof(uint8) + sizeof(uint8));
  const ShapeType tupleShape{k_DimZ, k_DimY, k_DimX};

  DataStructure dataStructure;
  const DataPath inputPath({"Input"});
  const DataPath outputPath({"Output"});
  auto inputStore = DataStoreUtilities::CreateDataStore<uint8>(dataStructure, inputPath, tupleShape, {1});
  auto outputStore = DataStoreUtilities::CreateDataStore<uint8>(dataStructure, outputPath, tupleShape, {1});
  REQUIRE(inputStore != nullptr);
  REQUIRE(outputStore != nullptr);
  auto* inputArray = DataArray<uint8>::Create(dataStructure, "Input", inputStore, {});
  auto* outputArray = DataArray<uint8>::Create(dataStructure, "Output", outputStore, {});
  REQUIRE(inputArray != nullptr);
  REQUIRE(outputArray != nullptr);
  REQUIRE(inputArray->getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(outputArray->getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);

  std::vector<uint8> input(k_TotalValues);
  std::vector<uint8> expected(k_TotalValues);
  for(usize index = 0; index < k_TotalValues; ++index)
  {
    input[index] = static_cast<uint8>((index * 17 + index / k_PlaneValues * 3) % 11);
    expected[index] = input[index] == 0 ? uint8{1} : uint8{0};
  }
  Result<> copyFromBufferResult = inputArray->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint8>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);

  const auto logicalNot = [](uint8 value) -> uint8 { return value == 0 ? uint8{1} : uint8{0}; };
  std::atomic_bool shouldCancel{false};
  const ImageProcessing::detail::PointwiseExecutionOptions options{k_TargetScratchBytes, false};
  const Result<> result = ImageProcessing::detail::ApplyPointwiseImpl(inputArray->getDataStoreRef(), outputArray->getDataStoreRef(), logicalNot, shouldCancel, {}, options);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  std::vector<uint8> actual(k_TotalValues);
  Result<> copyIntoBufferResult = outputArray->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
