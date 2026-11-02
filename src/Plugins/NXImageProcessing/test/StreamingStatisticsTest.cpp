#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <mutex>
#include <numbers>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

using namespace nx::core;

namespace
{
template <class T>
class OocStatisticsStore : public DataStore<T>
{
public:
  explicit OocStatisticsStore(usize valueCount)
  : DataStore<T>(ShapeType{valueCount}, ShapeType{1}, T{})
  {
  }

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      ++m_ActiveReads;
      m_MaxConcurrentReads = std::max(m_MaxConcurrentReads, m_ActiveReads);
      m_ReadBatchValues.push_back(buffer.size());
    }
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    usize completedReads = 0;
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      --m_ActiveReads;
      completedReads = m_ReadBatchValues.size();
    }
    if(m_Cancel != nullptr && m_CancelAfterReadCalls > 0 && completedReads >= m_CancelAfterReadCalls)
    {
      m_Cancel->store(true);
    }
    return result;
  }

  void cancelAfterReadCalls(std::atomic_bool& shouldCancel, usize readCalls)
  {
    m_Cancel = &shouldCancel;
    m_CancelAfterReadCalls = readCalls;
  }

  [[nodiscard]] std::vector<usize> readBatchValues() const
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ReadBatchValues;
  }

  [[nodiscard]] usize maxConcurrentReads() const
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_MaxConcurrentReads;
  }

private:
  mutable std::mutex m_Mutex;
  mutable std::vector<usize> m_ReadBatchValues;
  mutable usize m_ActiveReads = 0;
  mutable usize m_MaxConcurrentReads = 0;
  std::atomic_bool* m_Cancel = nullptr;
  usize m_CancelAfterReadCalls = 0;
};

class StatisticsCacheBudgetScope
{
public:
  explicit StatisticsCacheBudgetScope(uint64 budgetBytes)
  : m_Manager(CacheMemoryBudgetManager::instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(budgetBytes);
  }

  ~StatisticsCacheBudgetScope()
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }

  StatisticsCacheBudgetScope(const StatisticsCacheBudgetScope&) = delete;
  StatisticsCacheBudgetScope& operator=(const StatisticsCacheBudgetScope&) = delete;

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget = 0;
};

template <class T>
void RequireStatisticsExactlyEqual(const ImageProcessing::ArrayStatistics<T>& actual, const ImageProcessing::ArrayStatistics<T>& expected)
{
  REQUIRE(actual.count == expected.count);
  REQUIRE(actual.min == expected.min);
  REQUIRE(actual.max == expected.max);
  REQUIRE(actual.sum == expected.sum);
  REQUIRE(actual.sumOfSquares == expected.sumOfSquares);
  REQUIRE(actual.mean == expected.mean);
  REQUIRE(actual.variance == expected.variance);
  REQUIRE(actual.sigma == expected.sigma);
}

template <class T>
ImageProcessing::ArrayMinMax<T> ComputeSerialMinMaxReference(const std::vector<T>& values)
{
  ImageProcessing::ArrayMinMax<T> result;
  result.count = values.size();
  if(values.empty())
  {
    return result;
  }
  result.min = values.front();
  result.max = values.front();
  for(usize index = 1; index < values.size(); ++index)
  {
    if(values[index] < result.min)
    {
      result.min = values[index];
    }
    if(values[index] > result.max)
    {
      result.max = values[index];
    }
  }
  return result;
}

void RequireMinMaxBitsEqual(const ImageProcessing::ArrayMinMax<float32>& actual, const ImageProcessing::ArrayMinMax<float32>& expected)
{
  REQUIRE(actual.count == expected.count);
  REQUIRE(std::bit_cast<uint32>(actual.min) == std::bit_cast<uint32>(expected.min));
  REQUIRE(std::bit_cast<uint32>(actual.max) == std::bit_cast<uint32>(expected.max));
}
} // namespace

