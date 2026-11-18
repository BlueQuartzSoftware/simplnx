#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace nx::core::ImageProcessing::detail
{
/**
 * @enum TerminalImageConversionStatus
 * @brief Reports the final conversion without allocating diagnostics inside a pixel loop.
 */
enum class TerminalImageConversionStatus : std::uint8_t
{
  Valid,    ///< A converted result is available without integer clipping.
  Clipped,  ///< A finite integer result was clipped after truncation.
  NonFinite ///< A nonfinite integer result was rejected; output is unchanged.
};

/**
 * @class TerminalConversionState
 * @brief Counts results from final output buffers. Records the flat index for a single rejection.
 * @details Workers can record outcomes concurrently. Read the state after the workers join.
 */
class TerminalConversionState
{
public:
  explicit TerminalConversionState(usize totalValues)
  : m_TotalValues(totalValues)
  {
  }
  ~TerminalConversionState() = default;
  TerminalConversionState(const TerminalConversionState&) = delete;
  TerminalConversionState(TerminalConversionState&&) = delete;
  TerminalConversionState& operator=(const TerminalConversionState&) = delete;
  TerminalConversionState& operator=(TerminalConversionState&&) = delete;

  struct BufferCounts
  {
    usize Inspected = 0;
    usize Clipped = 0;
    usize NonFinite = 0;
    usize NonFiniteIndex = 0;
  };

  /**
   * @brief Merges one worker's counts after it finishes inspecting a buffer.
   * @param counts Counts and single-rejection location collected by the worker.
   */
  void recordBuffer(const BufferCounts& counts) noexcept
  {
    m_Inspected.fetch_add(counts.Inspected, std::memory_order_relaxed);
    if(counts.Clipped != 0)
    {
      m_Clipped.fetch_add(counts.Clipped, std::memory_order_relaxed);
    }
    if(counts.NonFinite != 0)
    {
      m_NonFinite.fetch_add(counts.NonFinite, std::memory_order_relaxed);
      m_NonFiniteIndex.store(counts.NonFiniteIndex, std::memory_order_relaxed);
    }
  }

  usize clippedCount() const noexcept
  {
    return m_Clipped.load(std::memory_order_relaxed);
  }
  usize nonFiniteCount() const noexcept
  {
    return m_NonFinite.load(std::memory_order_relaxed);
  }
  /**
   * @brief Returns the rejected index when nonFiniteCount() is one and all workers have joined.
   * @return The flat output index of the single rejected value.
   */
  usize nonFiniteIndex() const noexcept
  {
    return m_NonFiniteIndex.load(std::memory_order_relaxed);
  }
  bool partial() const noexcept
  {
    return m_Inspected.load(std::memory_order_relaxed) < m_TotalValues;
  }

private:
  usize m_TotalValues;
  std::atomic<usize> m_Inspected{0};
  std::atomic<usize> m_Clipped{0};
  std::atomic<usize> m_NonFinite{0};
  std::atomic<usize> m_NonFiniteIndex{0};
};

/**
 * @brief Adds caller-owned diagnostics to the result at the end of terminal conversion.
 */
using TerminalConversionReporter = Result<>(const TerminalConversionState&, Result<>);

/**
 * @brief Converts floating image values and maps finite narrowing overflow to signed infinity.
 * @tparam OutputType float or double destination type.
 * @tparam InputType float or double source type.
 * @param value Floating value to convert.
 * @return The converted value, retaining NaN, infinity, and signed zero behavior.
 *
 * Preserve direct integer-to-float32 conversions at callers. Passing those integers through float64 first can introduce double rounding.
 */
template <class OutputType, class InputType>
OutputType NarrowFloatingImageValue(InputType value)
{
  static_assert(std::is_floating_point_v<OutputType> && std::is_floating_point_v<InputType>);
  static_assert(std::is_same_v<InputType, float> || std::is_same_v<InputType, double>);
  static_assert(std::is_same_v<OutputType, float> || std::is_same_v<OutputType, double>);
  if constexpr(std::is_same_v<InputType, float64> && std::is_same_v<OutputType, float32>)
  {
    static_assert(std::numeric_limits<float32>::is_iec559 && std::numeric_limits<float32>::digits == 24 && std::numeric_limits<float32>::max_exponent == 128);
    // The midpoint between FLT_MAX and 2^128 rounds to infinity (ties to even).
    // Values between FLT_MAX and the midpoint round back to FLT_MAX, without an out-of-range cast.
    constexpr float64 overflowMidpoint = 0x1.ffffffp127;
    const float64 magnitude = std::abs(value);
    if(magnitude > static_cast<float64>(std::numeric_limits<float32>::max()))
    {
      const float32 rounded = magnitude < overflowMidpoint ? std::numeric_limits<float32>::max() : std::numeric_limits<float32>::infinity();
      return std::signbit(value) ? -rounded : rounded;
    }
  }
  return static_cast<OutputType>(value);
}

/**
 * @brief Converts one final image value without changing intermediate image precision.
 * @tparam OutputType Integer, float, or double output type.
 * @tparam InputType float or double working-value type.
 * @param value Final working value to convert.
 * @param output Receives the converted value. Remains unchanged when NonFinite is returned.
 * @return Valid for a floating conversion or an integer conversion within range. Clipped for a finite integer result outside the range after truncation.
 * NonFinite only for a nonfinite value with an integer output type.
 *
 * A Valid floating result can be NaN or infinity. Finite integer values truncate toward zero before clipping.
 * The exact binary exclusive upper bound avoids rounding a native 64-bit maximum into an unsafe cast.
 * This helper does not aggregate warnings.
 */
template <class OutputType, class InputType>
TerminalImageConversionStatus ConvertTerminalImageValue(InputType value, OutputType& output)
{
  static_assert(std::is_floating_point_v<InputType>);
  static_assert(std::is_same_v<InputType, float> || std::is_same_v<InputType, double>);
  if constexpr(std::is_integral_v<OutputType>)
  {
    if(!std::isfinite(value))
    {
      return TerminalImageConversionStatus::NonFinite;
    }
    const double truncated = std::trunc(static_cast<double>(value));
    const double upperExclusive = std::ldexp(1.0, std::numeric_limits<OutputType>::digits);
    const double lowerInclusive = std::is_signed_v<OutputType> ? -upperExclusive : 0.0;
    if(truncated < lowerInclusive)
    {
      output = std::numeric_limits<OutputType>::lowest();
      return TerminalImageConversionStatus::Clipped;
    }
    if(truncated >= upperExclusive)
    {
      output = std::numeric_limits<OutputType>::max();
      return TerminalImageConversionStatus::Clipped;
    }
    output = static_cast<OutputType>(truncated);
    return TerminalImageConversionStatus::Valid;
  }
  else
  {
    output = NarrowFloatingImageValue<OutputType>(value);
    return TerminalImageConversionStatus::Valid;
  }
}

/**
 * @brief Converts a working buffer and counts every inspected final value, including values after a rejection.
 * @tparam T Destination pixel type.
 * @tparam Real Working pixel type of the computed values.
 * @tparam WriteValues False to count conversions in a rejected block without publishing values.
 * @param input Final working values to inspect.
 * @param output Receives converted values when WriteValues is true. May be empty for counting alone.
 * @param offset Flat image index corresponding to the first value.
 * @param state Accumulates counts from this and other buffers.
 * @param shouldCancel Requests early termination of the conversion.
 * @param valuesPerRow Width of a tile row, or zero for a contiguous span.
 * @param rowStride Distance between successive row starts in the image.
 * @return True when the entire buffer can be written to the destination.
 * @details For an X tile, valuesPerRow is the tile width and rowStride is the image width.
 */
template <class T, class Real, bool WriteValues = true>
bool ConvertTerminalImageBuffer(nonstd::span<const Real> input, nonstd::span<T> output, usize offset, TerminalConversionState& state, const std::atomic_bool& shouldCancel, usize valuesPerRow = 0,
                                usize rowStride = 0)
{
  usize clipped = 0;
  usize nonFinite = 0;
  usize rejectedIndex = 0;
  usize index = 0;
  for(; index < input.size(); ++index)
  {
    if((index & 4095ULL) == 0 && shouldCancel)
    {
      break;
    }
    T value{};
    const auto status = ConvertTerminalImageValue(input[index], value);
    if constexpr(WriteValues)
    {
      if(status != TerminalImageConversionStatus::NonFinite)
      {
        output[index] = value;
      }
    }
    if(status == TerminalImageConversionStatus::Clipped)
    {
      ++clipped;
    }
    else if(status == TerminalImageConversionStatus::NonFinite)
    {
      if(nonFinite == 0)
      {
        rejectedIndex = valuesPerRow == 0 ? offset + index : offset + ((index / valuesPerRow) * rowStride) + (index % valuesPerRow);
      }
      ++nonFinite;
    }
  }
  state.recordBuffer({.Inspected = index, .Clipped = clipped, .NonFinite = nonFinite, .NonFiniteIndex = rejectedIndex});
  return index == input.size() && nonFinite == 0;
}

/**
 * @brief Publishes valid integer blocks directly into the resident output span.
 * @tparam T Destination pixel type.
 * @tparam Real Working pixel type.
 * @param input Final working values in image order.
 * @param output Destination values. A rejected integer block leaves its destination untouched.
 * @param offset Flat image index of the first value.
 * @param state Accumulates conversion counts.
 * @param shouldCancel Requests early termination between blocks.
 */
template <class T, class Real>
void ConvertResidentTerminalImageBuffer(nonstd::span<const Real> input, nonstd::span<T> output, usize offset, TerminalConversionState& state, const std::atomic_bool& shouldCancel)
{
  if constexpr(std::is_integral_v<T>)
  {
    constexpr usize blockSize = 512;
    for(usize start = 0; start < input.size() && !shouldCancel;)
    {
      const usize length = std::min(blockSize, input.size() - start);
      const auto block = input.subspan(start, length);
      // Check before touching the destination. Rejected blocks still contribute every final conversion to the counts.
      if(std::all_of(block.begin(), block.end(), [](Real value) { return std::isfinite(value); }))
      {
        ConvertTerminalImageBuffer<T, Real>(block, output.subspan(start, length), offset + start, state, shouldCancel);
      }
      else
      {
        ConvertTerminalImageBuffer<T, Real, false>(block, {}, offset + start, state, shouldCancel);
      }
      start += length;
    }
  }
  else
  {
    // Floating output has no rejected conversions. Check cancellation between whole blocks.
    for(usize start = 0; start < input.size() && !shouldCancel;)
    {
      const usize length = std::min<usize>(4096, input.size() - start);
      ConvertTerminalImageBuffer(input.subspan(start, length), output.subspan(start, length), offset + start, state, shouldCancel);
      start += length;
    }
  }
}
} // namespace nx::core::ImageProcessing::detail
