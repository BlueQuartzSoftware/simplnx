#pragma once

#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <array>
#include <charconv>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>

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
 * @struct Diagnostic
 * @brief Owns ordinary diagnostic text outside the data scratch grant.
 */
struct Diagnostic
{
  std::string text;
  int32 code = 0;

  /**
   * @brief Appends text with UTF-8 validation and control-byte escaping.
   * @param value Supplies borrowed diagnostic text.
   */
  void append(std::string_view value)
  {
    constexpr auto hex = std::to_array("0123456789ABCDEF");
    usize index = 0;
    while(index < value.size())
    {
      const auto first = static_cast<unsigned char>(value[index]);
      usize count = first < 0x80 ? 1 : first >= 0xC2 && first <= 0xDF ? 2 : first >= 0xE0 && first <= 0xEF ? 3 : first >= 0xF0 && first <= 0xF4 ? 4 : 0;
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
      const char* bytes = value.data() + index;
      usize consumed = count;
      if(!valid || first < 0x20 || first == 0x7F)
      {
        escaped = {'\\', 'x', hex[first >> 4], hex[first & 0x0F]};
        bytes = escaped.data();
        count = escaped.size();
        consumed = 1;
      }
      text.append(bytes, count);
      index += consumed;
    }
  }
  /**
   * @brief Appends a checked unsigned value to the diagnostic text.
   * @param value Supplies the numeric context.
   */
  void number(uint64 value)
  {
    std::array<char, 24> buffer{};
    const auto converted = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if(converted.ec == std::errc{})
    {
      append({buffer.data(), static_cast<usize>(converted.ptr - buffer.data())});
    }
  }
  /**
   * @brief Replaces diagnostic state with one terminal reason.
   * @param value Supplies the stable error or warning code.
   * @param message Supplies its fixed context prefix.
   * @return Failed for a negative code, otherwise Unavailable.
   */
  Status set(int32 value, std::string_view message)
  {
    code = value;
    text.clear();
    append(message);
    return code < 0 ? Status::Failed : Status::Unavailable;
  }
  /**
   * @brief Sets one numeric required/available diagnostic in the diagnostic text.
   * @param value Supplies the stable error or warning code.
   * @param message Supplies the fixed diagnostic prefix.
   * @param required Supplies the actual required count or bytes.
   * @param available Supplies the admitted or actual available count or bytes.
   * @return The status selected by value.
   */
  Status counts(int32 value, std::string_view message, uint64 required, uint64 available)
  {
    const auto status = set(value, message);
    append("; required=");
    number(required);
    append(", available=");
    number(available);
    return status;
  }

  /**
   * @brief Adds the full UTF-8 path and dataset context with native-unit escaping.
   * @param path Borrows the existing native representation.
   * @param dataset Borrows the dataset name; invalid UTF-8/control bytes use hexadecimal escapes.
   * @note Diagnostic encoding never selects the filename passed to the opening adapter.
   */
  void context(const std::filesystem::path& path, std::string_view dataset)
  {
    const auto& native = path.native();
    const usize count = native.size();
#ifdef _WIN32
    append(" [native UTF-16 path; malformed units escaped: ");
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
        append({escaped.data(), escaped.size()});
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
      append({encoded.data(), bytes});
    }
#else
    append(" [native path; invalid UTF-8/control bytes escaped: ");
    append({native.data(), count});
#endif
    append("; dataset (invalid/control bytes escaped): ");
    append(dataset);
    append("]");
  }
  /**
   * @brief Converts the operation status into the public result.
   * @param status Supplies completion, refusal, or failure.
   * @return True on completion, one warning on refusal, or one error on failure.
   * @throws std::bad_alloc If ordinary diagnostic storage cannot be allocated.
   */
  Result<bool> finish(Status status)
  {
    if(status == Status::Complete)
    {
      return {true};
    }
    if(status == Status::Unavailable)
    {
      Result<bool> result{false};
      result.warnings().push_back({.code = code, .message = text});
      return result;
    }
    return MakeErrorResult<bool>(code, text);
  }
};

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
Status ValidateExtent(const Extent& extent, const Shape& shape, uint64 components, uint64 destinationValues, uint64& selectedTuples, Diagnostic& diagnostic, uint64 elementBytes = 1)
{
  const usize rank = shape.size();
  if(rank > k_MaxRank)
  {
    return diagnostic.counts(Unsupported, "Bounded read rank exceeds the supported maximum.", rank, k_MaxRank);
  }
  if(extent.min.size() != rank || extent.max.size() != rank || extent.stride.size() != rank)
  {
    diagnostic.counts(InvalidExtent, "Bounded read extent minimum-vector rank does not match the source.", rank, extent.min.size());
    diagnostic.append(", maximum-vector rank=");
    diagnostic.number(extent.max.size());
    diagnostic.append(", stride-vector rank=");
    diagnostic.number(extent.stride.size());
    return Status::Failed;
  }
  uint64 sourceTuples = 1;
  selectedTuples = 1;
  for(usize dimension = 0; dimension < rank; ++dimension)
  {
    const uint64 bound = static_cast<uint64>(shape[dimension]);
    if(bound == 0 || extent.stride[dimension] == 0 || extent.min[dimension] > extent.max[dimension] || extent.max[dimension] >= bound)
    {
      diagnostic.set(InvalidExtent, "Bounded read has invalid bounds/stride at dimension ");
      diagnostic.number(dimension);
      diagnostic.append("; minimum=");
      diagnostic.number(extent.min[dimension]);
      diagnostic.append(", maximum=");
      diagnostic.number(extent.max[dimension]);
      diagnostic.append(", bound=");
      diagnostic.number(bound);
      diagnostic.append(", stride=");
      diagnostic.number(extent.stride[dimension]);
      return Status::Failed;
    }
    const uint64 count = (extent.max[dimension] - extent.min[dimension]) / extent.stride[dimension] + 1;
    if(!Multiply(sourceTuples, bound, sourceTuples) || !Multiply(selectedTuples, count, selectedTuples))
    {
      return diagnostic.set(InvalidExtent, "Bounded read shape or selected count overflows uint64.");
    }
  }
  uint64 sourceValues = 0;
  uint64 selectedValues = 0;
  if(elementBytes == 0 || !Multiply(sourceTuples, components, sourceValues) || sourceValues > std::numeric_limits<usize>::max() / elementBytes ||
     !Multiply(selectedTuples, components, selectedValues) || selectedValues > std::numeric_limits<usize>::max() / elementBytes || selectedValues != destinationValues)
  {
    diagnostic.set(InvalidExtent, "Bounded read value count is invalid; destination=");
    diagnostic.number(destinationValues);
    diagnostic.append(", components=");
    diagnostic.number(components);
    diagnostic.append(", tuples=");
    diagnostic.number(selectedTuples);
    return Status::Failed;
  }
  return Status::Complete;
}
} // namespace nx::core::BoundedRead
