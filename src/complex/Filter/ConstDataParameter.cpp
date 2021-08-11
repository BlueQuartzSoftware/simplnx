#include "ConstDataParameter.hpp"

namespace complex
{
ConstDataParameter::Mutability ConstDataParameter::mutability() const
{
  return Mutability::Const;
}
} // namespace complex
