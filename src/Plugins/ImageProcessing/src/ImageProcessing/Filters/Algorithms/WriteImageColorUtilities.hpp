#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/** @namespace nx::core::WriteImageColor
 * @brief Contains color safety operations used only by the image writer.
 */
namespace nx::core::WriteImageColor
{
/**
 * @brief Checks the products used by color slice, group and RGB indexes.
 * @tparam T Scalar input type.
 * @param width Output slice width.
 * @param height Output slice height.
 * @param slices Number of output slices.
 * @param hasMask Whether the source group also holds a mask.
 * @return An error for empty dimensions or unrepresentable buffer sizes.
 */
template <typename T>
Result<> ValidateSize(usize width, usize height, usize slices, bool hasMask)
{
  constexpr usize maximum = std::numeric_limits<usize>::max();
  const usize bytesPerValue = sizeof(T) + (hasMask ? std::max(sizeof(bool), sizeof(uint8)) : 0);
  if(width == 0 || height == 0 || slices == 0 || width > maximum / height)
  {
    return MakeErrorResult(-27030, "Write Image color slice dimensions are empty or too large. Reduce the image dimensions.");
  }
  const usize pixels = width * height;
  if(pixels > maximum / 3 || pixels > maximum / bytesPerValue || slices > maximum / (pixels * bytesPerValue))
  {
    return MakeErrorResult(-27032, "Write Image color buffer sizes exceed the supported index range. Reduce the image dimensions.");
  }
  return {};
}

/**
 * @struct Range
 * @brief Native endpoints and cancellation state from the writer's initial scan.
 * @tparam T Scalar input type.
 */
template <typename T>
struct Range
{
  T Minimum{};
  T Maximum{};
  bool Cancelled = false;
};

/**
 * @brief Scans scalar and optional mask pages before any slice is opened.
 * Finite masked values participate in the range. Masked nonfinite values do not
 * trigger the policy, and no nonfinite value participates in the range.
 * @tparam T Scalar input type.
 * @param array Single-component source array.
 * @param maskArray Optional single-component bool or uint8 mask.
 * @param policy Error (0) or Warn and Continue (1).
 * @param shouldCancel Stops the scan between pages.
 * @param reportRange Synchronous callback after each complete page.
 * @return Native range and warnings, cancellation state, or the first scan error.
 */
template <typename T>
Result<Range<T>> ScanRange(const IDataArray& array, const IDataArray* maskArray, uint64 policy, const std::atomic_bool& shouldCancel, const std::function<void(usize, usize)>& reportRange)
{
  const auto& store = array.getIDataStoreRefAs<AbstractDataStore<T>>();
  const usize count = store.getNumberOfTuples();
  if(count == 0)
  {
    return MakeErrorResult<Range<T>>(-27020, "Input array is empty.");
  }
  constexpr usize targetPageBytes = 1024 * 1024;
  const usize maskBytes = maskArray == nullptr ? 0 : (maskArray->getDataType() == DataType::boolean ? sizeof(bool) : sizeof(uint8));
  const usize pageCount = std::min(count, std::max<usize>(1, targetPageBytes / (sizeof(T) + maskBytes)));
  auto values = std::make_unique<T[]>(pageCount);
  std::unique_ptr<bool[]> boolMask;
  std::unique_ptr<uint8[]> byteMask;
  if(maskArray != nullptr)
  {
    if(maskArray->getDataType() == DataType::boolean)
    {
      boolMask = std::make_unique<bool[]>(pageCount);
    }
    else
    {
      byteMask = std::make_unique<uint8[]>(pageCount);
    }
  }
  Range<T> range;
  Result<> status;
  bool initialized = false;
  usize nonfiniteCount = 0;
  for(usize offset = 0; offset < count;)
  {
    if(shouldCancel)
    {
      range.Cancelled = true;
      return ConvertResultTo<Range<T>>(std::move(status), std::move(range));
    }
    const usize size = std::min(pageCount, count - offset);
    status = MergeResults(std::move(status), store.copyIntoBuffer(offset, nonstd::span<T>(values.get(), size)));
    if(status.invalid())
    {
      return ConvertInvalidResult<Range<T>>(std::move(status));
    }
    if(boolMask != nullptr)
    {
      status = MergeResults(std::move(status), maskArray->getIDataStoreRefAs<AbstractDataStore<bool>>().copyIntoBuffer(offset, nonstd::span<bool>(boolMask.get(), size)));
    }
    else if(byteMask != nullptr)
    {
      status = MergeResults(std::move(status), maskArray->getIDataStoreRefAs<AbstractDataStore<uint8>>().copyIntoBuffer(offset, nonstd::span<uint8>(byteMask.get(), size)));
    }
    if(status.invalid())
    {
      return ConvertInvalidResult<Range<T>>(std::move(status));
    }
    for(usize index = 0; index < size; ++index)
    {
      const T value = values[index];
      if constexpr(std::is_floating_point_v<T>)
      {
        if(!std::isfinite(value))
        {
          const bool selected = boolMask != nullptr ? boolMask[index] : (byteMask == nullptr || byteMask[index] != 0);
          if(selected)
          {
            if(policy == 0)
            {
              return ConvertResultTo<Range<T>>(
                  MergeResults(std::move(status), MakeErrorResult(-27027, fmt::format("Write Image array '{}' has an unmasked nonfinite value at tuple {}. Repair or mask this value, or select 'Warn "
                                                                                      "and Continue' in Nonfinite Input Handling. No image slice was written.",
                                                                                      array.getName(), offset + index))),
                  Range<T>{});
            }
            ++nonfiniteCount;
          }
          continue;
        }
      }
      if(!initialized)
      {
        range.Minimum = value;
        range.Maximum = value;
        initialized = true;
      }
      else
      {
        range.Minimum = std::min(range.Minimum, value);
        range.Maximum = std::max(range.Maximum, value);
      }
    }
    offset += size;
    if(reportRange)
    {
      reportRange(offset, count);
    }
  }
  if(shouldCancel)
  {
    range.Cancelled = true;
    return ConvertResultTo<Range<T>>(std::move(status), std::move(range));
  }
  if(nonfiniteCount != 0)
  {
    status.warnings().push_back({.code = -27031,
                                 .message = fmt::format("Write Image array '{}' contains {} unmasked nonfinite value(s). They use the preset's first control color, which is also used for the finite "
                                                        "minimum. The color range uses finite values only.",
                                                        array.getName(), nonfiniteCount)});
  }
  if(!initialized)
  {
    return ConvertResultTo<Range<T>>(MergeResults(std::move(status), MakeErrorResult(-27028, fmt::format("Write Image array '{}' has no finite values for its color range. Supply at least one finite "
                                                                                                         "value; masking nonfinite values does not create a finite range. No image slice was written.",
                                                                                                         array.getName()))),
                                     Range<T>{});
  }
  return ConvertResultTo<Range<T>>(std::move(status), std::move(range));
}

/**
 * @brief Normalizes native distances without signed overflow or endpoint collapse.
 * @tparam T Scalar input type.
 * @param value Input value; nonfinite values use the first control color.
 * @param minimum Finite native minimum.
 * @param maximum Finite native maximum.
 * @return Finite float32 coordinate in [0, 1].
 */
template <typename T>
float32 Normalize(T value, T minimum, T maximum)
{
  if constexpr(std::is_floating_point_v<T>)
  {
    if(!std::isfinite(value))
    {
      return 0.0F;
    }
  }
  if(minimum == maximum || value <= minimum)
  {
    return 0.0F;
  }
  if(value >= maximum)
  {
    return 1.0F;
  }
  float64 fraction = 0;
  if constexpr(std::is_same_v<T, bool>)
  {
    fraction = value ? 1.0 : 0.0;
  }
  else if constexpr(std::is_integral_v<T>)
  {
    // Unsigned subtraction is defined modulo 2^N. Since minimum <= value <=
    // maximum, both distances fit the corresponding unsigned native type.
    using UnsignedType = std::make_unsigned_t<T>;
    const auto numerator = static_cast<UnsignedType>(static_cast<UnsignedType>(value) - static_cast<UnsignedType>(minimum));
    const auto denominator = static_cast<UnsignedType>(static_cast<UnsignedType>(maximum) - static_cast<UnsignedType>(minimum));
    fraction = static_cast<float64>(numerator) / static_cast<float64>(denominator);
  }
  else
  {
    const float64 low = static_cast<float64>(minimum);
    const float64 high = static_cast<float64>(maximum);
    const float64 current = static_cast<float64>(value);
    if(low < 0 && high > 0 && high > std::numeric_limits<float64>::max() + low)
    {
      fraction = (current * 0.5 - low * 0.5) / (high * 0.5 - low * 0.5);
    }
    else
    {
      fraction = (current - low) / (high - low);
    }
  }
  return static_cast<float32>(std::clamp(fraction, 0.0, 1.0));
}

/**
 * @brief Validates the palette data the writer will index and interpolate.
 * @param points Packed position/RGB controls.
 * @param bins Normalized control positions.
 * @return An error for invalid sizes, nonfinite values or unordered positions.
 */
inline Result<> ValidatePalette(const std::vector<float32>& points, const std::vector<float32>& bins)
{
  if(points.size() < 8 || points.size() % 4 != 0 || bins.size() != points.size() / 4 || !std::all_of(points.begin(), points.end(), [](float32 value) { return std::isfinite(value); }) ||
     !std::all_of(bins.begin(), bins.end(), [](float32 value) { return std::isfinite(value); }) || !std::is_sorted(bins.begin(), bins.end()))
  {
    return MakeErrorResult(-27029, "Write Image requires at least two finite color controls in position order. Select a valid color preset.");
  }
  return {};
}

/**
 * @brief Uses the existing linear palette interpolation and truncating RGB bytes.
 * Checked endpoints and usize indexes avoid invalid segment/channel casts.
 * The caller validates the palette once before entering the pixel loop.
 * @param value Finite normalized coordinate in [0, 1].
 * @param bins Validated normalized control positions.
 * @param points Validated packed position/RGB controls.
 * @return Three uint8 channels, with the existing truncation rule.
 */
inline std::array<uint8, 3> ComputeRgb(float32 value, const std::vector<float32>& bins, const std::vector<float32>& points)
{
  const usize last = bins.size() - 1;
  usize left = 0;
  usize right = 0;
  float32 fraction = 0;
  if(value >= 1)
  {
    left = last;
    right = last;
  }
  else if(value > 0)
  {
    right = static_cast<usize>(std::lower_bound(bins.begin(), bins.end(), value) - bins.begin());
    right = std::min(last, std::max<usize>(1, right));
    left = right - 1;
    const float32 width = bins[right] - bins[left];
    if(width > 0)
    {
      fraction = std::clamp((value - bins[left]) / width, 0.0F, 1.0F);
    }
  }
  std::array<uint8, 3> rgb{};
  for(usize channel = 0; channel < rgb.size(); ++channel)
  {
    const float64 scaled = (points[left * 4 + channel + 1] * (1.0 - fraction) + points[right * 4 + channel + 1] * fraction) * 255;
    rgb.at(channel) = static_cast<uint8>(std::clamp(scaled, 0.0, 255.0));
  }
  return rgb;
}
} // namespace nx::core::WriteImageColor