TEST_CASE("ImageProcessing::StreamingStatistics: OOC default scan reservation scales with input bytes", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  constexpr usize k_ValueCount = 384ULL * k_MiB / sizeof(int16);
  OocStatisticsStore<int16> store(1);
  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(384 * k_MiB);
  {
    auto planResult = ImageProcessing::detail::CreateStreamingScanBufferPlan(store, k_ValueCount);
    SIMPLNX_RESULT_REQUIRE_VALID(planResult);
    REQUIRE(planResult.value().reservation.sizeBytes() == 96 * k_MiB);
    REQUIRE(planResult.value().batchValues == (96 * k_MiB) / sizeof(int16));
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::StreamingStatistics: OOC scan rejects a grant smaller than one input value", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  OocStatisticsStore<float32> store(1);
  const StatisticsCacheBudgetScope budgetScope(4);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(1);
  const auto planResult = ImageProcessing::detail::CreateStreamingScanBufferPlan(store);
  SIMPLNX_RESULT_REQUIRE_INVALID(planResult);
  REQUIRE(planResult.errors()[0].code == -8790);
  REQUIRE(planResult.errors()[0].message.find("1-byte") != std::string::npos);
  REQUIRE(planResult.errors()[0].message.find("4-byte") != std::string::npos);
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
}

TEST_CASE("ImageProcessing::StreamingStatistics: OOC normalize blocks match in-memory statistics exactly with serial bounded reads", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_NormalizeStatisticsBlockValues;
  constexpr usize k_ValueCount = 3 * k_BlockValues + 17;
  constexpr uint64 k_GrantedBytes = 2ULL * (k_BlockValues * sizeof(float64) + sizeof(ImageProcessing::detail::NormalizeStatisticsBlock<float64>));
  DataStore<float64> inMemoryStore(ShapeType{k_ValueCount}, ShapeType{1}, 0.0);
  OocStatisticsStore<float64> oocStore(k_ValueCount);
  constexpr std::array<float64, 8> k_Pattern = {1.0e12, 1.0, -1.0e12, 0.125, 3.141592653589793, -2.75, 1.0e-9, -1.0e-9};
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    const float64 value = k_Pattern[index % k_Pattern.size()];
    inMemoryStore.setValue(index, value);
    oocStore.setValue(index, value);
  }

  const StatisticsCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  const auto expectedResult = ImageProcessing::ComputeNormalizeStatistics<float64>(inMemoryStore, shouldCancel);
  const auto actualResult = ImageProcessing::ComputeNormalizeStatistics<float64>(oocStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(expectedResult);
  SIMPLNX_RESULT_REQUIRE_VALID(actualResult);
  RequireStatisticsExactlyEqual(actualResult.value(), expectedResult.value());

  const usize workerBlocks = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
  const usize batchBlocks = std::min<usize>(2, workerBlocks);
  const usize batchValues = batchBlocks * k_BlockValues;
  std::vector<usize> expectedReads;
  for(usize start = 0; start < k_ValueCount; start += batchValues)
  {
    expectedReads.push_back(std::min(batchValues, k_ValueCount - start));
  }
  REQUIRE(oocStore.readBatchValues() == expectedReads);
  REQUIRE(oocStore.maxConcurrentReads() == 1);
}

TEST_CASE("ImageProcessing::StreamingStatistics: OOC normalize handles a total shorter than one statistics block", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_ValueCount = ImageProcessing::detail::k_NormalizeStatisticsBlockValues / 2 + 3;
  constexpr uint64 k_GrantedBytes = static_cast<uint64>(k_ValueCount) * sizeof(float32);
  DataStore<float32> inMemoryStore(ShapeType{k_ValueCount}, ShapeType{1}, 0.0f);
  OocStatisticsStore<float32> oocStore(k_ValueCount);
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    const float32 value = static_cast<float32>(static_cast<int32>(index % 257) - 128) * 0.25f;
    inMemoryStore.setValue(index, value);
    oocStore.setValue(index, value);
  }

  const StatisticsCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  const auto expectedResult = ImageProcessing::ComputeNormalizeStatistics<float32>(inMemoryStore, shouldCancel);
  const auto actualResult = ImageProcessing::ComputeNormalizeStatistics<float32>(oocStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(expectedResult);
  SIMPLNX_RESULT_REQUIRE_VALID(actualResult);
  RequireStatisticsExactlyEqual(actualResult.value(), expectedResult.value());
  REQUIRE(oocStore.readBatchValues() == std::vector<usize>{k_ValueCount});
  REQUIRE(oocStore.maxConcurrentReads() == 1);
}

TEST_CASE("ImageProcessing::StreamingStatistics: an under-block OOC grant preserves fixed-block arithmetic with bounded reads", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_NormalizeStatisticsBlockValues;
  constexpr usize k_ValueCount = k_BlockValues + 19;
  constexpr usize k_GrantedValues = k_BlockValues / 2 - 1;
  constexpr uint64 k_GrantedBytes = static_cast<uint64>(k_GrantedValues) * sizeof(float64);
  DataStore<float64> inMemoryStore(ShapeType{k_ValueCount}, ShapeType{1}, 0.0);
  OocStatisticsStore<float64> oocStore(k_ValueCount);
  constexpr std::array<float64, 8> k_Pattern = {1.0e12, 1.0, -1.0e12, 0.125, 3.141592653589793, -2.75, 1.0e-9, -1.0e-9};
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    const float64 value = k_Pattern[index % k_Pattern.size()];
    inMemoryStore.setValue(index, value);
    oocStore.setValue(index, value);
  }

  const StatisticsCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  const auto expectedResult = ImageProcessing::ComputeNormalizeStatistics<float64>(inMemoryStore, shouldCancel);
  const auto actualResult = ImageProcessing::ComputeNormalizeStatistics<float64>(oocStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(expectedResult);
  SIMPLNX_RESULT_REQUIRE_VALID(actualResult);
  RequireStatisticsExactlyEqual(actualResult.value(), expectedResult.value());
  REQUIRE(oocStore.readBatchValues() == std::vector<usize>{k_GrantedValues, k_GrantedValues, 2, k_ValueCount - k_BlockValues});
  REQUIRE(oocStore.maxConcurrentReads() == 1);
}

TEST_CASE("ImageProcessing::StreamingStatistics: cancellation after the final OOC read skips its block merge", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_NormalizeStatisticsBlockValues;
  constexpr usize k_ValueCount = 2 * k_BlockValues + 17;
  constexpr uint64 k_GrantedBytes = static_cast<uint64>(k_BlockValues) * sizeof(int16);
  OocStatisticsStore<int16> oocStore(k_ValueCount);
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    oocStore.setValue(index, static_cast<int16>(index % 101));
  }

  const StatisticsCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  oocStore.cancelAfterReadCalls(shouldCancel, 3);
  const auto result = ImageProcessing::ComputeNormalizeStatistics<int16>(oocStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(shouldCancel.load());
  REQUIRE(result.value().count == 2 * k_BlockValues);
  REQUIRE(oocStore.readBatchValues() == std::vector<usize>{k_BlockValues, k_BlockValues, 17});
  REQUIRE(oocStore.maxConcurrentReads() == 1);
}

TEMPLATE_TEST_CASE("ImageProcessing::StreamingStatistics: ComputeArraySum matches serial statistics sum", "[ImageProcessing][StreamingStatistics]", int16, uint16, float32, float64)
{
  using T = TestType;
  std::vector<T> values = {static_cast<T>(1), static_cast<T>(3), static_cast<T>(5), static_cast<T>(7), static_cast<T>(11), static_cast<T>(13), static_cast<T>(17)};
  DataStore<T> store(ShapeType{values.size()}, ShapeType{1}, T{});
  const Result<> copyResult = store.copyFromBuffer(0, values);
  SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
  std::atomic_bool shouldCancel{false};
  const auto statistics = ImageProcessing::ComputeArrayStatistics(store, shouldCancel);
  const auto sum = ImageProcessing::ComputeArraySum(store, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(statistics);
  SIMPLNX_RESULT_REQUIRE_VALID(sum);
  REQUIRE(sum.value() == statistics.value().sum);
}

TEST_CASE("ImageProcessing::StreamingStatistics: ComputeArraySum OOC blocks are serial with a short final batch", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_NormalizeStatisticsBlockValues;
  constexpr usize k_ValueCount = 2 * k_BlockValues + 17;
  const std::array<float64, 8> pattern = {1.0e16, 1.0, -1.0e16, 0.125, std::numbers::pi, -2.75, 1.0e-9, -1.0e-9};
  OocStatisticsStore<float64> oocStore(k_ValueCount);
  DataStore<float64> inMemoryStore(ShapeType{k_ValueCount}, ShapeType{1}, 0.0);
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    const float64 value = pattern[index % pattern.size()];
    oocStore.setValue(index, value);
    inMemoryStore.setValue(index, value);
  }
  ImageProcessing::ScopedWorkingMemoryTuningOverride tuningOverride(k_BlockValues * sizeof(float64));
  std::atomic_bool shouldCancel{false};
  const auto expected = ImageProcessing::ComputeArrayStatistics(inMemoryStore, shouldCancel);
  const auto actual = ImageProcessing::ComputeArraySum(oocStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(expected);
  SIMPLNX_RESULT_REQUIRE_VALID(actual);
  REQUIRE(actual.value() == expected.value().sum);
  REQUIRE(oocStore.readBatchValues() == std::vector<usize>{k_BlockValues, k_BlockValues, 17});
  REQUIRE(oocStore.maxConcurrentReads() == 1);
}

TEST_CASE("ImageProcessing::StreamingStatistics: exact integral sum plan is bounded and overflow-safe", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_ArraySumBlockValues;
  constexpr usize k_ValueBytes = sizeof(int16);
  constexpr usize k_TwoBlockPayload = 2 * k_BlockValues * k_ValueBytes + 2 * sizeof(int64);
  const auto plan = ImageProcessing::detail::CreateExactIntegralSumBatchPlan(/*totalValues=*/2 * k_BlockValues + 17, k_ValueBytes, k_TwoBlockPayload, /*workerCount=*/8);
  REQUIRE(plan.has_value());
  REQUIRE(plan->batchValues == 2 * k_BlockValues);
  REQUIRE(plan->blockCapacity == 2);
  REQUIRE(plan->residentBytes == k_TwoBlockPayload);

  REQUIRE_FALSE(ImageProcessing::detail::CreateExactIntegralSumBatchPlan(/*totalValues=*/k_BlockValues, k_ValueBytes,
                                                                         /*availableBytes=*/k_BlockValues * k_ValueBytes, /*workerCount=*/8)
                    .has_value());
  REQUIRE_FALSE(ImageProcessing::detail::CreateExactIntegralSumBatchPlan(/*totalValues=*/k_BlockValues, /*valueBytes=*/0, std::numeric_limits<usize>::max(), /*workerCount=*/8).has_value());
  REQUIRE_FALSE(ImageProcessing::detail::CreateExactIntegralSumBatchPlan(std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max(),
                                                                         /*workerCount=*/8)
                    .has_value());

  constexpr usize k_Int32ExactBoundary = (usize{1} << 53) / (uint64{1} << 31);
  REQUIRE(ImageProcessing::detail::CanUseExactIntegralArraySum<int16>(268'435'456));
  REQUIRE(ImageProcessing::detail::CanUseExactIntegralArraySum<int32>(k_Int32ExactBoundary));
  REQUIRE_FALSE(ImageProcessing::detail::CanUseExactIntegralArraySum<int32>(k_Int32ExactBoundary + 1));
  REQUIRE_FALSE(ImageProcessing::detail::CanUseExactIntegralArraySum<uint64>(1));
  REQUIRE_FALSE(ImageProcessing::detail::CanUseExactIntegralArraySum<float32>(1));
}

TEST_CASE("ImageProcessing::StreamingStatistics: exact integral OOC sum uses parallel bounded blocks", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_ArraySumBlockValues;
  constexpr usize k_ValueCount = 2 * k_BlockValues + 17;
  constexpr uint64 k_GrantedBytes = 2ULL * k_BlockValues * sizeof(int16) + 2ULL * sizeof(int64);
  DataStore<int16> inMemoryStore(ShapeType{k_ValueCount}, ShapeType{1}, int16{});
  OocStatisticsStore<int16> oocStore(k_ValueCount);
  constexpr std::array<int16, 8> k_Pattern = {32767, -32768, 1, -1, 73, -91, 2048, -4096};
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    const int16 value = k_Pattern[index % k_Pattern.size()];
    inMemoryStore.setValue(index, value);
    oocStore.setValue(index, value);
  }

  const StatisticsCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride tuningOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  const auto expected = ImageProcessing::ComputeArrayStatistics(inMemoryStore, shouldCancel);
  ImageProcessing::detail::ArraySumExecutionDetails execution;
  const auto actual = ImageProcessing::ComputeArraySum(oocStore, shouldCancel, &execution);
  SIMPLNX_RESULT_REQUIRE_VALID(expected);
  SIMPLNX_RESULT_REQUIRE_VALID(actual);
  REQUIRE(actual.value() == expected.value().sum);
  REQUIRE(execution.usedExactIntegralParallel);
  REQUIRE(execution.blockCapacity == 2);
  REQUIRE(execution.residentBytes == k_GrantedBytes);
  REQUIRE(oocStore.readBatchValues() == std::vector<usize>{2 * k_BlockValues, 17});
  REQUIRE(oocStore.maxConcurrentReads() == 1);
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
}

TEMPLATE_TEST_CASE("ImageProcessing::StreamingStatistics: unsafe integral and floating sums keep serial Kahan", "[ImageProcessing][StreamingStatistics]", uint64, float64)
{
  using T = TestType;
  constexpr usize k_ValueCount = 17;
  DataStore<T> store(ShapeType{k_ValueCount}, ShapeType{1}, T{});
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    store.setValue(index, static_cast<T>(index + 1));
  }
  std::atomic_bool shouldCancel{false};
  const auto expected = ImageProcessing::ComputeArrayStatistics(store, shouldCancel);
  ImageProcessing::detail::ArraySumExecutionDetails execution;
  const auto actual = ImageProcessing::ComputeArraySum(store, shouldCancel, &execution);
  SIMPLNX_RESULT_REQUIRE_VALID(expected);
  SIMPLNX_RESULT_REQUIRE_VALID(actual);
  REQUIRE(actual.value() == expected.value().sum);
  REQUIRE_FALSE(execution.usedExactIntegralParallel);
}

