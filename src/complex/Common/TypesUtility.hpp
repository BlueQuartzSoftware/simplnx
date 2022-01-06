#pragma once

#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/Types.hpp"

#include <stdexcept>
#include <type_traits>

namespace complex
{
template <class... T>
inline constexpr bool AlwaysFalse_v = false;

/**
 * @brief Returns the NumericType associated with T.
 * @tparam T
 * @return
 */
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

/**
 * @brief Returns sizeof(T) for the T associated with the given NumericType.
 * @param numericType
 * @return
 */
inline constexpr usize GetNumericTypeSize(NumericType numericType)
{
  switch(numericType)
  {
  case NumericType::int8:
    return sizeof(int8);
  case NumericType::uint8:
    return sizeof(uint8);
  case NumericType::int16:
    return sizeof(int16);
  case NumericType::uint16:
    return sizeof(uint16);
  case NumericType::int32:
    return sizeof(int32);
  case NumericType::uint32:
    return sizeof(uint32);
  case NumericType::int64:
    return sizeof(int64);
  case NumericType::uint64:
    return sizeof(uint64);
  case NumericType::float32:
    return sizeof(float32);
  case NumericType::float64:
    return sizeof(float64);
  default:
    throw std::runtime_error("complex::GetNumericTypeSize: Unsupported type");
  }
}
/**
 * @brief Returns a string representation of the passed in NumericType
 * @param numericType
 * @return
 */
inline constexpr StringLiteral NumericTypeToString(NumericType numericType)
{
  switch(numericType)
  {
  case NumericType::int8: {
    return "int8";
  }
  case NumericType::uint8: {
    return "uint8";
  }
  case NumericType::int16: {
    return "int16";
  }
  case NumericType::uint16: {
    return "uint16";
  }
  case NumericType::int32: {
    return "int32";
  }
  case NumericType::uint32: {
    return "uint32";
  }
  case NumericType::int64: {
    return "int64";
  }
  case NumericType::uint64: {
    return "uint64";
  }
  case NumericType::float32: {
    return "float32";
  }
  case NumericType::float64: {
    return "float64";
  }
  default:
    throw std::runtime_error("complex::NumericTypeToString: Unknown NumericType");
  }
}
} // namespace complex
