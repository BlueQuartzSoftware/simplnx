#include "ValueParameter.hpp"

namespace nx::core
{
ValueParameter::Type ValueParameter::type() const
{
  return Type::Value;
}
} // namespace nx::core
