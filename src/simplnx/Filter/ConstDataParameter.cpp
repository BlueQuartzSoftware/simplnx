#include "ConstDataParameter.hpp"

namespace nx::core
{
ConstDataParameter::Mutability ConstDataParameter::mutability() const
{
  return Mutability::Const;
}
} // namespace nx::core
