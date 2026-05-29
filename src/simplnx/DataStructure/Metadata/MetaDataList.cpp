#include "MetaDataList.hpp"

#include "simplnx/DataStructure/Metadata/BoolMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/BoolVectorMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/Float64MetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/Float64VectorMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/Int32MetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/Int32VectorMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/StringMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/StringVectorMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/UnknownMetadataValue.hpp"

#include "nlohmann/json.hpp"

namespace nx::core
{
MetaDataList::MetaDataList()
{
  addDefaultTypes();
}

void MetaDataList::addDefaultTypes()
{
  // Boolean Metadata
  addMetaDataType(BoolMetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<BoolMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // Boolean Vector Metadata
  addMetaDataType(BoolVectorMetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<BoolVectorMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // Double Metadata
  addMetaDataType(Float64MetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<Float64MetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // Double Vector Metadata
  addMetaDataType(Float64VectorMetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<Float64VectorMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // Integer Metadata
  addMetaDataType(Int32MetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<Int32MetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // Integer Vector Metadata
  addMetaDataType(Int32VectorMetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<Int32VectorMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // String Metadata
  addMetaDataType(StringMetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<StringMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // String Vector Metadata
  addMetaDataType(StringVectorMetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<StringVectorMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
}

void MetaDataList::addMetaDataType(const KeyType& name, MetaDataCreationFnc constructorFnc)
{
  m_CreationMap[name] = constructorFnc;
}

std::unique_ptr<BaseMetadataValue> MetaDataList::createValueFromJson(const nlohmann::json& json) const
{
  std::string type = json[BaseMetadataValue::k_ValueTypeKey.str()].get<std::string>();

  if(!m_CreationMap.contains(type))
  {
    return std::make_unique<UnknownMetadataValue>(json);
  }

  return m_CreationMap.at(type)(json);
}
} // namespace nx::core
