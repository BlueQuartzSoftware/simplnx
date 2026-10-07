#pragma once

#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <string_view>
#include <utility>

namespace nx::core::BoundedRead
{
/** @brief Funds fixed diagnostic text and one terminal result record. */
inline constexpr uint64 k_DiagnosticBytes = 4096;
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
  Failed       ///< The caller discards the destination, which may contain a prefix.
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
 * @brief Reports reviewed standard-library terminal and counted-container representations.
 * @return True only for source-reviewed compiler-library configurations.
 * @note Native support tests must fail on an unreviewed configuration. This is not a platform opt-out.
 * Apple's libc++ 180100 (Xcode MacOSX15.2.sdk) was reviewed for exact counted
 * default-allocator vector allocation, allocation-free empty controls and moves,
 * and bounded string reserve. Other libc++ 18 releases require their own review.
 * GCC 11.4 libstdc++ (__GLIBCXX__ == 20230528, CXX11 ABI) satisfies the same
 * bounds as GCC 14. The review covers bits/stl_vector.h, bits/vector.tcc,
 * bits/basic_string.h, bits/basic_string.tcc and expected-lite 0.8.0.
 */
inline constexpr bool DiagnosticRepresentationSupported() noexcept
{
#if !defined(expected_lite_MAJOR) || expected_lite_MAJOR != 0 || expected_lite_MINOR != 8 || expected_lite_PATCH != 0 || nsel_USES_STD_EXPECTED
  return false;
#elif defined(_MSVC_STL_VERSION) && defined(_MSVC_STL_UPDATE)
  return _MSVC_STL_VERSION == 143 && _MSVC_STL_UPDATE == 202503L;
#elif defined(_GLIBCXX_RELEASE)
  return (_GLIBCXX_RELEASE == 11 || _GLIBCXX_RELEASE == 14) && _GLIBCXX_USE_CXX11_ABI == 1;
#elif defined(_LIBCPP_VERSION)
  return _LIBCPP_VERSION == 180100 || (_LIBCPP_VERSION >= 190000 && _LIBCPP_VERSION < 200000);
#else
  return false;
#endif
}

/**
 * @brief Bounds the fixed empty-vector proxy in the reviewed configuration.
 * @return Two pointers for MSVC iterator debugging, otherwise zero.
 * @note GCC 11/14 libstdc++ and libc++ 180100/19.x have no heap iterator proxy.
 */
inline constexpr uint64 EmptyVectorProxyBytes() noexcept
{
#if defined(_MSVC_STL_VERSION) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
  return 2 * sizeof(void*);
#else
  return 0;
#endif
}

/**
 * @brief Bounds caller-funded mandatory empty Result return controls.
 * @return Two Result objects and warning-vector proxies for a named return and receiving object.
 * @pre DiagnosticRepresentationSupported() is true for an accepted bound.
 * @note Transparent prvalue forwarding adds no object in C++17. Nonempty diagnostics use their own D slice.
 */
inline constexpr uint64 EmptyResultControlBytes() noexcept
{
  return 2 * (sizeof(Result<bool>) + EmptyVectorProxyBytes());
}

/**
 * @brief Bounds typed empty-invalid return controls without constructing a value payload.
 * @tparam Value Specifies the checked API's inactive value representation.
 * @return Two Result controls, empty vector/expected/unexpected temporaries, and their fixed proxies.
 * @pre DiagnosticRepresentationSupported() is true for an accepted source bound.
 * @note The direct empty-error construction has no Error elements or message text.
 * Seven proxy slots cover two typed results, one standalone error vector, one
 * unexpected carrier and one expected construction carrier. C++17 prvalue forwarding
 * removes extra returned objects; the second Result slot covers a named caller move.
 */
template <typename Value>
inline constexpr uint64 EmptyInvalidResultControlBytes() noexcept
{
  using UnexpectedErrors = decltype(nonstd::make_unexpected(std::declval<ErrorCollection&&>()));
  return 2 * sizeof(Result<Value>) + sizeof(ErrorCollection) + sizeof(UnexpectedErrors) + sizeof(nonstd::expected<Value, ErrorCollection>) + 7 * EmptyVectorProxyBytes();
}

/**
 * @brief Supplies the complete pre-service bound for one terminal diagnostic.
 * @return Zero for an unreviewed representation; otherwise fixed controls, collection and text bytes.
 */
inline constexpr uint64 TerminalDiagnosticBoundBytes() noexcept;

/**
 * @brief Bounds a fresh counted default-allocator vector on reviewed STL releases.
 * @param count Supplies elements for counted construction or fresh reserve.
 * @param width Supplies bytes per non-owning, non-bool element.
 * @param bytes Receives payload and a conservative debug iterator-proxy allowance.
 * @return False for an unreviewed STL or overflow, without changing bytes.
 * @note Native CI must exercise this guard; a toolchain upgrade requires source review.
 * GCC 11/14 libstdc++ and libc++ 180100 counted construction and fresh reserve
 * request exactly count elements. They have no heap iterator proxy, so the
 * 64-byte allowance is conservative.
 */
inline bool FreshVectorBytes(uint64 count, uint64 width, uint64& bytes) noexcept
{
  uint64 value = 0;
  if(!DiagnosticRepresentationSupported() || width == 0 || !Multiply(count, width, value) || !Add(value, 64, value))
  {
    return false;
  }
  bytes = value;
  return true;
}

#if SIMPLNX_BUILD_TESTS
/**
 * @brief Reports a terminal allocation request or public retained capacities through the shared codec observer.
 * @param file Borrows an optional backend identity; null selects an unobserved resident diagnostic.
 * @param dataset Borrows the dataset identity.
 * @param requested Selects request bytes instead of retained capacities.
 * @param first Supplies requested bytes or retained record-vector bytes.
 * @param second Selects request kind (zero vector, one message), or retained message bytes.
 */
SIMPLNX_EXPORT void ObserveTerminalForTesting(const std::filesystem::path* file, std::string_view dataset, bool requested, uint64 first, uint64 second) noexcept;
#endif

/**
 * @struct Diagnostic
 * @brief Owns one bounded message until payload scratch has been destroyed.
 */
struct Diagnostic
{
  std::array<char, 512> text{};
  usize length = 0;
  int32 code = 0;
  bool truncated = false;
#if SIMPLNX_BUILD_TESTS
  const std::filesystem::path* observedFile = nullptr;
  std::string_view observedDataset;
#endif

