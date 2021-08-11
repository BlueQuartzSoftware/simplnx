#include "MutableDataParameter.hpp"

namespace complex
{
MutableDataParameter::Mutability MutableDataParameter::mutability() const
{
  return Mutability::Mutable;
}
} // namespace complex
