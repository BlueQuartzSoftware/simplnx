#pragma once

#include <cstring>
#include <type_traits>

#include "simplnx/Common/Types.hpp"

#ifdef _WIN32
#define SIMPLNX_BYTE_ORDER little
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SIMPLNX_BYTE_ORDER little
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SIMPLNX_BYTE_ORDER big
#endif
#endif

namespace nx::core
{
enum class endian : uint8
{
  little = 0,
  big = 1,
  native = SIMPLNX_BYTE_ORDER
};

inline endian checkEndian()
{
  constexpr uint32_t i = 0x01020304;
  const auto* u8 = reinterpret_cast<const std::byte*>(&i);

  return u8[0] == std::byte{0x01} ? endian::big : endian::little;
}

inline constexpr uint16 byteswap16(uint16 value) noexcept
{
  return ((value >> 8) & 0xFFu) | ((value & 0xFFu) << 8);
}

inline constexpr uint32 byteswap32(uint32 value) noexcept
{
  return ((value & 0x000000FFu) << 24) | ((value & 0x00FF0000u) >> 8) | ((value & 0x0000FF00u) << 8) | ((value & 0xFF000000u) >> 24);
}

inline constexpr uint64 byteswap64(uint64 value) noexcept
{
  return (((value & 0xFF00000000000000ull) >> 56) | ((value & 0x00FF000000000000ull) >> 40) | ((value & 0x0000FF0000000000ull) >> 24) | ((value & 0x000000FF00000000ull) >> 8) |
          ((value & 0x00000000FF000000ull) << 8) | ((value & 0x0000000000FF0000ull) << 24) | ((value & 0x000000000000FF00ull) << 40) | ((value & 0x00000000000000FFull) << 56));
}

template <class T, endian Endianness = endian::native, usize Size = sizeof(T)>
inline constexpr T bit_cast_int(const std::byte* data) noexcept
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
inline T bit_cast_ptr(const std::byte* data) noexcept
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

template <class T>
inline constexpr T byteswap(T value) noexcept
{
  static_assert(std::is_arithmetic_v<T>, "byteswap only works on arithmetic types");
  if constexpr(sizeof(T) == sizeof(uint8))
  {
    return value;
  }
  else if constexpr(sizeof(T) == sizeof(uint16))
  {
    return byteswap16(value);
  }
  else if constexpr(sizeof(T) == sizeof(uint32))
  {
    if constexpr(std::is_floating_point_v<T>)
    {
      return bit_cast<T>(byteswap32(bit_cast<uint32>(value)));
    }
    else
    {
      return byteswap32(value);
    }
  }
  else if constexpr(sizeof(T) == sizeof(uint64))
  {
    if constexpr(std::is_floating_point_v<T>)
    {
      return bit_cast<T>(byteswap64(bit_cast<uint64>(value)));
    }
    else
    {
      return byteswap64(value);
    }
  }
}
} // namespace nx::core
