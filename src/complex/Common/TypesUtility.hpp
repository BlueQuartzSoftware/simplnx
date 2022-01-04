#pragma once

#include "complex/Common/Types.hpp"

#include <stdexcept>

#include <type_traits>

namespace complex
{
template <class... T>
inline constexpr bool AlwaysFalse_v = false;

template <class T>
constexpr NumericType GetNumericType() noexcept
{
  if constexpr(std::is_same_v<T, int8>)
  {
    return NumericType::int8;
  }
  else if constexpr(std::is_same_v<T, uint8>)
  {
    return NumericType::uint8;
  }
  else if constexpr(std::is_same_v<T, int16>)
  {
    return NumericType::int16;
  }
  else if constexpr(std::is_same_v<T, uint16>)
  {
    return NumericType::uint16;
  }
  else if constexpr(std::is_same_v<T, int32>)
  {
    return NumericType::int32;
  }
  else if constexpr(std::is_same_v<T, uint32>)
  {
    return NumericType::uint32;
  }
  else if constexpr(std::is_same_v<T, int64>)
  {
    return NumericType::int64;
  }
  else if constexpr(std::is_same_v<T, uint64>)
  {
    return NumericType::uint64;
  }
  else if constexpr(std::is_same_v<T, float32>)
  {
    return NumericType::float32;
  }
  else if constexpr(std::is_same_v<T, float64>)
  {
    return NumericType::float64;
  }
  else
  {
    static_assert(AlwaysFalse_v<T>, "complex::GetNumericType: Unsupported type");
  }
}

inline constexpr usize GetNumericTypeSize(const NumericType& numericType)
{
  usize typeSize = 0;
  switch(numericType)
  {
  case NumericType::int8:
    typeSize = sizeof(int8);
    break;
  case NumericType::uint8:
    typeSize = sizeof(uint8);
    break;
  case NumericType::int16:
    typeSize = sizeof(int16);
    break;
  case NumericType::uint16:
    typeSize = sizeof(uint16);
    break;
  case NumericType::int32:
    typeSize = sizeof(int32);
    break;
  case NumericType::uint32:
    typeSize = sizeof(uint32);
    break;
  case NumericType::int64:
    typeSize = sizeof(int64);
    break;
  case NumericType::uint64:
    typeSize = sizeof(uint64);
    break;
  case NumericType::float32:
    typeSize = sizeof(float32);
    break;
  case NumericType::float64:
    typeSize = sizeof(float64);
    break;
  default:
    throw std::runtime_error("complex::GetNumericTypeSize: Unsupported type");
  }

  return typeSize;
}
} // namespace complex
