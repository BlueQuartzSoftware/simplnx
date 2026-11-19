#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageProcessing/ConnectedComponentEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/HistogramEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingOperationTypes.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/OtsuImageEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"
#include "simplnx/Utilities/ImageProcessing/TerminalImageConversion.hpp"
#include "simplnx/Utilities/ImageProcessing/ThresholdMaximumConnectedComponentsEngine.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace nx::core;
using nx::core::ImageProcessing::ConnectedComponentLabelLimitHandling;

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
TEMPLATE_TEST_CASE("Image terminal conversion: exact integer bounds", "[simplnx][ImageNumericContracts]", int8, uint8, int16, uint16, int32, uint32, int64, uint64)
{
  using ImageProcessing::detail::ConvertTerminalImageValue;
  using ImageProcessing::detail::TerminalImageConversionStatus;
  TestType output = 7;
  REQUIRE(ConvertTerminalImageValue(93.75, output) == TerminalImageConversionStatus::Valid);
  REQUIRE(output == TestType{93});
  REQUIRE(ConvertTerminalImageValue(-0.5, output) == TerminalImageConversionStatus::Valid);
  REQUIRE(output == TestType{0});
  const double upper = std::ldexp(1.0, std::numeric_limits<TestType>::digits);
  REQUIRE(ConvertTerminalImageValue(upper, output) == TerminalImageConversionStatus::Clipped);
  REQUIRE(output == std::numeric_limits<TestType>::max());
  if constexpr(std::numeric_limits<TestType>::digits <= 32)
  {
    REQUIRE(ConvertTerminalImageValue(static_cast<double>(std::numeric_limits<TestType>::max()) + 0.5, output) == TerminalImageConversionStatus::Valid);
    REQUIRE(output == std::numeric_limits<TestType>::max());
  }
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(ConvertTerminalImageValue(-upper, output) == TerminalImageConversionStatus::Valid);
    REQUIRE(output == std::numeric_limits<TestType>::lowest());
    REQUIRE(ConvertTerminalImageValue(std::nextafter(-upper - 1.0, -std::numeric_limits<double>::infinity()), output) == TerminalImageConversionStatus::Clipped);
    REQUIRE(output == std::numeric_limits<TestType>::lowest());
    REQUIRE(ConvertTerminalImageValue(-1.5, output) == TerminalImageConversionStatus::Valid);
    REQUIRE(output == TestType{-1});
  }
  else
  {
    REQUIRE(ConvertTerminalImageValue(-1.5, output) == TerminalImageConversionStatus::Clipped);
    REQUIRE(output == TestType{0});
  }
  for(const double value : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()})
  {
    output = 7;
    REQUIRE(ConvertTerminalImageValue(value, output) == TerminalImageConversionStatus::NonFinite);
    REQUIRE(output == TestType{7});
  }
}
// NOLINTEND(bugprone-chained-comparison)