TEST_CASE("ImageProcessing::StreamingStatistics: cancelled integral sum does not merge a partial batch", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_ArraySumBlockValues;
  constexpr usize k_ValueCount = k_BlockValues + 4;
  constexpr uint64 k_GrantedBytes = static_cast<uint64>(k_BlockValues) * sizeof(int16) + sizeof(int64);
  OocStatisticsStore<int16> oocStore(k_ValueCount);
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    oocStore.setValue(index, int16{1});
  }
  const StatisticsCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride tuningOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  oocStore.cancelAfterReadCalls(shouldCancel, 1);
  ImageProcessing::detail::ArraySumExecutionDetails execution;
  const auto result = ImageProcessing::ComputeArraySum(oocStore, shouldCancel, &execution);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(shouldCancel.load());
  REQUIRE(result.value() == 0.0);
  REQUIRE(execution.usedExactIntegralParallel);
  REQUIRE(oocStore.readBatchValues() == std::vector<usize>{k_BlockValues});
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
}

// Regression for the sqrt-of-negative NaN in ComputeArrayStatistics: the one-pass (SumSq - Sum^2/N) variance
// form can cancel a few ULPs NEGATIVE on a constant / near-uniform image, which would make std::sqrt return NaN
// and poison sigma (and NormalizeImageFilter, which divides by it). Variance is non-negative by definition, so
// sigma must be finite and 0 for a constant image, never NaN.
TEST_CASE("ImageProcessing::StreamingStatistics: constant image yields finite zero variance/sigma", "[ImageProcessing][StreamingStatistics]")
{
  std::atomic_bool shouldCancel{false};

  SECTION("float32 constant 0.1 (rounding makes Sum^2/N vs SumSq cancellation-prone)")
  {
    DataStore<float32> store(ShapeType{4, 5, 6}, ShapeType{1}, 0.1f);
    const auto result = ImageProcessing::ComputeArrayStatistics<float32>(store, shouldCancel);
    REQUIRE(result.valid());
    const ImageProcessing::ArrayStatistics<float32>& stats = result.value();
    REQUIRE(stats.min == 0.1f);
    REQUIRE(stats.max == 0.1f);
    REQUIRE(std::isfinite(stats.variance));
    REQUIRE(stats.variance == 0.0);
    REQUIRE(std::isfinite(stats.sigma));
    REQUIRE(stats.sigma == 0.0);
  }

  SECTION("float64 constant large magnitude")
  {
    DataStore<float64> store(ShapeType{3, 3, 3}, ShapeType{1}, 1.0e6);
    const auto result = ImageProcessing::ComputeArrayStatistics<float64>(store, shouldCancel);
    REQUIRE(result.valid());
    const ImageProcessing::ArrayStatistics<float64>& stats = result.value();
    REQUIRE(std::isfinite(stats.sigma));
    REQUIRE(stats.sigma == 0.0);
  }

  SECTION("single voxel: n==1 -> variance/sigma 0, no divide-by-(n-1)")
  {
    DataStore<int32> store(ShapeType{1, 1, 1}, ShapeType{1}, 42);
    const auto result = ImageProcessing::ComputeArrayStatistics<int32>(store, shouldCancel);
    REQUIRE(result.valid());
    const ImageProcessing::ArrayStatistics<int32>& stats = result.value();
    REQUIRE(stats.count == 1);
    REQUIRE(stats.min == 42);
    REQUIRE(stats.max == 42);
    REQUIRE(stats.variance == 0.0);
    REQUIRE(stats.sigma == 0.0);
  }
}

