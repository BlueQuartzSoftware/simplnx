#pragma once

#include <type_traits>

namespace nx::core
{
// Based on P1830R1
template <class...>
inline constexpr bool dependent_false = false;

// Based on std::to_underlying from c++23
template <class Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
{
  return static_cast<std::underlying_type_t<Enum>>(e);
}
} // namespace nx::core