TEST_CASE("Image terminal conversion: IEEE floating results", "[simplnx][ImageNumericContracts]")
{
  using ImageProcessing::detail::NarrowFloatingImageValue;
  const auto positive = NarrowFloatingImageValue<float32>(std::numeric_limits<float64>::max());
  const auto negative = NarrowFloatingImageValue<float32>(-std::numeric_limits<float64>::max());
  REQUIRE(std::isinf(positive));
  REQUIRE_FALSE(std::signbit(positive));
  REQUIRE(std::isinf(negative));
  REQUIRE(std::signbit(negative));
  REQUIRE(std::isnan(NarrowFloatingImageValue<float32>(std::numeric_limits<float64>::quiet_NaN())));
  REQUIRE(std::signbit(NarrowFloatingImageValue<float32>(-0.0)));
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(NarrowFloatingImageValue<float64>(std::numeric_limits<float32>::max()) == static_cast<float64>(std::numeric_limits<float32>::max()));
}

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros.
TEST_CASE("Connected component engine: native provisional label capacity", "[simplnx][ImageNumericContracts]")
{
  if constexpr(std::numeric_limits<usize>::max() <= std::numeric_limits<uint32>::max())
  {
    SUCCEED("A native index on this platform cannot exceed uint32 capacity; no overflowing candidate was executed.");
    return;
  }
  const std::array<uint64, 6> candidates{1, 4294967295ULL, 4294967296ULL, 4294967297ULL, 8589934592ULL, 8589934593ULL};
  const std::array<uint32, 6> legacyLabels{1, 4294967295U, 0, 1, 0, 1};
  usize warningMessages = 0;
  const IFilter::MessageHandler handler{[&warningMessages](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Warning)
    {
      ++warningMessages;
    }
  }};
  ImageProcessing::detail::ConnectedComponentLabelState strictState(ConnectedComponentLabelLimitHandling::Error, handler);
  ImageProcessing::detail::ConnectedComponentLabelState legacyState(ConnectedComponentLabelLimitHandling::WarnAndContinueLegacy, handler);
  for(usize index = 0; index < candidates.size(); ++index)
  {
    const usize candidate = static_cast<usize>(candidates.at(index));
    CAPTURE(candidate);
    const auto strictResult = strictState.labelFor(candidate);
    if(index < 2)
    {
      SIMPLNX_RESULT_REQUIRE_VALID(strictResult);
      REQUIRE(strictResult.value() == legacyLabels.at(index));
    }
    else
    {
      SIMPLNX_RESULT_REQUIRE_INVALID(strictResult);
      REQUIRE(strictResult.errors().front().code == -29301);
      REQUIRE(strictResult.errors().front().message.find(std::to_string(candidate)) != std::string::npos);
      REQUIRE(strictResult.errors().front().message.find("Warn and Continue (Legacy Behavior)") != std::string::npos);
    }
    const auto legacyResult = legacyState.labelFor(candidate);
    SIMPLNX_RESULT_REQUIRE_VALID(legacyResult);
    REQUIRE(legacyResult.value() == legacyLabels.at(index));
    REQUIRE(warningMessages == (index < 2 ? 0 : 1)); // immediate, before finish(), and only once
  }
  REQUIRE(warningMessages == 1);
  const auto strictFinished = strictState.finish({});
  SIMPLNX_RESULT_REQUIRE_VALID(strictFinished);
  REQUIRE(strictFinished.warnings().empty());
  const auto legacyFinished = legacyState.finish({});
  SIMPLNX_RESULT_REQUIRE_VALID(legacyFinished);
  REQUIRE(legacyFinished.warnings().size() == 1);
  REQUIRE(legacyFinished.warnings().front().code == -29302);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

TEST_CASE("Image terminal conversion: last finite Float32 rounding interval", "[simplnx][ImageNumericContracts]")
{
  const auto sign = GENERATE(-1.0, 1.0);
  const float32 maximum = std::numeric_limits<float32>::max();
  constexpr float64 midpoint = 0x1.ffffffp127;
  for(const float64 magnitude : {static_cast<float64>(maximum), std::nextafter(static_cast<float64>(maximum), midpoint), std::nextafter(midpoint, 0.0)})
  {
    // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
    REQUIRE(ImageProcessing::detail::NarrowFloatingImageValue<float32>(sign * magnitude) == static_cast<float32>(sign) * maximum);
  }
  for(const float64 magnitude : {midpoint, std::nextafter(midpoint, std::numeric_limits<float64>::infinity())})
  {
    const float32 result = ImageProcessing::detail::NarrowFloatingImageValue<float32>(sign * magnitude);
    REQUIRE(std::isinf(result));
    // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
    REQUIRE(std::signbit(result) == (sign < 0));
  }
}

TEST_CASE("Otsu histogram: inconsistent bounds cannot index a bin", "[simplnx][ImageNumericContracts]")
{
  ImageProcessing::Histogram histogram{.counts = {0, 0}, .min = 2, .max = 4, .binWidth = 1};
  const std::array<float64, 1> values{1.0};
  const std::atomic_bool shouldCancel{false};
  const auto result = ImageProcessing::detail::AccumulateOtsuBatch<float64, uint8>(histogram, 0.0, 4.0, 1.0, values, {}, shouldCancel);
  // NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == ImageProcessing::k_OtsuInvalidBinCoordinate);
  REQUIRE(histogram.counts == std::vector<usize>{0, 0});
  // NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)
}

