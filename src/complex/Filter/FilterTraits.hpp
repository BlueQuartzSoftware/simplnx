#pragma once

#include <type_traits>

#include "complex/Filter/IFilter.hpp"

namespace complex
{
template <class T, class = std::enable_if_t<std::is_base_of_v<IFilter, T>>>
struct FilterTraits
{
};
} // namespace complex