  /**
   * @brief Appends text without allocating.
   * @param value Supplies borrowed diagnostic text.
   */
  void append(std::string_view value) noexcept
  {
    constexpr char hex[] = "0123456789ABCDEF";
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
      if(count > text.size() - 1 - length)
      {
        truncated = true;
        return;
      }
      std::memcpy(text.data() + length, bytes, count);
      length += count;
      text[length] = '\0';
      index += consumed;
    }
  }
  /**
   * @brief Appends a checked unsigned value without formatting allocation.
   * @param value Supplies the numeric context.
   */
  void number(uint64 value) noexcept
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
  Status set(int32 value, std::string_view message) noexcept
  {
    code = value;
    length = 0;
    text[0] = '\0';
    truncated = false;
    append(message);
    return code < 0 ? Status::Failed : Status::Unavailable;
  }
  /**
   * @brief Sets one numeric required/available diagnostic without allocation.
   * @param value Supplies the stable error or warning code.
   * @param message Supplies the fixed diagnostic prefix.
   * @param required Supplies the actual required count or bytes.
   * @param available Supplies the admitted or actual available count or bytes.
   * @return The status selected by value.
   */
  Status counts(int32 value, std::string_view message, uint64 required, uint64 available) noexcept
  {
    const auto status = set(value, message);
    append("; required=");
    number(required);
    append(", available=");
    number(available);
    return status;
  }

