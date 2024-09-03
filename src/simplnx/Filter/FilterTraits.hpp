#pragma once

//#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <type_traits>

namespace nx::core
{
/**
 * @brief Specializations of FilterTraits provide name and uuid for T
 * @tparam T
 */
template <class T, class = std::enable_if_t<std::is_base_of_v<IFilter, T>>>
struct FilterTraits
{
};
} // namespace nx::core

#define SIMPLNX_DEF(ns_name) #ns_name

#define SIMPLNX_DEF_FILTER_TRAITS_NAMED(ns, filter_name, uuidString)                                                                                                                                   \
  template <>                                                                                                                                                                                          \
  struct nx::core::FilterTraits<ns::filter_name>                                                                                                                                                       \
  {                                                                                                                                                                                                    \
    static inline constexpr nx::core::StringLiteral className = #filter_name;                                                                                                                          \
    static inline constexpr nx::core::StringLiteral name = SIMPLNX_DEF(ns::filter_name);                                                                                                               \
    static inline constexpr nx::core::Uuid uuid = *nx::core::Uuid::FromString(uuidString);                                                                                                             \
  }

#define SIMPLNX_DEF_FILTER_TRAITS(ns, name, uuidString) SIMPLNX_DEF_FILTER_TRAITS_NAMED(ns, name, uuidString)

#define SIMPLNX_NAMESPACE_CONCAT(nameSpace, type) nameSpace::type

#define SIMPLNX_DEF_FILTER_TRAITS_NAMEDSPACED(type, nameSpace, uuidString) SIMPLNX_DEF_FILTER_TRAITS_NAMED(SIMPLNX_NAMESPACE_CONCAT(nameSpace, type), #type, uuidString)
