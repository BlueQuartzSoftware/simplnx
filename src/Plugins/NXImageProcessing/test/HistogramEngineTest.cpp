#include "simplnx/Utilities/ImageProcessing/HistogramEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
template <class T>
class CountingReadDataStore : public DataStore<T>
{
public:
  CountingReadDataStore(usize size, std::optional<T> initialValue)
  : DataStore<T>({size}, {1}, initialValue)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    m_MaximumReadValues = std::max(m_MaximumReadValues, buffer.size());
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

  void resetReadCount() const noexcept
  {
    m_ReadCount = 0;
    m_MaximumReadValues = 0;
  }

  usize maximumReadValues() const noexcept
  {
    return m_MaximumReadValues;
  }

private:
  mutable usize m_ReadCount = 0;
  mutable usize m_MaximumReadValues = 0;
};

template <class T>
class OutOfCoreCountingReadDataStore : public CountingReadDataStore<T>
{
public:
  using CountingReadDataStore<T>::CountingReadDataStore;
  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }
};

template <class T>
class FailingReadDataStore : public DataStore<T>
{
public:
  explicit FailingReadDataStore(int32 errorCode)
  : DataStore<T>({8}, {1}, T{})
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyIntoBuffer(usize, nonstd::span<T>) const override
  {
    return MakeErrorResult(m_ErrorCode, "Injected histogram read failure");
  }

private:
  int32 m_ErrorCode = 0;
};

