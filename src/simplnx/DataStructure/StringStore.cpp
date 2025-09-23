#include "StringStore.hpp"

namespace nx::core
{
StringStore::StringStore(uint64 size)
: AbstractStringStore()
, m_Data(size)
{
}

StringStore::StringStore(std::vector<std::string> strings)
: AbstractStringStore()
, m_Data(std::move(strings))
{
}

StringStore::~StringStore() = default;

usize StringStore::size() const
{
  return m_Data.size();
}

bool StringStore::empty() const
{
  return m_Data.size() == 0;
}

StringStore::reference StringStore::operator[](usize index)
{
  return m_Data[index];
}
StringStore::const_reference StringStore::operator[](usize index) const
{
  return m_Data[index];
}
StringStore::const_reference StringStore::at(usize index) const
{
  return getValue(index);
}

StringStore::const_reference StringStore::getValue(usize index) const
{
  return m_Data.at(index);
}

void StringStore::setValue(usize index, const value_type& value)
{
  m_Data.at(index) = value;
}

void StringStore::resize(usize count)
{
  m_Data.resize(count);
}

std::unique_ptr<AbstractStringStore> StringStore::deepCopy() const
{
  return std::make_unique<StringStore>(m_Data);
}

AbstractStringStore& StringStore::operator=(const std::vector<std::string>& values)
{
  m_Data = values;
  return *this;
}
} // namespace nx::core
