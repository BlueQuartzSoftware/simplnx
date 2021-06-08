#include "Metadata.h"

#include <exception>

using namespace Complex;


Metadata::Metadata()
{
}

Metadata::Metadata(const Metadata& other)
: m_Map(other.m_Map)
{
}

Metadata::Metadata(Metadata&& other) noexcept
: m_Map(std::move(other.m_Map))
{
}

Metadata::~Metadata() = default;

Metadata::ValueType Metadata::getData(const KeyType& key) const
{
  return m_Map.at(key);
}
void Metadata::setData(const KeyType& key, const ValueType& value)
{
  m_Map[key] = value;
}
void Metadata::remove(const KeyType& key)
{
  m_Map.erase(key);
}
void Metadata::clear()
{
  m_Map.clear();
}
Metadata::ValueType& Metadata::operator[](const KeyType& key)
{
  return m_Map[key];
}
Metadata::Iterator Metadata::begin()
{
  return m_Map.begin();
}
Metadata::Iterator Metadata::end()
{
  return m_Map.end();
}
Metadata::ConstIterator Metadata::begin() const
{
  return m_Map.begin();
}
Metadata::ConstIterator Metadata::end() const
{
  return m_Map.end();
}

Metadata& Metadata::operator=(const Metadata& rhs)
{
  m_Map = rhs.m_Map;
  return *this;
}
Metadata& Metadata::operator=(Metadata&& rhs) noexcept
{
  m_Map = std::move(rhs.m_Map);
  return *this;
}