template <class T>
class CancelAfterReadDataStore : public CountingReadDataStore<T>
{
public:
  CancelAfterReadDataStore(usize size, T initialValue, std::atomic_bool& shouldCancel)
  : CountingReadDataStore<T>(size, initialValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = CountingReadDataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(result.valid())
    {
      m_ShouldCancel.store(true);
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
};

template <class T>
void RequireIntegralHistogramParity(const std::vector<T>& values, usize numBins, std::string_view typeName)
{
  INFO("type=" << typeName);
  DataStore<T> store({values.size()}, {1}, T{});
  for(usize i = 0; i < values.size(); ++i)
  {
    store.setValue(i, values[i]);
  }

  std::atomic_bool cancel{false};
  const Result<ArrayStatistics<T>> stats = ComputeArrayStatistics<T>(store, cancel);
  REQUIRE(stats.valid());
  const Result<Histogram> reference = ComputeHistogram<T>(store, numBins, static_cast<float64>(stats.value().min), static_cast<float64>(stats.value().max), cancel);
  REQUIRE(reference.valid());
  const Result<Histogram> fused = ComputeIntegralHistogram<T>(store, numBins, cancel);
  REQUIRE(fused.valid());

  REQUIRE(fused.value().min == reference.value().min);
  REQUIRE(fused.value().max == reference.value().max);
  REQUIRE(fused.value().binWidth == reference.value().binWidth);
  REQUIRE(fused.value().counts == reference.value().counts);
}

template <class T>
void RequireCompactTypeParity(std::string_view typeName)
{
  if constexpr(std::is_signed_v<T>)
  {
    RequireIntegralHistogramParity<T>({static_cast<T>(-31), static_cast<T>(-7), static_cast<T>(-7), static_cast<T>(0), static_cast<T>(4), static_cast<T>(19), static_cast<T>(31)}, 17, typeName);
  }
  else
  {
    RequireIntegralHistogramParity<T>({static_cast<T>(0), static_cast<T>(3), static_cast<T>(3), static_cast<T>(9), static_cast<T>(17), static_cast<T>(29), static_cast<T>(63)}, 17, typeName);
  }
}

template <class T>
void RequireCompactCountingParity(const std::vector<T>& values, usize numBins, std::string_view caseName)
{
  INFO("case=" << caseName);
  CountingReadDataStore<T> store(values.size(), T{});
  for(usize i = 0; i < values.size(); ++i)
  {
    store.setValue(i, values[i]);
  }

  std::atomic_bool cancel{false};
  const Result<ArrayStatistics<T>> stats = ComputeArrayStatistics<T>(store, cancel);
  REQUIRE(stats.valid());
  store.resetReadCount();
  const Result<Histogram> reference = ComputeHistogram<T>(store, numBins, static_cast<float64>(stats.value().min), static_cast<float64>(stats.value().max), cancel);
  REQUIRE(reference.valid());
  REQUIRE(store.readCount() == 1);

  store.resetReadCount();
  const Result<Histogram> fused = ComputeIntegralHistogram<T>(store, numBins, cancel);
  REQUIRE(fused.valid());
  REQUIRE(store.readCount() == 1);
  REQUIRE(fused.value().min != fused.value().max);
  REQUIRE(fused.value().min == reference.value().min);
  REQUIRE(fused.value().max == reference.value().max);
  REQUIRE(fused.value().binWidth == reference.value().binWidth);
  REQUIRE(fused.value().counts == reference.value().counts);
}
} // namespace

TEST_CASE("ImageProcessing::HistogramEngine: counts + Otsu bimodal", "[ImageProcessing][HistogramEngine]")
{
  // (a) ComputeHistogram: values 0..9, 10 bins, min=0, max=9 -> one count per bin; value==max lands in last bin.
  DataStore<float32> store({10}, {1}, 0.0f);
  for(usize i = 0; i < 10; ++i)
  {
    store.setValue(i, static_cast<float32>(i));
  }
  std::atomic_bool cancel{false};
  auto h = ComputeHistogram<float32>(store, 10, 0.0, 9.0, cancel);
  REQUIRE(h.valid());
  for(usize b = 0; b < 10; ++b)
  {
    REQUIRE(h.value().counts[b] == 1);
  }
  // Bin width reproduces ITK's marginal-scale widening: upper = max + (max-min)/(numBins*100), so binWidth =
  // (upper - min)/numBins = (9 + 9/1000)/10 = 0.9009 (NOT the naive (9-0)/10 = 0.9).
  REQUIRE(h.value().binWidth == Approx((9.0 + 9.0 / (10.0 * 100.0)) / 10.0));

  // (b) OtsuMultipleThresholds: clean bimodal histogram, single threshold in the valley.
  Histogram bim;
  bim.min = 0.0;
  bim.max = 9.0;
  bim.binWidth = 1.0;
  bim.counts = {50, 50, 0, 0, 0, 0, 0, 0, 50, 50};
  auto th = OtsuMultipleThresholds(bim, 1, /*valley*/ false, /*binMidpoint*/ false);
  REQUIRE(th.size() == 1);
  REQUIRE(th[0] >= 1.0);
  REQUIRE(th[0] <= 8.0);
}

TEST_CASE("ImageProcessing::HistogramEngine: Otsu trimodal, two thresholds", "[ImageProcessing][HistogramEngine]")
{
  // Clean trimodal histogram (20 bins): three separated, equal-mass modes with empty valleys between them.
  //   mode A: bins 1-2, mode B: bins 9-10, mode C: bins 17-18
  Histogram tri;
  tri.min = 0.0;
  tri.max = 20.0;
  tri.binWidth = 1.0;
  tri.counts = {0, 50, 50, 0, 0, 0, 0, 0, 0, 50, 50, 0, 0, 0, 0, 0, 0, 50, 50, 0};

  auto th = OtsuMultipleThresholds(tri, 2, /*valley*/ false, /*binMidpoint*/ false);
  REQUIRE(th.size() == 2);
  // First threshold separates mode A from mode B (in the bin 3..8 valley).
  REQUIRE(th[0] > 2.0);
  REQUIRE(th[0] < 9.5);
  // Second threshold separates mode B from mode C (in the bin 11..16 valley).
  REQUIRE(th[1] > 10.0);
  REQUIRE(th[1] < 17.5);
  REQUIRE(th[0] < th[1]);
}

TEST_CASE("ImageProcessing::HistogramEngine: Otsu guards + options", "[ImageProcessing][HistogramEngine]")
{
  Histogram bim;
  bim.min = 0.0;
  bim.max = 9.0;
  bim.binWidth = 1.0;
  bim.counts = {50, 50, 0, 0, 0, 0, 0, 0, 50, 50};

  SECTION("numThresholds >= numBins returns an empty vector (no OOB)")
  {
    Histogram tiny;
    tiny.min = 0.0;
    tiny.max = 3.0;
    tiny.binWidth = 1.0;
    tiny.counts = {10, 0, 20}; // 3 bins
    REQUIRE(OtsuMultipleThresholds(tiny, 3, false, false).empty());
    REQUIRE(OtsuMultipleThresholds(tiny, 5, false, false).empty());
  }

  SECTION("returnBinMidpoint shifts the threshold by half a bin vs bin-max")
  {
    auto thMax = OtsuMultipleThresholds(bim, 1, /*valley*/ false, /*binMidpoint*/ false);
    auto thMid = OtsuMultipleThresholds(bim, 1, /*valley*/ false, /*binMidpoint*/ true);
    REQUIRE(thMax.size() == 1);
    REQUIRE(thMid.size() == 1);
    // Same selected bin, different in-bin offset: bin-max is exactly half a bin above the midpoint.
    REQUIRE(thMax[0] == Approx(thMid[0] + 0.5 * bim.binWidth));
  }

  SECTION("valleyEmphasis=true runs and returns the requested threshold count")
  {
    auto th = OtsuMultipleThresholds(bim, 1, /*valley*/ true, /*binMidpoint*/ false);
    REQUIRE(th.size() == 1);
    REQUIRE(th[0] >= 1.0);
    REQUIRE(th[0] <= 8.0);
  }
}

TEST_CASE("ImageProcessing::HistogramEngine: reservation-owned scan policy honors default and explicit capacities", "[ImageProcessing][HistogramEngine]")
{
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(384ULL * k_MiB);
  {
    OutOfCoreCountingReadDataStore<uint8> oocStore(384ULL * k_MiB, uint8{0});
    const auto scan = ImageProcessing::detail::ReserveHistogramScanBuffer<uint8>(oocStore, 384ULL * k_MiB, {});
    REQUIRE(scan.reservation.sizeBytes() == 96ULL * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);

  CountingReadDataStore<int32> store(23, 0);
  for(usize index = 0; index < store.getSize(); ++index)
  {
    store.setValue(index, static_cast<int32>(index));
  }
  std::atomic_bool cancel{false};
  const HistogramScanOptions options{5 * sizeof(int32)};
  const auto histogram = ComputeHistogram(store, 8, 0.0, 22.0, cancel, options);
  REQUIRE(histogram.valid());
  REQUIRE(store.readCount() == 5);
  REQUIRE(store.maximumReadValues() == 5);
}

TEST_CASE("ImageProcessing::HistogramEngine: ComputeHistogram degenerate range", "[ImageProcessing][HistogramEngine]")
{
  // Constant-valued store: min == max, so the range is degenerate and all mass goes to bin 0.
  DataStore<float32> store({8}, {1}, 5.0f);
  std::atomic_bool cancel{false};
  auto h = ComputeHistogram<float32>(store, 4, 5.0, 5.0, cancel);
  REQUIRE(h.valid());
  REQUIRE(h.value().counts.size() == 4);
  REQUIRE(h.value().counts[0] == 8);
  for(usize b = 1; b < 4; ++b)
  {
    REQUIRE(h.value().counts[b] == 0);
  }
}

TEST_CASE("ImageProcessing::HistogramEngine: fused integral histogram matches the existing two-pass result for every integral type", "[ImageProcessing][HistogramEngine]")
{
  RequireCompactTypeParity<int8>("int8");
  RequireCompactTypeParity<uint8>("uint8");
  RequireCompactTypeParity<int16>("int16");
  RequireCompactTypeParity<uint16>("uint16");
  RequireCompactTypeParity<int32>("int32");
  RequireCompactTypeParity<uint32>("uint32");
  RequireCompactTypeParity<int64>("int64");
  RequireCompactTypeParity<uint64>("uint64");
}

TEST_CASE("ImageProcessing::HistogramEngine: fused integral compact and fallback boundaries preserve exact histograms", "[ImageProcessing][HistogramEngine]")
{
  SECTION("signed int64 cross-zero span of exactly 65535 stays compact")
  {
    RequireIntegralHistogramParity<int64>({-32768, -32767, -1, 0, 1, 32766, 32767}, 128, "int64 cross-zero compact boundary");
  }
  SECTION("span of 65536 takes the exact fallback")
  {
    RequireIntegralHistogramParity<int32>({-32768, -1, 0, 1, 32768}, 128, "int32 wide boundary");
  }
  SECTION("int64 extrema")
  {
    RequireIntegralHistogramParity<int64>(
        {std::numeric_limits<int64>::lowest(), std::numeric_limits<int64>::lowest() + 1, -1, 0, 1, std::numeric_limits<int64>::max() - 1, std::numeric_limits<int64>::max()}, 256, "int64 extrema");
  }
  SECTION("uint64 near maximum compact range")
  {
    constexpr uint64 k_Max = std::numeric_limits<uint64>::max();
    RequireCompactCountingParity<uint64>({k_Max - 65535, k_Max - 61440, k_Max - 32768, k_Max - 1, k_Max}, 128, "uint64 near-maximum compact boundary");
  }
  SECTION("int64 near minimum compact range")
  {
    constexpr int64 k_Min = std::numeric_limits<int64>::lowest();
    RequireCompactCountingParity<int64>({k_Min, k_Min + 1, k_Min + 4096, k_Min + 32768, k_Min + 65535}, 128, "int64 near-minimum compact boundary");
  }
  SECTION("uint64 extrema")
  {
    RequireIntegralHistogramParity<uint64>({0, 1, std::numeric_limits<uint64>::max() - 1, std::numeric_limits<uint64>::max()}, 128, "uint64 extrema");
  }
  SECTION("constant")
  {
    RequireIntegralHistogramParity<int16>({42, 42, 42, 42}, 8, "constant");
  }
  SECTION("empty")
  {
    RequireIntegralHistogramParity<uint32>({}, 8, "empty");
  }
  SECTION("short cancellation-check tail")
  {
    std::vector<int32> values(65539);
    for(usize i = 0; i < values.size(); ++i)
    {
      values[i] = static_cast<int32>((i * 17) % 251) - 125;
    }
    RequireIntegralHistogramParity<int32>(values, 64, "short tail");
  }
}

TEST_CASE("ImageProcessing::HistogramEngine: fused integral histogram uses one compact pass and two fallback passes", "[ImageProcessing][HistogramEngine]")
{
  std::atomic_bool cancel{false};
  SECTION("compact")
  {
    CountingReadDataStore<int32> store(7, 0);
    const std::array<int32, 7> values{-20, -1, 0, 1, 7, 20, 20};
    for(usize i = 0; i < values.size(); ++i)
    {
      store.setValue(i, values[i]);
    }
    const auto histogram = ComputeIntegralHistogram<int32>(store, 16, cancel);
    REQUIRE(histogram.valid());
    REQUIRE(store.readCount() == 1);
  }
  SECTION("wide fallback")
  {
    CountingReadDataStore<int32> store(3, 0);
    store.setValue(0, -32768);
    store.setValue(1, 0);
    store.setValue(2, 32768);
    const auto histogram = ComputeIntegralHistogram<int32>(store, 16, cancel);
    REQUIRE(histogram.valid());
    REQUIRE(store.readCount() == 2);
  }
  SECTION("pre-cancel performs no read")
  {
    CountingReadDataStore<int16> store(8, 3);
    cancel.store(true);
    const auto histogram = ComputeIntegralHistogram<int16>(store, 16, cancel);
    REQUIRE(histogram.valid());
    REQUIRE(histogram.value().counts.size() == 16);
    REQUIRE(store.readCount() == 0);
  }
  SECTION("zero bins performs no read")
  {
    CountingReadDataStore<uint32> store(8, 3);
    const auto histogram = ComputeIntegralHistogram<uint32>(store, 0, cancel);
    REQUIRE(histogram.valid());
    REQUIRE(histogram.value().counts.empty());
    REQUIRE(store.readCount() == 0);
  }
  SECTION("cancel after a successful read stops before fallback or frequency reduction")
  {
    CancelAfterReadDataStore<int32> store(3, 0, cancel);
    store.setValue(0, -32768);
    store.setValue(1, 0);
    store.setValue(2, 32768);
    const auto histogram = ComputeIntegralHistogram<int32>(store, 16, cancel);
    REQUIRE(histogram.valid());
    REQUIRE(cancel.load());
    REQUIRE(store.readCount() == 1);
  }
}

TEST_CASE("ImageProcessing::HistogramEngine: fused integral histogram propagates bulk-read errors", "[ImageProcessing][HistogramEngine]")
{
  constexpr int32 k_ErrorCode = -987654;
  FailingReadDataStore<int16> store(k_ErrorCode);
  std::atomic_bool cancel{false};
  const auto histogram = ComputeIntegralHistogram<int16>(store, 16, cancel);
  REQUIRE(histogram.invalid());
  REQUIRE(histogram.errors().size() == 1);
  REQUIRE(histogram.errors().front().code == k_ErrorCode);
}
