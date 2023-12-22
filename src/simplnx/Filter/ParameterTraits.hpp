#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/IParameter.hpp"

#include <type_traits>

namespace nx::core
{
/**
 * @brief Specializations of ParameterTraits provide name and uuid for T
 * @tparam T
 */
template <class T, class = std::enable_if_t<std::is_base_of_v<IParameter, T>>>
struct ParameterTraits
{
};
} // namespace nx::core

#define SIMPLNX_DEF_PARAMETER_TRAITS_NAMED(type, nameString, uuidString)                                                                                                                               \
  template <>                                                                                                                                                                                          \
  struct nx::core::ParameterTraits<type>                                                                                                                                                               \
  {                                                                                                                                                                                                    \
    static inline constexpr nx::core::StringLiteral name = nameString;                                                                                                                                 \
    static inline constexpr nx::core::Uuid uuid = *nx::core::Uuid::FromString(uuidString);                                                                                                             \
  }

#define SIMPLNX_DEF_PARAMETER_TRAITS(type, uuidString) SIMPLNX_DEF_PARAMETER_TRAITS_NAMED(type, #type, uuidString)

#define SIMPLNX_NAMESPACE_CONCAT(nameSpace, type) nameSpace::type

#define SIMPLNX_DEF_PARAMETER_TRAITS_NAMEDSPACED(type, nameSpace, uuidString) SIMPLNX_DEF_PARAMETER_TRAITS_NAMED(SIMPLNX_NAMESPACE_CONCAT(nameSpace, type), #type, uuidString)
