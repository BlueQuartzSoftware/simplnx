#include <catch2/catch.hpp>

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/PointwiseEngine.hpp"

#include <nonstd/span.hpp>

#include <atomic>
#include <initializer_list>
#include <limits>
#include <vector>

using namespace nx::core;

namespace
{
constexpr int32 k_InjectedReadError = -187401;
constexpr int32 k_InjectedWriteError = -187402;

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
    if(m_FailTransfers)
    {
      return MakeErrorResult(k_InjectedReadError, "Injected pointwise input read failure.");
    }
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_WriteStarts.push_back(startIndex);
    m_WriteCounts.push_back(buffer.size());
    if(m_FailTransfers)
    {
      return MakeErrorResult(k_InjectedWriteError, "Injected pointwise output write failure.");
    }
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

  void failTransfers() noexcept
  {
    m_FailTransfers = true;
  }

private:
  bool m_FailTransfers = false;
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

TEST_CASE("NXImageProcessing::ApplyPointwise plans rectangular OOC batches", "[NXImageProcessing][ApplyPointwise]")
{
  constexpr usize k_DefaultScratchBytes = ImageProcessing::detail::k_PointwiseTargetScratchBytes;

  SECTION("Not 600 by 450 by 128 rounds to 124 complete planes")
  {
    constexpr usize k_PlaneValues = 600 * 450;
    constexpr usize k_TotalValues = k_PlaneValues * 128;
    constexpr usize k_TargetValues = k_DefaultScratchBytes / (sizeof(uint8) + sizeof(uint8));
    const ShapeType tupleShape{128, 450, 600};
    const auto plan = RequirePlan(k_TotalValues, k_TargetValues,
                                  {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 1, .name = "input"},
                                   {.store_type = IDataStore::StoreType::OutOfCore,
                                    .tuple_shape = tupleShape,
                                    .num_components = 1,
                                    .name = "output",
                                    .access = ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(plan.batch_values == 33'480'000);
    REQUIRE(plan.total_batches == 2);
    REQUIRE(plan.alignment_values == k_PlaneValues);
    REQUIRE_FALSE(plan.used_mismatched_layout_fallback);
    REQUIRE(k_TotalValues - plan.batch_values == 4 * k_PlaneValues);
  }

  SECTION("512 by 512 planes already divide the uint8 budget exactly")
  {
    constexpr usize k_PlaneValues = 512 * 512;
    constexpr usize k_TotalValues = k_PlaneValues * 129;
    constexpr usize k_TargetValues = k_DefaultScratchBytes / (sizeof(uint8) + sizeof(uint8));
    const ShapeType tupleShape{129, 512, 512};
    const auto plan = RequirePlan(k_TotalValues, k_TargetValues, {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 1, .name = "input"}});
    REQUIRE(plan.batch_values == k_TargetValues);
    REQUIRE(plan.total_batches == 2);
    REQUIRE(k_TotalValues - plan.batch_values == k_PlaneValues);
  }

  SECTION("int16 to uint8 uses the combined three-byte scratch cost")
  {
    constexpr usize k_PlaneValues = 600 * 450;
    constexpr usize k_TotalValues = k_PlaneValues * 128;
    constexpr usize k_TargetValues = k_DefaultScratchBytes / (sizeof(int16) + sizeof(uint8));
    const ShapeType tupleShape{128, 450, 600};
    const auto plan = RequirePlan(k_TotalValues, k_TargetValues,
                                  {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 1, .name = "input"},
                                   {.store_type = IDataStore::StoreType::OutOfCore,
                                    .tuple_shape = tupleShape,
                                    .num_components = 1,
                                    .name = "output",
                                    .access = ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(k_TargetValues == 22'369'621);
    REQUIRE(plan.batch_values == 22'140'000);
    REQUIRE(plan.batch_values == 82 * k_PlaneValues);
    REQUIRE(plan.total_batches == 2);
  }

  SECTION("rank-one multi-component stores align to complete tuples")
  {
    const ShapeType tupleShape{100};
    const auto plan = RequirePlan(300, 100, {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 3, .name = "input"}});
    REQUIRE(plan.batch_values == 99);
    REQUIRE(plan.total_batches == 4);
  }

  SECTION("small volumes use one complete batch")
  {
    const ShapeType tupleShape{2, 3, 5};
    const auto plan = RequirePlan(30, 100, {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 1, .name = "input"}});
    REQUIRE(plan.batch_values == 30);
    REQUIRE(plan.total_batches == 1);
  }

  SECTION("mixed endpoints use only the OOC alignment")
  {
    const ShapeType inMemoryShape{60};
    const ShapeType oocShape{2, 3, 10};
    const auto plan = RequirePlan(
        60, 31,
        {{.store_type = IDataStore::StoreType::InMemory, .tuple_shape = inMemoryShape, .num_components = 1, .name = "input"},
         {.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = oocShape, .num_components = 1, .name = "output", .access = ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(plan.batch_values == 30);
    REQUIRE(plan.total_batches == 2);
  }

  SECTION("differing OOC layouts use their checked least common multiple")
  {
    const ShapeType inputShape{2, 3, 10}; // 30-value slabs
    const ShapeType outputShape{3, 4, 5}; // 20-value slabs
    const auto plan = RequirePlan(60, 60,
                                  {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = inputShape, .num_components = 1, .name = "input"},
                                   {.store_type = IDataStore::StoreType::OutOfCore,
                                    .tuple_shape = outputShape,
                                    .num_components = 1,
                                    .name = "output",
                                    .access = ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(plan.batch_values == 60);
    REQUIRE(plan.total_batches == 1);
    REQUIRE(plan.alignment_values == 60);
    REQUIRE_FALSE(plan.used_mismatched_layout_fallback);
  }

  SECTION("large coprime OOC layouts fall back to the bounded output alignment")
  {
    constexpr usize k_InputSlabValues = 1009;
    constexpr usize k_OutputSlabValues = 1013;
    constexpr usize k_TotalValues = k_InputSlabValues * k_OutputSlabValues;
    const ShapeType inputShape{k_OutputSlabValues, k_InputSlabValues};
    const ShapeType outputShape{k_InputSlabValues, k_OutputSlabValues};
    const auto plan = RequirePlan(k_TotalValues, 1000,
                                  {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = inputShape, .num_components = 1, .name = "input"},
                                   {.store_type = IDataStore::StoreType::OutOfCore,
                                    .tuple_shape = outputShape,
                                    .num_components = 1,
                                    .name = "output",
                                    .access = ImageProcessing::detail::PointwiseEndpointLayout::Access::Write}});
    REQUIRE(plan.used_mismatched_layout_fallback);
    REQUIRE(plan.alignment_values == k_OutputSlabValues);
    REQUIRE(plan.batch_values == k_OutputSlabValues);
    REQUIRE(plan.batch_values <= std::max<usize>(1000, k_OutputSlabValues));
    REQUIRE(plan.batch_values < k_TotalValues);
    REQUIRE(plan.total_batches == k_InputSlabValues);
  }

  SECTION("an exact target cap is retained")
  {
    const ShapeType tupleShape{2, 4, 5};
    const auto plan = RequirePlan(40, 40, {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 1, .name = "input"}});
    REQUIRE(plan.batch_values == 40);
    REQUIRE(plan.total_batches == 1);
  }

  SECTION("one selected alignment unit is the minimum when it exceeds the target")
  {
    const ShapeType tupleShape{2, 10, 10};
    const auto plan = RequirePlan(200, 64, {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = 1, .name = "input"}});
    REQUIRE(plan.batch_values == 100);
    REQUIRE(plan.total_batches == 2);
  }

  SECTION("zero values require no batches and do not inspect layouts")
  {
    const ShapeType emptyShape;
    const auto plan = RequirePlan(0, 0, {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = emptyShape, .num_components = 0, .name = "input"}});
    REQUIRE(plan.batch_values == 0);
    REQUIRE(plan.total_batches == 0);
  }
}

TEST_CASE("NXImageProcessing::ApplyPointwise rejects malformed OOC batch layouts", "[NXImageProcessing][ApplyPointwise]")
{
  auto requireError = [](usize totalValues, const ShapeType& tupleShape, usize numComponents, int32 expectedCode) {
    const Result<ImageProcessing::detail::PointwiseBatchPlan> result = ImageProcessing::detail::MakePointwiseBatchPlan(
        totalValues, 100, {{.store_type = IDataStore::StoreType::OutOfCore, .tuple_shape = tupleShape, .num_components = numComponents, .name = "input"}});
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

TEST_CASE("NXImageProcessing::ApplyPointwise doubles every value", "[NXImageProcessing]")
{
  unit_test::LoadPlugins();
  // Keep the basic transform check pinned in memory; the dedicated non-power-of-two test below covers real OOC stores.
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dim = 10;
  const ShapeType tupleShape = {k_Dim, k_Dim, k_Dim};
  DataStructure dataStructure;

  const DataPath inputPath({"input"});
  const DataPath outputPath({"output"});
  auto inStore = data_store_utilities::CreateDataStore<int32>(dataStructure, inputPath, tupleShape, {1});
  auto* inArrayPtr = DataArray<int32>::Create(dataStructure, "input", inStore, {});
  auto outStore = data_store_utilities::CreateDataStore<int32>(dataStructure, outputPath, tupleShape, {1});
  auto* outArrayPtr = DataArray<int32>::Create(dataStructure, "output", outStore, {});

  // Fill input with 0..N-1
  auto& inRef = inArrayPtr->getDataStoreRef();
  const usize n = inRef.getSize();
  for(usize i = 0; i < n; ++i)
  {
    inRef.setValue(i, static_cast<int32>(i));
  }

  const auto doubler = [](int32 v) -> int32 { return static_cast<int32>(v * 2); };
  Result<> result =
      ImageProcessing::ApplyPointwise<int32, int32>(inArrayPtr->getDataStoreRef(), outArrayPtr->getDataStoreRef(), doubler, /*shouldCancel=*/std::atomic_bool(false), IFilter::MessageHandler{});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  auto& outRef = outArrayPtr->getDataStoreRef();
  for(usize i = 0; i < n; ++i)
  {
    REQUIRE(outRef.getValue(i) == static_cast<int32>(i * 2));
  }
}

TEST_CASE("NXImageProcessing::ApplyPointwise validates stores and cancellation", "[NXImageProcessing][ApplyPointwise]")
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

TEST_CASE("NXImageProcessing::ApplyPointwise forced buffered execution is exact and cancel-safe", "[NXImageProcessing][ApplyPointwise]")
{
  const auto doubler = [](int32 value) -> int32 { return value * 2; };
  constexpr usize k_ValueCount = 23;
  constexpr usize k_TargetValues = 5;
  constexpr usize k_TargetScratchBytes = k_TargetValues * sizeof(int32);
  const ImageProcessing::detail::PointwiseExecutionOptions options{.target_scratch_bytes = k_TargetScratchBytes, .force_buffered = true};

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

TEST_CASE("NXImageProcessing::ApplyPointwise same-type buffered transfer reuses one target-sized buffer", "[NXImageProcessing][ApplyPointwise]")
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
  const ImageProcessing::detail::PointwiseExecutionOptions options{.target_scratch_bytes = k_TargetScratchBytes, .force_buffered = true};
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

TEST_CASE("NXImageProcessing::ApplyPointwise different-type buffered transfer retains two-buffer batches", "[NXImageProcessing][ApplyPointwise]")
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
  const ImageProcessing::detail::PointwiseExecutionOptions options{.target_scratch_bytes = k_TargetValues * (sizeof(int32) + sizeof(float64)), .force_buffered = true};
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

TEST_CASE("NXImageProcessing::ApplyPointwise preserves the destination when its first type-changing batch stops", "[NXImageProcessing][ApplyPointwise]")
{
  constexpr usize valueCount = 4;
  constexpr float32 unwrittenValue = -7.0F;
  CountingDataStore<int16> inputStore({valueCount}, {1}, 1);
  CountingDataStore<float32> outputStore({valueCount}, {1}, unwrittenValue);
  const auto run = [&](bool cancel) {
    const std::atomic_bool shouldCancel{cancel};
    const auto mapValue = [](int16 value) -> float32 { return static_cast<float32>(value) * 0.5F; };
    return ImageProcessing::detail::ApplyPointwiseImpl(inputStore, outputStore, mapValue, shouldCancel, {}, {.force_buffered = true});
  };
  SECTION("cancellation before the first read")
  {
    const auto result = run(true);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(inputStore.readCounts().empty());
    CHECK(outputStore.writeCounts().empty());
  }
  SECTION("cancellation while mapping the first batch prevents its write")
  {
    std::atomic_bool shouldCancel{false};
    const auto cancelDuringMap = [&](int16 value) -> float32 {
      shouldCancel = true;
      return static_cast<float32>(value) * 0.5F;
    };
    const ImageProcessing::detail::PointwiseExecutionOptions options{.target_scratch_bytes = sizeof(int16) + sizeof(float32), .force_buffered = true};
    const auto result = ImageProcessing::detail::ApplyPointwiseImpl(inputStore, outputStore, cancelDuringMap, shouldCancel, {}, options);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(shouldCancel.load());
    CHECK(inputStore.readCounts() == std::vector<usize>{1});
    CHECK(outputStore.writeCounts().empty());
  }
  SECTION("a failed read returns its error before any write")
  {
    inputStore.failTransfers();
    const auto result = run(false);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors().front().code == k_InjectedReadError);
    CHECK(inputStore.readCounts() == std::vector<usize>{valueCount});
    CHECK(outputStore.writeCounts().empty());
  }
  SECTION("a failed write returns its error after one read")
  {
    outputStore.failTransfers();
    const auto result = run(false);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors().front().code == k_InjectedWriteError);
    CHECK(inputStore.readCounts() == std::vector<usize>{valueCount});
    CHECK(outputStore.writeCounts() == std::vector<usize>{valueCount});
  }
  for(usize index = 0; index < valueCount; ++index)
  {
    CHECK(outputStore.getValue(index) == unwrittenValue);
  }
}

TEST_CASE("NXImageProcessing::ApplyPointwise same-type buffered execution supports store aliasing", "[NXImageProcessing][ApplyPointwise]")
{
  CountingDataStore<int32> store({4}, {1}, 0);
  for(usize index = 0; index < store.getSize(); ++index)
  {
    store.setValue(index, static_cast<int32>(index) + 1);
  }
  const auto doubler = [](int32 value) -> int32 { return value * 2; };
  std::atomic_bool shouldCancel{false};
  Result<> applyPointwiseResult = ImageProcessing::detail::ApplyPointwiseImpl(store, store, doubler, shouldCancel, {}, {.target_scratch_bytes = 2 * sizeof(int32), .force_buffered = true});
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

TEST_CASE("NXImageProcessing::ApplyPointwise buffered target smaller than one value uses one-value batches", "[NXImageProcessing][ApplyPointwise]")
{
  CountingDataStore<int32> inputStore({3}, {1}, 2);
  CountingDataStore<int32> outputStore({3}, {1}, -1);
  const auto increment = [](int32 value) -> int32 { return value + 1; };
  std::atomic_bool shouldCancel{false};
  Result<> applyPointwiseResult = ImageProcessing::detail::ApplyPointwiseImpl(inputStore, outputStore, increment, shouldCancel, {}, {.target_scratch_bytes = 1, .force_buffered = true});
  SIMPLNX_RESULT_REQUIRE_VALID(applyPointwiseResult);
  REQUIRE(inputStore.readStarts() == std::vector<usize>{0, 1, 2});
  REQUIRE(inputStore.readCounts() == std::vector<usize>{1, 1, 1});
  REQUIRE(outputStore.writeStarts() == inputStore.readStarts());
  REQUIRE(outputStore.writeCounts() == inputStore.readCounts());
  REQUIRE(outputStore.getValue(0) == 3);
  REQUIRE(outputStore.getValue(1) == 3);
  REQUIRE(outputStore.getValue(2) == 3);
}

TEST_CASE("NXImageProcessing::ApplyPointwise preserves bytes on non-power-of-two OOC stores", "[NXImageProcessing][ApplyPointwise][OOC]")
{
  unit_test::LoadPlugins();
  const auto application = Application::Instance();
  REQUIRE(application != nullptr);
  if(application->getIOManager("HDF5-OOC") == nullptr)
  {
    SUCCEED("The HDF5-OOC backend is not present in this build");
    return;
  }

  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceOutOfCore, 0);
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
  auto inputStore = data_store_utilities::CreateDataStore<uint8>(dataStructure, inputPath, tupleShape, {1});
  auto outputStore = data_store_utilities::CreateDataStore<uint8>(dataStructure, outputPath, tupleShape, {1});
  REQUIRE(inputStore != nullptr);
  REQUIRE(outputStore != nullptr);
  auto* inputArrayPtr = DataArray<uint8>::Create(dataStructure, "Input", inputStore, {});
  auto* outputArrayPtr = DataArray<uint8>::Create(dataStructure, "Output", outputStore, {});
  REQUIRE(inputArrayPtr != nullptr);
  REQUIRE(outputArrayPtr != nullptr);
  REQUIRE(inputArrayPtr->getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(outputArrayPtr->getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);

  std::vector<uint8> input(k_TotalValues);
  std::vector<uint8> expected(k_TotalValues);
  for(usize index = 0; index < k_TotalValues; ++index)
  {
    input[index] = static_cast<uint8>((index * 17 + index / k_PlaneValues * 3) % 11);
    expected[index] = input[index] == 0 ? uint8{1} : uint8{0};
  }
  Result<> copyFromBufferResult = inputArrayPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint8>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);

  const auto logicalNot = [](uint8 value) -> uint8 { return value == 0 ? uint8{1} : uint8{0}; };
  std::atomic_bool shouldCancel{false};
  const ImageProcessing::detail::PointwiseExecutionOptions options{.target_scratch_bytes = k_TargetScratchBytes, .force_buffered = false};
  const Result<> result = ImageProcessing::detail::ApplyPointwiseImpl(inputArrayPtr->getDataStoreRef(), outputArrayPtr->getDataStoreRef(), logicalNot, shouldCancel, {}, options);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  std::vector<uint8> actual(k_TotalValues);
  Result<> copyIntoBufferResult = outputArrayPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expected);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
