#pragma once

#include <cstddef>
#include <cstdint>

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
  boolean,
  error = 255
};
} // namespace complex
