#include "IStringStore.hpp"

namespace nx::core
{
IStringStore::iterator IStringStore::begin()
{
  return Iterator(*this, 0);
}

IStringStore::iterator IStringStore::end()
{
  return Iterator(*this, size());
}

IStringStore::const_iterator IStringStore::begin() const
{
  return ConstIterator(*this, 0);
}

IStringStore::const_iterator IStringStore::end() const
{
  return ConstIterator(*this, size());
}

IStringStore::const_iterator IStringStore::cbegin() const
{
  return ConstIterator(*this, 0);
}

IStringStore::const_iterator IStringStore::cend() const
{
  return ConstIterator(*this, size());
}

bool IStringStore::operator==(const std::vector<std::string>& values) const
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
bool IStringStore::operator!=(const std::vector<std::string>& values) const
{
  bool equals = (*this) == values;
  return !equals;
}
} // namespace nx::core