TEST_CASE("Otsu histogram: a positive bin width must have a finite reciprocal", "[simplnx][ImageNumericContracts]")
{
  ImageProcessing::Histogram histogram{.counts = std::vector<usize>(8), .min = 0.0, .max = std::numeric_limits<float64>::min()};
  const std::atomic_bool shouldCancel{false};
  const auto result = ImageProcessing::detail::ConfigureOtsuBinsAndGetInverseWidth(histogram, false, shouldCancel);
  // NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == ImageProcessing::k_OtsuInvalidBinWidth);
  REQUIRE(histogram.binWidth > 0.0);
  REQUIRE(histogram.binWidth < 1.0 / std::numeric_limits<float64>::max());
  // NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)
}

TEST_CASE("Otsu histogram: bin count fits the platform container", "[simplnx][ImageNumericContracts]")
{
  const usize capacity = std::vector<usize>{}.max_size();
  if(capacity >= std::numeric_limits<uint32>::max())
  {
    SUCCEED("Every representable bin-count parameter fits the vector size limit on this platform.");
    return;
  }
  const DataStore<float64> input(ShapeType{1}, ShapeType{1}, 2.0);
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = static_cast<uint32>(capacity + 1)};
  const std::atomic_bool cancelled = false;
  const auto result = ImageProcessing::ComputeOtsuHistogram(input, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, cancelled);
  // NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == ImageProcessing::k_OtsuBinAllocationTooLarge);
  // NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)
}

TEST_CASE("Otsu masked labeling: two thresholds give the same classes across the serial cutoff", "[simplnx][ImageNumericContracts]")
{
  const usize batchPixels = GENERATE(511U, 512U, 513U);
  CAPTURE(batchPixels);
  // One full batch and a tail one pixel shorter exercise the cutoff within the same image.
  const usize totalPixels = batchPixels + (batchPixels - 1);
  DataStore<float32> input({totalPixels}, {1}, 0.0F);
  DataStore<uint8> mask({totalPixels}, {1}, uint8{0});
  DataStore<uint8> output({totalPixels}, {1}, uint8{254});
  std::vector<uint8> expected(totalPixels);
  for(usize i = 0; i < totalPixels; ++i)
  {
    const usize classIndex = i % 3;
    const bool selected = (i % 5) != 0;
    input.setValue(i, static_cast<float32>(classIndex * 32));
    mask.setValue(i, static_cast<uint8>(selected));
    // Three occupied bins and two thresholds assign one distinct label to each selected intensity.
    expected[i] = selected ? static_cast<uint8>(7 + classIndex) : uint8{255};
  }
  constexpr usize bytesPerLabelPixel = sizeof(float32) + 2 * sizeof(uint8);
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 3, .NumberOfThresholds = 2, .LabelOffset = 7, .TargetBytes = batchPixels * bytesPerLabelPixel};
  const std::atomic_bool shouldCancel{false};
  const auto result = ImageProcessing::ApplyOtsuImage<float32, uint8>(input, output, &mask, options, shouldCancel, {});
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) simplnx result assertion macro.
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto labels = output.createSpan();
  REQUIRE(std::equal(labels.begin(), labels.end(), expected.begin(), expected.end()));
}

TEST_CASE("Image reconstruction: preserve native integer marker offsets", "[simplnx][ImageNumericContracts]")
{
  constexpr int64 intensity = int64{1} << 54;
  // NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(ImageProcessing::detail::SubtractExactIntegerReconstructionHeight(intensity, 1) == intensity - 1);
  REQUIRE(ImageProcessing::detail::AddExactIntegerReconstructionHeight(intensity, 1) == intensity + 1);
  REQUIRE(ImageProcessing::detail::SubtractExactIntegerReconstructionHeight(std::numeric_limits<int64>::lowest(), 1) == std::numeric_limits<int64>::lowest());
  REQUIRE(ImageProcessing::detail::AddExactIntegerReconstructionHeight(std::numeric_limits<uint64>::max(), 1) == std::numeric_limits<uint64>::max());
  // NOLINTEND(bugprone-chained-comparison)
}

