#pragma once

#include "complex/Common/StringLiteral.hpp"

#define COMPLEX_DEF_STRING_CONSTANT(name) StringLiteral k_##name = #name

namespace complex
{
namespace Constants
{
inline constexpr COMPLEX_DEF_STRING_CONSTANT(Int8);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(UInt8);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(Int16);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(UInt16);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(Int32);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(UInt32);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(Int64);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(UInt64);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(Float32);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(Float64);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(USize);
inline constexpr COMPLEX_DEF_STRING_CONSTANT(IndexDataObjectAdded);
} // namespace Constants
} // namespace complex

#undef COMPLEX_DEF_STRING_CONSTANT
