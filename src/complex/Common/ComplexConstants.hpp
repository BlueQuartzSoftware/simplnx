#pragma once

#include "complex/Common/StringLiteral.hpp"

#define MAKE_STRING_CONSTANT(name) static inline constexpr StringLiteral k_##name = #name;

namespace complex
{
namespace Constants
{
MAKE_STRING_CONSTANT(Int8)
MAKE_STRING_CONSTANT(UInt8)
MAKE_STRING_CONSTANT(Int16)
MAKE_STRING_CONSTANT(UInt16)
MAKE_STRING_CONSTANT(Int32)
MAKE_STRING_CONSTANT(UInt32)
MAKE_STRING_CONSTANT(Int64)
MAKE_STRING_CONSTANT(UInt64)
MAKE_STRING_CONSTANT(Float32)
MAKE_STRING_CONSTANT(Float64)
MAKE_STRING_CONSTANT(SizeT)
} // namespace Constants

} // namespace complex
