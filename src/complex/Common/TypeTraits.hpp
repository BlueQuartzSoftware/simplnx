#pragma once

#include <type_traits>

namespace complex
{
// Based on P1830R1
template <class...>
inline constexpr bool dependent_false = false;
} // namespace complex
