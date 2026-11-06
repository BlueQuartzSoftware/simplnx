#pragma once

#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <array>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>

#include <fmt/format.h>

namespace nx::core::BoundedRead
{
/** @brief Limits each active decoder to 128 KiB of workspace payload. */
inline constexpr uint64 k_DefaultDecoderWorkspaceBytes = uint64{128} * 1024;
/** @brief Limits the explicitly supported rank without importing HDF5 into stores. */
inline constexpr usize k_MaxRank = 32;

/**
 * @enum Status
 * @brief Separates completion, optional refusal and a failed operation.
 */
enum class Status
{
  Complete,    ///< Every selected destination value is valid.
  Unavailable, ///< No payload or destination transfer took place.
  Failed       ///< A complete read failed without changing caller-visible output.
};

/**
 * @enum Code
 * @brief Supplies stable bounded-read errors and refusal reasons.
 */
enum Code : int32
{
  Insufficient = 6170,
  Unsupported = 6171,
  UnsupportedFilter = 6172,
  PendingWrites = 6173,
  UndefinedFill = 6174,
  InvalidExtent = -6170,
  MetadataFailure = -6171,
  AllocationFailure = -6172,
  ReadFailure = -6173,
  DecodeFailure = -6174,
  DecoderQuotaFailure = -6175,
  MetadataChanged = -6176
};

#if SIMPLNX_BUILD_TESTS
/**
 * @brief Supplies the actual return object's warnings member with an optional construction failure.
 * @param file Borrows a codec identity; null selects a resident read.
 * @param dataset Borrows the dataset identity.
 * @return An empty warnings collection, constructed directly in the result member.
 * @throws std::bad_alloc When the existing bounded fault observer requests a construction failure.
 */
SIMPLNX_EXPORT WarningCollection MakeSuccessWarningsForTesting(const std::filesystem::path* file = nullptr, std::string_view dataset = {});
#endif

/**
 * @brief Checks an unsigned product before storing it.
 * @param first Supplies the first factor.
 * @param second Supplies the second factor.
 * @param result Receives the product on success.
 * @return False without mutation when the product exceeds uint64.
 */
inline bool Multiply(uint64 first, uint64 second, uint64& result) noexcept
{
  if(second != 0 && first > std::numeric_limits<uint64>::max() / second)
  {
    return false;
  }
  result = first * second;
  return true;
}

/**
 * @brief Checks an unsigned sum before storing it.
 * @param first Supplies the first term.
 * @param second Supplies the second term.
 * @param result Receives the sum on success.
 * @return False without mutation when the sum exceeds uint64.
 */
inline bool Add(uint64 first, uint64 second, uint64& result) noexcept
{
  if(second > std::numeric_limits<uint64>::max() - first)
  {
    return false;
  }
  result = first + second;
  return true;
}

/**
 * @brief Escapes invalid UTF-8 and control bytes in diagnostic text.
 * @param value Supplies text to encode.
 * @return Valid UTF-8 with invalid and control bytes escaped.
 */
inline std::string EscapeDiagnosticText(std::string_view value)
{
  std::string text;
  constexpr auto hex = std::to_array("0123456789ABCDEF");
  usize index = 0;
  while(index < value.size())
  {
    const auto first = static_cast<unsigned char>(value[index]);
    usize count = 0;
    if(first < 0x80)
    {
      count = 1;
    }
    else if(first >= 0xC2 && first <= 0xDF)
    {
      count = 2;
    }
    else if(first >= 0xE0 && first <= 0xEF)
    {
      count = 3;
    }
    else if(first >= 0xF0 && first <= 0xF4)
    {
      count = 4;
    }
    bool valid = count != 0 && count <= value.size() - index;
    for(usize offset = 1; valid && offset < count; ++offset)
    {
      const auto next = static_cast<unsigned char>(value[index + offset]);
      valid = (next & 0xC0U) == 0x80U;
    }
    if(valid && count > 1)
    {
      const auto second = static_cast<unsigned char>(value[index + 1]);
      valid = !((first == 0xE0 && second < 0xA0) || (first == 0xED && second >= 0xA0) || (first == 0xF0 && second < 0x90) || (first == 0xF4 && second >= 0x90));
    }
    std::array<char, 4> escaped{};
    const char* bytesPtr = value.data() + index;
    usize consumed = count;
    if(!valid || first < 0x20 || first == 0x7F)
    {
      escaped = {'\\', 'x', hex[first >> 4], hex[first & 0x0F]};
      bytesPtr = escaped.data();
      count = escaped.size();
      consumed = 1;
    }
    text.append(bytesPtr, count);
    index += consumed;
  }
  return text;
}

/**
 * @brief Formats a native file path and dataset name for diagnostics.
 * @param path Supplies the original native path; this function never opens it.
 * @param dataset Supplies the dataset name.
 * @return The complete, escaped source context.
 */
inline std::string FormatSourceContext(const std::filesystem::path& path, std::string_view dataset)
{
  std::string text;
  const auto& native = path.native();
  const usize count = native.size();
#ifdef _WIN32
  text += EscapeDiagnosticText(" [native UTF-16 path; malformed units escaped: ");
  constexpr auto hex = std::to_array("0123456789ABCDEF");
  for(usize index = 0; index < count; ++index)
  {
    uint32 value = static_cast<uint32>(native[index]);
    if(value >= 0xD800 && value <= 0xDBFF && index + 1 < native.size())
    {
      const uint32 next = static_cast<uint32>(native[index + 1]);
      if(next >= 0xDC00 && next <= 0xDFFF)
      {
        value = 0x10000 + ((value - 0xD800) << 10) + (next - 0xDC00);
        ++index;
      }
    }
    if(value >= 0xD800 && value <= 0xDFFF)
    {
      const std::array<char, 6> escaped{'\\', 'u', hex[(value >> 12) & 15], hex[(value >> 8) & 15], hex[(value >> 4) & 15], hex[value & 15]};
      text += EscapeDiagnosticText({escaped.data(), escaped.size()});
      continue;
    }
    std::array<char, 4> encoded{};
    usize bytes = 0;
    if(value < 0x80)
    {
      encoded[bytes++] = static_cast<char>(value);
    }
    else if(value < 0x800)
    {
      encoded[bytes++] = static_cast<char>(0xC0U | (value >> 6));
      encoded[bytes++] = static_cast<char>(0x80U | (value & 0x3FU));
    }
    else if(value < 0x10000)
    {
      encoded[bytes++] = static_cast<char>(0xE0U | (value >> 12));
      encoded[bytes++] = static_cast<char>(0x80U | ((value >> 6) & 0x3FU));
      encoded[bytes++] = static_cast<char>(0x80U | (value & 0x3FU));
    }
    else
    {
      encoded[bytes++] = static_cast<char>(0xF0U | (value >> 18));
      encoded[bytes++] = static_cast<char>(0x80U | ((value >> 12) & 0x3FU));
      encoded[bytes++] = static_cast<char>(0x80U | ((value >> 6) & 0x3FU));
      encoded[bytes++] = static_cast<char>(0x80U | (value & 0x3FU));
    }
    text += EscapeDiagnosticText({encoded.data(), bytes});
  }
#else
  text += EscapeDiagnosticText(" [native path; invalid UTF-8/control bytes escaped: ");
  text += EscapeDiagnosticText({native.data(), count});
#endif
  text += EscapeDiagnosticText("; dataset (invalid/control bytes escaped): ");
  text += EscapeDiagnosticText(dataset);
  text += EscapeDiagnosticText("]");
  return text;
}

/**
 * @brief Validates tuple selection arithmetic before any destination transfer.
 * @tparam Shape Specifies the existing shape container.
 * @param extent Supplies the selected tuple coordinates.
 * @param shape Supplies positive source dimensions.
 * @param components Supplies the checked component count.
 * @param destinationValues Supplies the exact destination element count.
 * @param selectedTuples Receives the selected tuple count.
 * @param diagnostic Receives ordinary validation failure text.
 * @param elementBytes Supplies scalar width for addressable byte-count checks.
 * @return Complete when ranks, bounds, products and destination count agree.
 */
template <typename Shape>
Status ValidateExtent(const Extent& extent, const Shape& shape, uint64 components, uint64 destinationValues, uint64& selectedTuples, Result<>& diagnostic, uint64 elementBytes = 1)
{
  const usize rank = shape.size();
  if(rank > k_MaxRank)
  {
    diagnostic = MakeWarningVoidResult(Unsupported, fmt::format("Bounded read rank exceeds the supported maximum.; required={}, available={}", rank, k_MaxRank));
    return Status::Unavailable;
  }
  if(extent.min.size() != rank || extent.max.size() != rank || extent.stride.size() != rank)
  {
    diagnostic =
        MakeErrorResult<>(InvalidExtent, fmt::format("Bounded read extent minimum-vector rank does not match the source.; required={}, available={}, maximum-vector rank={}, stride-vector rank={}",
                                                     rank, extent.min.size(), extent.max.size(), extent.stride.size()));
    return Status::Failed;
  }
  uint64 sourceTuples = 1;
  selectedTuples = 1;
  for(usize dimension = 0; dimension < rank; ++dimension)
  {
    const uint64 bound = static_cast<uint64>(shape[dimension]);
    if(bound == 0 || extent.stride[dimension] == 0 || extent.min[dimension] > extent.max[dimension] || extent.max[dimension] >= bound)
    {
      diagnostic = MakeErrorResult<>(InvalidExtent, fmt::format("Bounded read has invalid bounds/stride at dimension {}; minimum={}, maximum={}, bound={}, stride={}", dimension, extent.min[dimension],
                                                                extent.max[dimension], bound, extent.stride[dimension]));
      return Status::Failed;
    }
    const uint64 count = (extent.max[dimension] - extent.min[dimension]) / extent.stride[dimension] + 1;
    if(!Multiply(sourceTuples, bound, sourceTuples) || !Multiply(selectedTuples, count, selectedTuples))
    {
      diagnostic = MakeErrorResult<>(InvalidExtent, fmt::format("Bounded read shape or selected count overflows uint64."));
      return Status::Failed;
    }
  }
  uint64 sourceValues = 0;
  uint64 selectedValues = 0;
  if(elementBytes == 0 || !Multiply(sourceTuples, components, sourceValues) || sourceValues > std::numeric_limits<usize>::max() / elementBytes ||
     !Multiply(selectedTuples, components, selectedValues) || selectedValues > std::numeric_limits<usize>::max() / elementBytes || selectedValues != destinationValues)
  {
    diagnostic = MakeErrorResult<>(InvalidExtent, fmt::format("Bounded read value count is invalid; destination={}, components={}, tuples={}", destinationValues, components, selectedTuples));
    return Status::Failed;
  }
  return Status::Complete;
}
} // namespace nx::core::BoundedRead
