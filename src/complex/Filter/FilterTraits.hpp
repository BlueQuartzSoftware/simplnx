#pragma once

#include <type_traits>

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @brief Specializations of FilterTraits provide name and uuid for T
 * @tparam T
 */
template <class T, class = std::enable_if_t<std::is_base_of_v<IFilter, T>>>
struct FilterTraits
{
};
} // namespace complex

#define COMPLEX_DEF(ns_name) #ns_name

#define COMPLEX_DEF_FILTER_TRAITS_NAMED(ns, filter_name, uuidString)                                                                                                                                   \
  template <>                                                                                                                                                                                          \
  struct complex::FilterTraits<ns::filter_name>                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    static inline constexpr complex::StringLiteral className = #filter_name;                                                                                                                           \
    static inline constexpr complex::StringLiteral name = COMPLEX_DEF(ns::filter_name);                                                                                                                \
    static inline constexpr complex::Uuid uuid = *complex::Uuid::FromString(uuidString);                                                                                                               \
  }

#define COMPLEX_DEF_FILTER_TRAITS(ns, name, uuidString) COMPLEX_DEF_FILTER_TRAITS_NAMED(ns, name, uuidString)

#define COMPLEX_NAMESPACE_CONCAT(nameSpace, type) nameSpace::type

#define COMPLEX_DEF_FILTER_TRAITS_NAMEDSPACED(type, nameSpace, uuidString) COMPLEX_DEF_FILTER_TRAITS_NAMED(COMPLEX_NAMESPACE_CONCAT(nameSpace, type), #type, uuidString)