  /**
   * @brief Adds a bounded, valid UTF-8 path prefix with explicit native-unit escaping.
   * @param path Borrows the existing native representation.
   * @param dataset Borrows the dataset name; invalid UTF-8/control bytes use hexadecimal escapes.
   * @note Diagnostic encoding never selects the filename passed to the opening adapter.
   */
  void context(const std::filesystem::path& path, std::string_view dataset) noexcept
  {
#if SIMPLNX_BUILD_TESTS
    observedFile = &path;
    observedDataset = dataset;
#endif
    const auto& native = path.native();
    usize count = std::min<usize>(native.size(), 120);
#ifdef _WIN32
    append(" [native UTF-16 path; malformed units escaped: ");
    constexpr char hex[] = "0123456789ABCDEF";
    for(usize index = 0; index < count; ++index)
    {
      uint32 value = static_cast<uint32>(native[index]);
      if(value >= 0xD800 && value <= 0xDBFF && index + 1 < native.size())
      {
        const uint32 next = static_cast<uint32>(native[index + 1]);
        if(next >= 0xDC00 && next <= 0xDFFF)
        {
          if(index + 1 >= count)
          {
            count = index;
            break;
          }
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
    if(count < native.size())
    {
      while(count != 0 && (static_cast<unsigned char>(native[count]) & 0xC0U) == 0x80U)
      {
        --count;
      }
    }
    append({native.data(), count});
#endif
    if(count != native.size())
    {
      append("...");
      truncated = true;
    }
    append("; dataset (invalid/control bytes escaped): ");
    append(dataset);
    append("]");
  }
  /**
   * @brief Constructs the single active terminal collection after scratch destruction.
   * @param status Supplies the operation outcome.
   * @return Completion, reason-coded refusal, or one error without initializer-list message copies.
   * @throws std::bad_alloc If even the admitted terminal record cannot be allocated.
   */
  Result<bool> finish(Status status)
  {
    // The unsupported-build exception permits only mandatory empty return bookkeeping.
    // It never admits collection elements or diagnostic text before this proof.
    if(!DiagnosticRepresentationSupported() || TerminalDiagnosticBoundBytes() > k_DiagnosticBytes)
    {
      return {false};
    }
    if(status == Status::Complete)
    {
      return {true};
    }
    if(truncated)
    {
      constexpr std::string_view marker = " [truncated]";
      usize start = std::min(length, text.size() - 1 - marker.size());
      while(start != 0 && start < length && (static_cast<unsigned char>(text[start]) & 0xC0U) == 0x80U)
      {
        --start;
      }
      std::memcpy(text.data() + start, marker.data(), marker.size());
      length = start + marker.size();
      text[length] = '\0';
    }
    if(status == Status::Unavailable)
    {
      Result<bool> result{false};
#if SIMPLNX_BUILD_TESTS
      ObserveTerminalForTesting(observedFile, observedDataset, true, sizeof(Warning), 0);
#endif
      result.warnings().reserve(1);
#if SIMPLNX_BUILD_TESTS
      ObserveTerminalForTesting(observedFile, observedDataset, false, result.warnings().capacity() * sizeof(Warning), 0);
#endif
      result.warnings().emplace_back();
      auto& warning = result.warnings().back();
      warning.code = code;
#if SIMPLNX_BUILD_TESTS
      ObserveTerminalForTesting(observedFile, observedDataset, true, text.size() - 1, 1);
#endif
      warning.message.reserve(text.size() - 1);
#if SIMPLNX_BUILD_TESTS
      ObserveTerminalForTesting(observedFile, observedDataset, false, result.warnings().capacity() * sizeof(Warning), warning.message.capacity() + 1);
#endif
      warning.message.assign(text.data(), length);
      return result;
    }
    std::vector<Error> errors;
#if SIMPLNX_BUILD_TESTS
    ObserveTerminalForTesting(observedFile, observedDataset, true, sizeof(Error), 0);
#endif
    errors.reserve(1);
#if SIMPLNX_BUILD_TESTS
    ObserveTerminalForTesting(observedFile, observedDataset, false, errors.capacity() * sizeof(Error), 0);
#endif
    errors.emplace_back();
    errors.back().code = code;
#if SIMPLNX_BUILD_TESTS
    ObserveTerminalForTesting(observedFile, observedDataset, true, text.size() - 1, 1);
#endif
    errors.back().message.reserve(text.size() - 1);
#if SIMPLNX_BUILD_TESTS
    ObserveTerminalForTesting(observedFile, observedDataset, false, errors.capacity() * sizeof(Error), errors.back().message.capacity() + 1);
#endif
    errors.back().message.assign(text.data(), length);
    Result<bool> result{false};
    result.m_Expected = nonstd::make_unexpected(std::move(errors));
    return result;
  }
};

/**
 * @brief Calculates the reviewed terminal lifetime envelope before any reserve.
 * @return A proven upper bound no larger than D on a supported library, or zero when unreviewed.
 *
 * MSVC counted vector reserve is exact. Its string growth rounds the requested
 * capacity and can use geometric growth. The fresh 511-character request fits
 * 2*(511+1)+64 bytes, plus a separately bounded proxy. GCC 11/14 counted reserve is
 * exact and fresh string creation fits that same bound. GCC 11.4 has SSO capacity 15.
 * On growth, _M_create doubles capacity only when the request is below twice
 * the old capacity. Fresh reserve(511) requests exactly 512 bytes.
 * Subsequent assign of at most 511 characters does not grow it.
 * libc++ 180100 and 19.x round fresh string capacity to their allocation alignment
 * and also fit that bound. In Apple's libc++ 180100, reserve(511) requests 512 bytes
 * including the terminator; subsequent assign does not grow it.
 * These GNU and libc++ implementations
 * have no empty-vector/string heap proxy. Default-allocator moves/swap transfer
 * the existing payload without allocation.
 *
 * expected-lite 0.8 assignment builds an expected temporary, then swaps through
 * one local error vector. Together with the local ErrorCollection, unexpected
 * temporary and two Result representations, eight vector proxy slots suffice.
 * The payload moves between these controls. Named return optimization is not required.
 */
inline constexpr uint64 TerminalDiagnosticBoundBytes() noexcept
{
  if(!DiagnosticRepresentationSupported())
  {
    return 0;
  }
  using UnexpectedErrors = decltype(nonstd::make_unexpected(std::declval<ErrorCollection&&>()));
  constexpr uint64 resultControls = 2 * sizeof(Result<bool>);
  constexpr uint64 errorMoveControls = 2 * sizeof(ErrorCollection) + sizeof(UnexpectedErrors) + sizeof(nonstd::expected<bool, ErrorCollection>);
  constexpr uint64 proxies = 8 * EmptyVectorProxyBytes();
  // Numeric conversion, native/UTF-8 escaping and bounded string-view arguments.
  constexpr uint64 formattingControls =
      sizeof(std::to_chars_result) + sizeof(std::array<char, 24>) + 2 * sizeof(std::array<char, 4>) + sizeof(std::array<char, 6>) + 8 * sizeof(std::string_view) + 4 * sizeof(usize);
  constexpr uint64 recordPayload = std::max(sizeof(Error), sizeof(Warning)) + 64;
  constexpr uint64 messagePayloadAndProxy = 2 * 512 + 64 + 64;
  return sizeof(Diagnostic) + resultControls + errorMoveControls + proxies + formattingControls + recordPayload + messagePayloadAndProxy;
}
static_assert(!DiagnosticRepresentationSupported() || TerminalDiagnosticBoundBytes() <= k_DiagnosticBytes);

/**
 * @brief Measures the active terminal collection's public retained capacities.
 * @param result Borrows the terminal result through synchronous observation.
 * @return Record-vector payload plus message capacities and terminators.
 * @note This is an owned-capacity witness, not allocator bookkeeping or process RSS.
 */
inline uint64 ResultCapacityBytes(const Result<bool>& result) noexcept
{
  uint64 bytes = result.warnings().capacity() * sizeof(Warning);
  for(const auto& warning : result.warnings())
  {
    bytes += warning.message.capacity() + 1;
  }
  if(result.invalid())
  {
    bytes += result.errors().capacity() * sizeof(Error);
    for(const auto& error : result.errors())
    {
      bytes += error.message.capacity() + 1;
    }
  }
  return bytes;
}

/**
 * @brief Validates tuple selection arithmetic without allocating or calling a store reader.
 * @tparam Shape Specifies the existing shape container.
 * @param extent Supplies the selected tuple coordinates.
 * @param shape Supplies positive source dimensions.
 * @param components Supplies the checked component count.
 * @param destinationValues Supplies the exact destination element count.
 * @param selectedTuples Receives the selected tuple count.
 * @param diagnostic Receives a fixed validation failure.
 * @param elementBytes Supplies scalar width for addressable byte-count checks.
 * @return Complete when ranks, bounds, products and destination count agree.
 */
template <typename Shape>
Status ValidateExtent(const Extent& extent, const Shape& shape, uint64 components, uint64 destinationValues, uint64& selectedTuples, Diagnostic& diagnostic, uint64 elementBytes = 1) noexcept
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
