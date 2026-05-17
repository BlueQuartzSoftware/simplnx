#include "Metadata.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Metadata/MetaDataList.hpp"

#include "nlohmann/json.hpp"

using namespace nx::core;

Metadata::Metadata() = default;

Metadata::Metadata(const Metadata& rhs) = default;

Metadata::Metadata(Metadata&& rhs) = default;

Metadata& Metadata::operator=(const Metadata& rhs) = default;

Metadata& Metadata::operator=(Metadata&& rhs) noexcept = default;

Metadata::~Metadata() noexcept = default;

bool Metadata::isEmpty() const
{
  return m_Map.empty();
}

bool Metadata::contains(const KeyType& key) const
{
  return m_Map.find(key) != m_Map.end();
}

const Metadata::ValuePtr& Metadata::getDataValuePtr(const KeyType& key) const
{
  if(!contains(key))
  {
    return nullptr;
  }

  return m_Map.at(key);
}

void Metadata::setDataValuePtr(const KeyType& key, const ValuePtr& value)
{
  // m_Map.insert(key, value);
  m_Map[key] = std::move(value);
}

void Metadata::remove(const KeyType& key)
{
  m_Map.erase(key);
}

void Metadata::clear()
{
  m_Map.clear();
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

nlohmann::json Metadata::toJson() const
{
  nlohmann::json json = nlohmann::json::object();
  for(const auto& [key, value] : m_Map)
  {
    json[key] = value->toJson();
  }

  return json;
}

void Metadata::fromJson(const std::string& jsonStr)
{
  MetaDataList* metaDataList = Application::Instance()->getMetaDataList();

  nlohmann::json json = nlohmann::json::parse(jsonStr);
  for(auto& [key, value] : json.items())
  {
    m_Map[key] = metaDataList->createValueFromJson(value);
  }
}