TEST_CASE("ImageProcessing::StreamingStatistics: min-max-only reduction", "[ImageProcessing][StreamingStatistics]")
{
  std::atomic_bool shouldCancel{false};

  SECTION("finds extrema and ignores a later NaN")
  {
    DataStore<float32> store(ShapeType{6}, ShapeType{1}, 0.0f);
    const std::array<float32, 6> values = {5.0f, -3.0f, std::numeric_limits<float32>::quiet_NaN(), 9.0f, 2.0f, -1.0f};
    for(usize i = 0; i < values.size(); ++i)
    {
      store.setValue(i, values[i]);
    }

    const auto result = ImageProcessing::ComputeArrayMinMax<float32>(store, shouldCancel);
    REQUIRE(result.valid());
    REQUIRE(result.value().count == values.size());
    REQUIRE(result.value().min == -3.0f);
    REQUIRE(result.value().max == 9.0f);
  }

  SECTION("propagates a leading NaN like the full sequential statistics pass")
  {
    DataStore<float32> store(ShapeType{3}, ShapeType{1}, 1.0f);
    store.setValue(0, std::numeric_limits<float32>::quiet_NaN());
    const auto result = ImageProcessing::ComputeArrayMinMax<float32>(store, shouldCancel);
    REQUIRE(result.valid());
    REQUIRE(std::isnan(result.value().min));
    REQUIRE(std::isnan(result.value().max));
  }

  SECTION("combines extrema from separate parallel partitions")
  {
    constexpr usize k_ValueCount = 262145;
    DataStore<int32> store(ShapeType{k_ValueCount}, ShapeType{1}, 7);
    store.setValue(1024, -123456);
    store.setValue(k_ValueCount - 2048, 654321);

    const auto result = ImageProcessing::ComputeArrayMinMax<int32>(store, shouldCancel);
    REQUIRE(result.valid());
    REQUIRE(result.value().count == k_ValueCount);
    REQUIRE(result.value().min == -123456);
    REQUIRE(result.value().max == 654321);
  }
}

