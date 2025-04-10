#include "AbstractStringStore.hpp"

namespace nx::core
{
AbstractStringStore::iterator AbstractStringStore::begin()
{
  return Iterator(*this, 0);
}

AbstractStringStore::iterator AbstractStringStore::end()
{
  return Iterator(*this, size());
}

AbstractStringStore::const_iterator AbstractStringStore::begin() const
{
  return ConstIterator(*this, 0);
}

AbstractStringStore::const_iterator AbstractStringStore::end() const
{
  return ConstIterator(*this, size());
}

AbstractStringStore::const_iterator AbstractStringStore::cbegin() const
{
  return ConstIterator(*this, 0);
}

AbstractStringStore::const_iterator AbstractStringStore::cend() const
{
  return ConstIterator(*this, size());
}

bool AbstractStringStore::operator==(const std::vector<std::string>& values) const
{
  usize count = size();
  if(values.size() != count)
  {
    return false;
  }
  for(usize i = 0; i < count; i++)
  {
    if(values[i] != getValue(i))
    {
      return false;
    }
  }
  return true;
}
bool AbstractStringStore::operator!=(const std::vector<std::string>& values) const
{
  bool equals = (*this) == values;
  return !equals;
}
} // namespace nx::core
