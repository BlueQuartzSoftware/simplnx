#pragma once

#include <array>
#include <cstring>
#include <functional>
#include <optional>
#include <string_view>

#include <fmt/core.h>

#include "complex/Common/Bit.hpp"
#include "complex/Common/Types.hpp"

namespace complex
{
namespace detail
{
[[nodiscard]] inline constexpr std::byte Hex2Byte(char ch) noexcept
{
  if(ch >= '0' && ch <= '9')
  {
    u8 value = ch - '0';
    return std::byte{value};
  }
  if(ch >= 'a' && ch <= 'f')
  {
    u8 value = 10 + ch - 'a';
    return std::byte{value};
  }
  if(ch >= 'A' && ch <= 'F')
  {
    u8 value = 10 + ch - 'A';
    return std::byte{value};
  }
  return std::byte{0};
}

[[nodiscard]] inline constexpr std::byte Hex2Byte(char first, char second) noexcept
{
  std::byte result = Hex2Byte(first) << 4;
  result |= Hex2Byte(second);
  return result;
}

[[nodiscard]] inline constexpr bool IsHex(char ch) noexcept
{
  return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

[[nodiscard]] inline constexpr usize String2Bytes(std::string_view string, usize offset, usize size, std::array<std::byte, 16>& uuid, usize uuidOffset) noexcept
{
  for(usize i = 0; i < size; i++)
  {
    usize firstIndex = (i * 2) + offset;
    usize secondIndex = firstIndex + 1;
    char firstChar = string[firstIndex];
    if(!IsHex(firstChar))
    {
      return 0;
    }
    char secondChar = string[secondIndex];
    if(!IsHex(secondChar))
    {
      return 0;
    }
    uuid[i + uuidOffset] = Hex2Byte(firstChar, secondChar);
  }

  return size * 2;
}
} // namespace detail

/**
 * @brief Uuid struct
 * Stored in big-endian order as according RFC 4122
 */
struct Uuid
{
  /**
   * @brief Parses a uuid string into a Uuid.
   * @param string Must be of the form "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}".
   * Must have both braces or none. Must have all dashes or none.
   * @return Parsed uuid if successful. Otherwise, empty optional.
   */
  [[nodiscard]] static inline constexpr std::optional<Uuid> FromString(std::string_view string)
  {
    if(string.empty())
    {
      return {};
    }

    bool hasFrontBrace = string.front() == '{';
    bool hasBackBrace = string.back() == '}';

    if((hasFrontBrace && !hasBackBrace) || (!hasFrontBrace && hasBackBrace))
    {
      return {};
    }

    bool hasBraces = hasFrontBrace && hasBackBrace;

    usize index = hasBraces ? 1 : 0;

    bool hasDashes = string[index + 8] == '-';

    usize totalSize = 32ull + (hasBraces ? 2 : 0) + (hasDashes ? 4 : 1);

    if(string.size() != totalSize)
    {
      return {};
    }

    if(hasDashes && (string[index + 8] != '-' || string[index + 13] != '-' || string[index + 18] != '-' || string[index + 23] != '-'))
    {
      return {};
    }

    Uuid uuid{};

    // time_low (4 bytes)
    usize result = detail::String2Bytes(string, index, 4, uuid.data, 0);
    if(result == 0)
    {
      return {};
    }
    index += result;
    index += string[index] == '-' ? 1 : 0;

    // time_mid (2 bytes)
    result = detail::String2Bytes(string, index, 2, uuid.data, 4);
    if(result == 0)
    {
      return {};
    }
    index += result;
    index += string[index] == '-' ? 1 : 0;

    // time_hi_and_version (2 bytes)
    result = detail::String2Bytes(string, index, 2, uuid.data, 6);
    if(result == 0)
    {
      return {};
    }
    index += result;
    index += string[index] == '-' ? 1 : 0;

    // clock_seq_hi_and_res_seq_low (2 bytes)
    result = detail::String2Bytes(string, index, 2, uuid.data, 8);
    if(result == 0)
    {
      return {};
    }
    index += result;
    index += string[index] == '-' ? 1 : 0;

    // node (6 bytes)
    result = detail::String2Bytes(string, index, 6, uuid.data, 10);
    if(result == 0)
    {
      return {};
    }

    return uuid;
  }

  [[nodiscard]] inline constexpr u32 time_low() const noexcept
  {
    return bit_cast_int<u32, endian::big>(data.data());
  }

  [[nodiscard]] inline constexpr u16 time_mid() const noexcept
  {
    return bit_cast_int<u16, endian::big>(data.data() + 4);
  }

  [[nodiscard]] inline constexpr u16 time_hi_version() const noexcept
  {
    return bit_cast_int<u16, endian::big>(data.data() + 6);
  }

  [[nodiscard]] inline constexpr u16 clock_seq_hi_and_res_clock_seq_low() const noexcept
  {
    return bit_cast_int<u16, endian::big>(data.data() + 8);
  }

  [[nodiscard]] inline constexpr u8 clock_seq_hi_variant() const noexcept
  {
    return bit_cast_int<u8, endian::big>(data.data() + 8);
  }

  [[nodiscard]] inline constexpr u8 clock_seq_low() const noexcept
  {
    return bit_cast_int<u8, endian::big>(data.data() + 9);
  }

  [[nodiscard]] inline constexpr u64 node() const noexcept
  {
    return bit_cast_int<u64, endian::big, 6>(data.data() + 10);
  }

  [[nodiscard]] inline std::string str() const
  {
    return fmt::format("{:x}-{:x}-{:x}-{:x}{:x}-{:x}", time_low(), time_mid(), time_hi_version(), clock_seq_hi_variant(), clock_seq_low(), node());
  }

  std::array<std::byte, 16> data;
};

inline constexpr bool operator==(const Uuid& lhs, const Uuid& rhs) noexcept
{
  for(usize i = 0; i < lhs.data.size(); i++)
  {
    if(lhs.data[i] != rhs.data[i])
    {
      return false;
    }
  }
  return true;
}

inline constexpr bool operator!=(const Uuid& lhs, const Uuid& rhs) noexcept
{
  return !(lhs == rhs);
}
} // namespace complex

namespace std
{
template <>
struct hash<complex::Uuid>
{
  std::size_t operator()(const complex::Uuid& value) const noexcept
  {
    std::uint64_t v1 = complex::bit_cast_ptr<std::uint64_t>(value.data.data());
    std::uint64_t v2 = complex::bit_cast_ptr<std::uint64_t>(value.data.data() + sizeof(v1));
    return v1 ^ v2;
  }
};
} // namespace std
