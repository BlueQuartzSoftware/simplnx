#pragma once

#include <cstddef>
#include <cstdint>
#include <set>

namespace complex
{
namespace types
{
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

static const std::set<DataType> k_AllDataTypes = {complex::DataType::int8,    complex::DataType::uint8,   complex::DataType::int16,  complex::DataType::uint16,
                                                  complex::DataType::int32,   complex::DataType::uint32,  complex::DataType::int64,  complex::DataType::uint64,
                                                  complex::DataType::float32, complex::DataType::float64, complex::DataType::boolean};
static const std::set<DataType> k_AllNumericDataTypes = {complex::DataType::int8,   complex::DataType::uint8, complex::DataType::int16,  complex::DataType::uint16,  complex::DataType::int32,
                                                         complex::DataType::uint32, complex::DataType::int64, complex::DataType::uint64, complex::DataType::float32, complex::DataType::float64};

inline const std::set<DataType> k_AllIntegerDataTypes = {complex::DataType::int8,  complex::DataType::uint8,  complex::DataType::int16, complex::DataType::uint16,
                                                         complex::DataType::int32, complex::DataType::uint32, complex::DataType::int64, complex::DataType::uint64};

} // namespace complex
