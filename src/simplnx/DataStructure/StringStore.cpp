#include "StringStore.hpp"

namespace nx::core
{
StringStore::StringStore(uint64 size)
: AbstractStringStore()
{
  m_xarray.resize({size});
}

StringStore::StringStore(const std::vector<std::string>& strings)
: AbstractStringStore()
{
  usize count = strings.size();
  m_xarray.resize({count});
  for (usize i = 0; i < count; i++)
  {
    m_xarray.flat(i) = strings[i];
  }
}

StringStore::~StringStore() = default;

StringStore::xarray_type& StringStore::xarray()
{
  return m_xarray;
}

const StringStore::xarray_type& StringStore::xarray() const
{
  return m_xarray;
}

void StringStore::resize(usize count)
{
  usize oldSize = size();
  auto data = xt::xarray<std::string>::from_shape({count});
  for (usize i = 0; i < count && i < oldSize; i++)
  {
    data.flat(i) = m_xarray.flat(i);
  }
  m_xarray.resize({count});
  m_xarray = std::move(data);
}

std::unique_ptr<AbstractStringStore> StringStore::deepCopy() const
{
  uint64 count = size();
  auto newStore = std::make_unique<StringStore>(count);
  for(uint64 i = 0; i < count; i++)
  {
    newStore->setValue(i, getValue(i));
  }
  return newStore;
}

AbstractStringStore& StringStore::operator=(const std::vector<std::string>& values)
{
  usize count = values.size();
  m_xarray = xt::xarray<std::string>::from_shape({count});
  for (usize i = 0; i < count; i++)
  {
    m_xarray.flat(i) = values[i];
  }
  return *this;
}
} // namespace nx::core
