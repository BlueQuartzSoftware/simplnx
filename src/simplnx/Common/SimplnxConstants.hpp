#pragma once

#include "simplnx/Common/StringLiteral.hpp"

#define SIMPLNX_DEF_STRING_CONSTANT(name) StringLiteralType k_##name = #name

namespace nx::core::constants
{
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(Int8);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(UInt8);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(Int16);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(UInt16);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(Int32);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(UInt32);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(Int64);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(UInt64);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(Float32);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(Float64);
inline constexpr SIMPLNX_DEF_STRING_CONSTANT(USize);
} // namespace nx::core::Constants

#undef SIMPLNX_DEF_STRING_CONSTANT