TEST_CASE("Threshold maximum components: distinguish unbounded and empty cutoffs", "[simplnx][ImageNumericContracts]")
{
  const auto high = ImageProcessing::detail::ConvertThresholdMaxCCCap<uint64>(std::numeric_limits<float64>::infinity());
  REQUIRE_FALSE(high.Empty);
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(high.Value == std::numeric_limits<uint64>::max());
  REQUIRE(ImageProcessing::detail::ConvertThresholdMaxCCCap<int64>(-std::numeric_limits<float64>::infinity()).Empty);
}

TEST_CASE("Image finite range: empty batches preserve exact contributing bounds", "[simplnx][ImageNumericContracts]")
{
  const ImageProcessing::ArrayMinMax<float64> noSamples{};
  ImageProcessing::ArrayMinMax<float64> finite;
  finite.addValue(4.0);
  finite.addValue(9.0);
  const bool emptyFirst = GENERATE(false, true);
  auto range = emptyFirst ? noSamples : finite;
  range.merge(emptyFirst ? finite : noSamples);
  // NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(range.count == 2);
  REQUIRE(range.min == 4.0);
  REQUIRE(range.max == 9.0);
  // NOLINTEND(bugprone-chained-comparison)

  constexpr uint64 base = uint64{1} << 54;
  ImageProcessing::ArrayMinMax<uint64> wide;
  wide.addValue(base + 1);
  wide.addValue(base + 2);
  // NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(wide.min == base + 1);
  REQUIRE(wide.max == base + 2);
  // NOLINTEND(bugprone-chained-comparison)
}

TEST_CASE("Image terminal conversion: a single rejection reports its image index", "[simplnx][ImageNumericContracts]")
{
  const bool tile = GENERATE(false, true);
  const std::array<float64, 6> input{1, 2, 256, std::numeric_limits<float64>::quiet_NaN(), 5, 6};
  std::array<uint8, 6> output{};
  const std::atomic_bool cancelled = false;
  ImageProcessing::detail::TerminalConversionState state(input.size());
  REQUIRE_FALSE((ImageProcessing::detail::ConvertTerminalImageBuffer<uint8, float64>(input, output, 13, state, cancelled, tile ? 2 : 0, tile ? 8 : 0)));
  // NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(state.nonFiniteCount() == 1);
  REQUIRE(state.nonFiniteIndex() == (tile ? 22 : 16));
  REQUIRE(state.clippedCount() == 1);
  // NOLINTEND(bugprone-chained-comparison)
  REQUIRE_FALSE(state.partial());
}

TEST_CASE("Image terminal conversion: a rejected resident block preserves every destination value", "[simplnx][ImageNumericContracts]")
{
  const std::array<float32, 3> rejected{1, std::numeric_limits<float32>::infinity(), 3};
  const std::array<float32, 2> good{4, 5};
  std::array<int32, 5> output{71, 72, 73, 74, 75};
  const std::atomic_bool cancelled = false;
  ImageProcessing::detail::TerminalConversionState state(output.size());
  auto destination = nonstd::span<int32>(output);
  ImageProcessing::detail::ConvertResidentTerminalImageBuffer<int32, float32>(rejected, destination.first(3), 0, state, cancelled);
  ImageProcessing::detail::ConvertResidentTerminalImageBuffer<int32, float32>(good, destination.subspan(3), 3, state, cancelled);
  // NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(output == std::array<int32, 5>{71, 72, 73, 4, 5});
  REQUIRE(state.nonFiniteCount() == 1);
  REQUIRE(state.nonFiniteIndex() == 1);
  // NOLINTEND(bugprone-chained-comparison)
  REQUIRE_FALSE(state.partial());
}
