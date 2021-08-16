#pragma once

#include <type_traits>

#include "complex/Filter/IParameter.hpp"

namespace complex
{
template <class T, class = std::enable_if_t<std::is_base_of_v<IParameter, T>>>
struct ParameterTraits
{
};
} // namespace complex