TEST_CASE("ImageProcessing::StreamingStatistics: OOC fixed-block min-max preserves serial special values", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_ArrayMinMaxBlockValues;
  constexpr usize k_ValueCount = 2 * k_BlockValues + 17;
  constexpr uint64 k_GrantedBytes = 2ULL * k_BlockValues * sizeof(float32) + 2ULL * sizeof(ImageProcessing::detail::ArrayMinMaxBlock<float32>);

  const auto runCase = [&](const std::vector<float32>& values) {
    OocStatisticsStore<float32> oocStore(values.size());
    for(usize index = 0; index < values.size(); ++index)
    {
      oocStore.setValue(index, values[index]);
    }

    const StatisticsCacheBudgetScope budgetScope(4 * k_GrantedBytes);
    const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_GrantedBytes);
    usize batchValues = 0;
    usize blockCapacity = 0;
    {
      const auto planResult = ImageProcessing::detail::CreateArrayMinMaxBatchPlan<float32>(oocStore);
      SIMPLNX_RESULT_REQUIRE_VALID(planResult);
      batchValues = planResult.value().batchValues;
      blockCapacity = planResult.value().blockCapacity;
      REQUIRE(blockCapacity >= 2);
      REQUIRE(planResult.value().residentBytes <= k_GrantedBytes);
    }

    std::atomic_bool shouldCancel{false};
    ImageProcessing::detail::ArrayMinMaxExecutionDetails execution;
    const auto actualResult = ImageProcessing::ComputeArrayMinMax<float32>(oocStore, shouldCancel, &execution);
    SIMPLNX_RESULT_REQUIRE_VALID(actualResult);
    RequireMinMaxBitsEqual(actualResult.value(), ComputeSerialMinMaxReference(values));
    const std::vector<usize> reads = oocStore.readBatchValues();
    REQUIRE(std::accumulate(reads.cbegin(), reads.cend(), usize{0}) == values.size());
    REQUIRE(reads.size() >= 2);
    REQUIRE(reads.back() < batchValues);
    REQUIRE(oocStore.maxConcurrentReads() == 1);
    REQUIRE(execution.usedFixedBlockParallel);
    REQUIRE(execution.blockCapacity == blockCapacity);
    REQUIRE(execution.residentBytes == batchValues * sizeof(float32) + blockCapacity * sizeof(ImageProcessing::detail::ArrayMinMaxBlock<float32>));
    REQUIRE(execution.residentBytes <= k_GrantedBytes);
  };

  SECTION("later NaNs do not replace the first signed zero")
  {
    std::vector<float32> values(k_ValueCount, std::bit_cast<float32>(uint32{0}));
    values[1] = std::bit_cast<float32>(uint32{0x80000000});
    values[k_BlockValues + 3] = std::bit_cast<float32>(uint32{0x7FC01234});
    runCase(values);
  }

  SECTION("infinities remain extrema while later NaNs are ignored")
  {
    std::vector<float32> values(k_ValueCount, 7.0F);
    values[k_BlockValues - 1] = std::numeric_limits<float32>::infinity();
    values[k_BlockValues + 2] = -std::numeric_limits<float32>::infinity();
    values[k_ValueCount - 1] = std::bit_cast<float32>(uint32{0x7FC05678});
    runCase(values);
  }

  SECTION("the leading NaN payload remains the result")
  {
    std::vector<float32> values(k_ValueCount, 3.0F);
    values[0] = std::bit_cast<float32>(uint32{0x7FC0ABCD});
    values[k_BlockValues] = -std::numeric_limits<float32>::infinity();
    values[k_ValueCount - 1] = std::numeric_limits<float32>::infinity();
    runCase(values);
  }
}

