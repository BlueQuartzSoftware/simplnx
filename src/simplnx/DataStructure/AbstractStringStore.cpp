#include "AbstractStringStore.hpp"

namespace nx::core
{
usize AbstractStringStore::size() const
{
  return xarray().size();
}

bool AbstractStringStore::empty() const
{
  return xarray().size() == 0;
}

AbstractStringStore::reference AbstractStringStore::operator[](usize index)
{
  return xarray().flat(index);
}
AbstractStringStore::const_reference AbstractStringStore::operator[](usize index) const
{
  return xarray().flat(index);
}
AbstractStringStore::const_reference AbstractStringStore::at(usize index) const
{
  return getValue(index);
}

AbstractStringStore::const_reference AbstractStringStore::getValue(usize index) const
{
  return xarray().flat(index);
}

void AbstractStringStore::setValue(usize index, const value_type& value)
{
  xarray().flat(index) = value;
}

AbstractStringStore::iterator AbstractStringStore::begin()
{
  return xarray().begin();
}

AbstractStringStore::iterator AbstractStringStore::end()
{
  return xarray().end();
}

AbstractStringStore::const_iterator AbstractStringStore::begin() const
{
  return xarray().begin();
}

AbstractStringStore::const_iterator AbstractStringStore::end() const
{
  return xarray().end();
}

AbstractStringStore::const_iterator AbstractStringStore::cbegin() const
{
  return xarray().begin();
}

AbstractStringStore::const_iterator AbstractStringStore::cend() const
{
  return xarray().end();
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
