#include "Metadata.hpp"

#include "nlohmann/json.hpp"

using namespace nx::core;

Metadata::Metadata() = default;

Metadata::Metadata(const Metadata& rhs) = default;

Metadata::Metadata(Metadata&& rhs) = default;

Metadata& Metadata::operator=(const Metadata& rhs) = default;

Metadata& Metadata::operator=(Metadata&& rhs) noexcept = default;

Metadata::~Metadata() noexcept = default;

bool Metadata::contains(const KeyType& key) const
{
  return m_Map.find(key) != m_Map.end();
}

const Metadata::ValueType& Metadata::getData(const KeyType& key) const
{
  return m_Map.at(key);
}

void Metadata::setData(const KeyType& key, const ValueType& value)
{
  //m_Map.insert(key, value);
  //m_Map[key] = std::move(value);
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

std::string Metadata::toJson() const
{
  nlohmann::json json;
  for(const auto& [key, value] : m_Map)
  {
    json[key] = value.toJson();
  }

  return json;
}

void Metadata::fromJson(const std::string& jsonStr)
{
  nlohmann::json json(jsonStr);
  for(auto& [key, value] : json.items())
  {
    // m_Map[key] = valueFromJson(value);
  }
}