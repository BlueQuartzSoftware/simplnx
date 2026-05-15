#include "MetaDataList.hpp"

#include "simplnx/DataStructure/Metadata/BoolMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/DoubleMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/IntMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/StringMetadataValue.hpp"
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
  // Double Metadata
  addMetaDataType(DoubleMetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<DoubleMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // Integer Metadata
  addMetaDataType(IntMetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<IntMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // String Metadata
  addMetaDataType(StringMetadataValue::k_TypeName, [](const nlohmann::json& json) {
    auto metaData = std::make_unique<StringMetadataValue>();
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
