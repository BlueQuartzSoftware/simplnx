#pragma once

#include <cstddef>
#include <cstdint>
#include <set>

namespace complex
{
namespace types
{
template <bool UseBoolean, bool UseInt8V, bool UseUInt8V, bool UseInt16V, bool UseUInt16V, bool UseInt32V, bool UseUInt32V, bool UseInt64V, bool UseUInt64V, bool UseFloat32V, bool UseFloat64V>
struct ArrayTypeOptions
{
  static inline constexpr bool UsingBoolean = UseBoolean;
  static inline constexpr bool UsingInt8 = UseInt8V;
  static inline constexpr bool UsingUInt8 = UseUInt8V;
  static inline constexpr bool UsingInt16 = UseInt16V;
  static inline constexpr bool UsingUInt16 = UseUInt16V;
  static inline constexpr bool UsingInt32 = UseInt32V;
  static inline constexpr bool UsingUInt32 = UseUInt32V;
  static inline constexpr bool UsingInt64 = UseInt64V;
  static inline constexpr bool UsingUInt64 = UseUInt64V;
  static inline constexpr bool UsingFloat32 = UseFloat32V;
  static inline constexpr bool UsingFloat64 = UseFloat64V;
};

using ArrayUseAllTypes = ArrayTypeOptions<true, true, true, true, true, true, true, true, true, true, true>;
using NoBooleanType = ArrayTypeOptions<false, true, true, true, true, true, true, true, true, true, true>;
using ArrayUseIntegerTypes = ArrayTypeOptions<false, true, true, true, true, true, true, true, true, false, false>;
using ArrayUseFloatingTypes = ArrayTypeOptions<false, false, false, false, false, false, false, false, false, true, true>;
using ArrayUseSignedTypes = ArrayTypeOptions<false, true, false, true, false, true, false, true, false, true, true>;
using ArrayUseUnsignedTypes = ArrayTypeOptions<false, false, true, false, true, false, true, false, true, false, false>;

using int8 = std::int8_t;
using uint8 = std::uint8_t;
using int16 = std::int16_t;
using uint16 = std::uint16_t;
using int32 = std::int32_t;
using uint32 = std::uint32_t;
using int64 = std::int64_t;
using uint64 = std::uint64_t;

using usize = std::size_t;

using float32 = float;
using float64 = double;
} // namespace types
using namespace types;

enum class NumericType : uint8
{
  int8,
  uint8,
  int16,
  uint16,
  int32,
  uint32,
  int64,
  uint64,
  float32,
  float64
};

enum class DataType : uint8
{
  int8,
  uint8,
  int16,
  uint16,
  int32,
  uint32,
  int64,
  uint64,
  float32,
  float64,
  boolean
};

enum class RunState
{
  Idle = 0,
  Queued = 1,
  Preflighting = 2,
  Executing = 3
};

enum class FaultState
{
  None = 0,
  Warnings = 1,
  Errors = 2
};

inline const std::set<DataType>& GetAllDataTypes()
{
  static const std::set<DataType> dataTypes = {complex::DataType::int8,    complex::DataType::uint8,   complex::DataType::int16,  complex::DataType::uint16,
                                               complex::DataType::int32,   complex::DataType::uint32,  complex::DataType::int64,  complex::DataType::uint64,
                                               complex::DataType::float32, complex::DataType::float64, complex::DataType::boolean};
  return dataTypes;
}

inline const std::set<DataType>& GetAllNumericTypes()
{
  static const std::set<DataType> dataTypes = {complex::DataType::int8,   complex::DataType::uint8, complex::DataType::int16,  complex::DataType::uint16,  complex::DataType::int32,
                                               complex::DataType::uint32, complex::DataType::int64, complex::DataType::uint64, complex::DataType::float32, complex::DataType::float64};
  return dataTypes;
}

inline const std::set<DataType>& GetIntegerDataTypes()
{
  static const std::set<DataType> dataTypes = {complex::DataType::int8,  complex::DataType::uint8,  complex::DataType::int16, complex::DataType::uint16,
                                               complex::DataType::int32, complex::DataType::uint32, complex::DataType::int64, complex::DataType::uint64};
  return dataTypes;
}

} // namespace complex