TEST_CASE("ImageProcessing::StreamingStatistics: OOC fixed-block min-max cancels before the first merge", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_ArrayMinMaxBlockValues;
  constexpr usize k_ValueCount = 2 * k_BlockValues + 17;
  constexpr uint64 k_GrantedBytes = 2ULL * k_BlockValues * sizeof(float32) + 2ULL * sizeof(ImageProcessing::detail::ArrayMinMaxBlock<float32>);
  OocStatisticsStore<float32> oocStore(k_ValueCount);
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    oocStore.setValue(index, 42.0F);
  }

  const StatisticsCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  oocStore.cancelAfterReadCalls(shouldCancel, 1);
  ImageProcessing::detail::ArrayMinMaxExecutionDetails execution;
  const auto result = ImageProcessing::ComputeArrayMinMax<float32>(oocStore, shouldCancel, &execution);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(shouldCancel.load());
  REQUIRE(result.value().count == k_ValueCount);
  REQUIRE(result.value().min == 0.0F);
  REQUIRE(result.value().max == 0.0F);
  REQUIRE(oocStore.readBatchValues().size() == 1);
  REQUIRE(execution.usedFixedBlockParallel);
}

TEST_CASE("ImageProcessing::StreamingStatistics: OOC fixed-block min-max does not return a completed batch after cancellation", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  constexpr usize k_BlockValues = ImageProcessing::detail::k_ArrayMinMaxBlockValues;
  constexpr usize k_ValueCount = 3 * k_BlockValues + 17;
  constexpr uint64 k_GrantedBytes = 2ULL * k_BlockValues * sizeof(float32) + 2ULL * sizeof(ImageProcessing::detail::ArrayMinMaxBlock<float32>);
  OocStatisticsStore<float32> oocStore(k_ValueCount);
  for(usize index = 0; index < k_ValueCount; ++index)
  {
    oocStore.setValue(index, index == 0 ? -7.0F : 42.0F);
  }

  const StatisticsCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  oocStore.cancelAfterReadCalls(shouldCancel, 2);
  const auto result = ImageProcessing::ComputeArrayMinMax<float32>(oocStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(shouldCancel.load());
  REQUIRE(result.value().count == k_ValueCount);
  REQUIRE(result.value().min == 0.0F);
  REQUIRE(result.value().max == 0.0F);
  REQUIRE(oocStore.readBatchValues().size() == 2);
}

TEST_CASE("ImageProcessing::StreamingStatistics: OOC fixed-block min-max handles an empty store", "[ImageProcessing][StreamingStatistics][WorkingMemory]")
{
  OocStatisticsStore<float32> oocStore(0);
  std::atomic_bool shouldCancel{false};
  ImageProcessing::detail::ArrayMinMaxExecutionDetails execution;
  const auto result = ImageProcessing::ComputeArrayMinMax<float32>(oocStore, shouldCancel, &execution);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.value().count == 0);
  REQUIRE(oocStore.readBatchValues().empty());
  REQUIRE_FALSE(execution.usedFixedBlockParallel);
}

