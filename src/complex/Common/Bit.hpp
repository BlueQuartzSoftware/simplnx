#pragma once

#include <cstring>
#include <type_traits>

#include "complex/Common/Types.hpp"

#ifdef _WIN32
#define COMPLEX_BYTE_ORDER little
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define COMPLEX_BYTE_ORDER little
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define COMPLEX_BYTE_ORDER big
#endif
#endif

namespace complex
{
enum class endian : uint8
{
  little = 0,
  big = 1,
  native = COMPLEX_BYTE_ORDER
};

template <class T>
[[nodiscard]] inline constexpr T byteswap(T value) noexcept
{
  static_assert(std::is_integral_v<T>);
  if constexpr(sizeof(T) == sizeof(uint8))
  {
    return value;
  }
  else if constexpr(sizeof(T) == sizeof(uint16))
  {
    return ((value >> 8) & 0xFFu) | ((value & 0xFFu) << 8);
  }
  else if constexpr(sizeof(T) == sizeof(uint32))
  {
    return ((value & 0x000000FFu) << 24) | ((value & 0x00FF0000u) >> 8) | ((value & 0x0000FF00u) << 8) | ((value & 0xFF000000u) >> 24);
  }
  else if constexpr(sizeof(T) == sizeof(uint64))
  {
    return (((value & 0xFF00000000000000ull) >> 56) | ((value & 0x00FF000000000000ull) >> 40) | ((value & 0x0000FF0000000000ull) >> 24) | ((value & 0x000000FF00000000ull) >> 8) |
            ((value & 0x00000000FF000000ull) << 8) | ((value & 0x0000000000FF0000ull) << 24) | ((value & 0x000000000000FF00ull) << 40) | ((value & 0x00000000000000FFull) << 56));
  }
}

template <class T, endian Endianness = endian::native, usize Size = sizeof(T)>
[[nodiscard]] inline constexpr T bit_cast_int(const std::byte* data) noexcept
{
  static_assert(std::is_integral_v<T>);
  static_assert(Size <= sizeof(T));

  T value = {};
  for(usize i = 0; i < Size; i++)
  {
    if constexpr(Endianness == endian::little)
    {
      value |= static_cast<T>(data[i]) << (8 * i);
    }
    else if constexpr(Endianness == endian::big)
    {
      value |= static_cast<T>(data[i]) << (8 * (Size - 1 - i));
    }
  }
  return value;
}

template <class T>
[[nodiscard]] inline T bit_cast_ptr(const std::byte* data) noexcept
{
  static_assert(std::is_trivially_constructible_v<T>);
  static_assert(std::is_trivially_copyable_v<T>);

  T value;
  std::memcpy(&value, data, sizeof(T));
  return value;
}

template <class To, class From>
To bit_cast(const From& src) noexcept
{
  static_assert(sizeof(To) == sizeof(From));
  static_assert(std::is_trivially_constructible_v<To>);
  static_assert(std::is_trivially_copyable_v<From>);
  static_assert(std::is_trivially_copyable_v<To>);

  To dst;
  std::memcpy(&dst, &src, sizeof(To));
  return dst;
}
} // namespace complex
