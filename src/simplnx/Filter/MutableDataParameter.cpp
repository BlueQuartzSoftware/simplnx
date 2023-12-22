#include "MutableDataParameter.hpp"

namespace nx::core
{
MutableDataParameter::Mutability MutableDataParameter::mutability() const
{
  return Mutability::Mutable;
}
} // namespace nx::core