TEST_CASE("ImageProcessing::StreamingStatistics: parallel normalize statistics match streaming", "[ImageProcessing][StreamingStatistics]")
{
  constexpr usize k_ValueCount = 600013;
  DataStore<int16> store(ShapeType{k_ValueCount}, ShapeType{1}, 0);
  for(usize i = 0; i < k_ValueCount; ++i)
  {
    store.setValue(i, static_cast<int16>(static_cast<int32>((i * 37) % 1001) - 500));
  }
  std::atomic_bool shouldCancel{false};
  const auto streamingResult = ImageProcessing::ComputeArrayStatistics<int16>(store, shouldCancel);
  const auto parallelResult = ImageProcessing::ComputeNormalizeStatistics<int16>(store, shouldCancel);
  REQUIRE(streamingResult.valid());
  REQUIRE(parallelResult.valid());
  const auto& streaming = streamingResult.value();
  const auto& parallel = parallelResult.value();
  REQUIRE(parallel.count == streaming.count);
  REQUIRE(parallel.min == streaming.min);
  REQUIRE(parallel.max == streaming.max);
  REQUIRE(parallel.mean == Approx(streaming.mean).margin(1.0e-12));
  REQUIRE(parallel.sigma == Approx(streaming.sigma).margin(1.0e-12));
}
