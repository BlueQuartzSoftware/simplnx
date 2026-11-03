#include "simplnx/Utilities/Math/StatisticsCalculations.hpp"

#include <catch2/catch.hpp>

#include <array>

using namespace nx::core;
using StatisticsCalculations::CalculateBin;
using StatisticsCalculations::HistogramBinPolicy;

TEMPLATE_TEST_CASE("simplnx::StatisticsCalculations: Classifier policies preserve types and values", "[simplnx][StatisticsCalculations]", bool, int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64, float32, float64)
{
  using ValueType = TestType;
  using RawResultType = std::conditional_t<std::is_same_v<ValueType, bool>, uint8, ValueType>;
  static_assert(std::is_same_v<decltype(CalculateBin(ValueType{1}, ValueType{0}, 1.0F)), RawResultType>);
  static_assert(std::is_same_v<decltype(CalculateBin<HistogramBinPolicy::LegacyInt32>(ValueType{1}, ValueType{0}, 1.0F)), int32>);
  // floor((1 - 0) / 1) = 1 for every supported type and both policies.
  CHECK(CalculateBin(ValueType{1}, ValueType{0}, 1.0F) == RawResultType{1});
  CHECK(CalculateBin<HistogramBinPolicy::LegacyInt32>(ValueType{1}, ValueType{0}, 1.0F) == int32{1});
}

TEST_CASE("simplnx::StatisticsCalculations: Preserve signed classifier conversion sequences", "[simplnx][StatisticsCalculations]")
{
  // int8 subtraction promotes to int: 100 - (-100) = 200, then 200 / 2 = 100.
  CHECK(CalculateBin(int8{100}, int8{-100}, 2.0F) == int8{100});
  // Legacy fallback converts the quotient directly to int32: 200/1 and 60000/1.
  CHECK(CalculateBin<HistogramBinPolicy::LegacyInt32>(int8{100}, int8{-100}, 1.0F) == int32{200});
  CHECK(CalculateBin<HistogramBinPolicy::LegacyInt32>(int16{30000}, int16{-30000}, 1.0F) == int32{60000});
  // Float32 subtraction gives 3500000000; floor(3500000000 / 1e9) = 3.
  CHECK(CalculateBin<HistogramBinPolicy::LegacyInt32>(int32{1500000000}, int32{-2000000000}, 1.0E9F) == int32{3});
}

TEMPLATE_TEST_CASE("simplnx::StatisticsCalculations: Modal lookup uses stored pairs", "[simplnx][StatisticsCalculations]", (std::integral_constant<HistogramBinPolicy, HistogramBinPolicy::Raw>),
                   (std::integral_constant<HistogramBinPolicy, HistogramBinPolicy::LegacyInt32>))
{
  constexpr auto policy = TestType::value;
  const std::array<int32, 4> ranges = {1, 3, 3, 6};
  const nonstd::span<const int32> rangeSpan(ranges.data(), ranges.size());
  // floor((mode-1)/2.5): modes 1,3 -> bin 0; mode 5 -> bin 1; modes 0,7 -> -1,2.
  CHECK(StatisticsCalculations::findModalBinRange<policy>(int32{1}, int32{1}, 2.5F, 2, rangeSpan) == std::make_pair(int32{1}, int32{3}));
  CHECK(StatisticsCalculations::findModalBinRange<policy>(int32{3}, int32{1}, 2.5F, 2, rangeSpan) == std::make_pair(int32{1}, int32{3}));
  CHECK(StatisticsCalculations::findModalBinRange<policy>(int32{5}, int32{1}, 2.5F, 2, rangeSpan) == std::make_pair(int32{3}, int32{6}));
  CHECK_FALSE(StatisticsCalculations::findModalBinRange<policy>(int32{0}, int32{1}, 2.5F, 2, rangeSpan).has_value());
  CHECK_FALSE(StatisticsCalculations::findModalBinRange<policy>(int32{7}, int32{1}, 2.5F, 2, rangeSpan).has_value());
  CHECK_FALSE(StatisticsCalculations::findModalBinRange<policy>(int32{1}, int32{1}, 0.0F, 2, rangeSpan).has_value());
}

TEST_CASE("simplnx::StatisticsCalculations: Floating modal lookup uses stored pairs", "[simplnx][StatisticsCalculations]")
{
  const std::array<float32, 4> ranges = {0, 0.5F, 0.5F, 1};
  const nonstd::span<const float32> rangeSpan(ranges.data(), ranges.size());
  // floor(0.25/0.5)=0 and floor(0.75/0.5)=1 select the two literal pairs.
  CHECK(StatisticsCalculations::findModalBinRange(0.25F, 0.0F, 0.5F, 2, rangeSpan) == std::make_pair(0.0F, 0.5F));
  CHECK(StatisticsCalculations::findModalBinRange(0.75F, 0.0F, 0.5F, 2, rangeSpan) == std::make_pair(0.5F, 1.0F));
  CHECK_FALSE(StatisticsCalculations::findModalBinRange(0.25F, 0.0F, 0.0F, 2, rangeSpan).has_value());
}

TEST_CASE("simplnx::StatisticsCalculations: Large modal offsets use integer arithmetic", "[simplnx][StatisticsCalculations]")
{
  // 2*8388609=16777218 and the upper offset is 16777219, below length 16777220.
  // Float32 2*bin+1 instead rounds to 16777220. This test-only buffer is about 64 MiB.
  std::vector<float32> ranges(16777220, 0.0F);
  ranges[16777218] = -1.0F;
  ranges[16777219] = -2.0F;
  CHECK(StatisticsCalculations::findModalBinRange(8388609.0F, 0.0F, 1.0F, 8388610, nonstd::span<const float32>(ranges.data(), ranges.size())) == std::make_pair(-1.0F, -2.0F));
}
