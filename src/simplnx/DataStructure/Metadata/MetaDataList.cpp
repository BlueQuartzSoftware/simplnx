#include "MetaDataList.hpp"

#include "simplnx/DataStructure/Metadata/BoolMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/DoubleMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/IntMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/StringMetadataValue.hpp"

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
  addMetaDataType(BoolMetadataValue::k_TypeName, [](const std::string& json) {
    auto metaData = std::make_unique<BoolMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // Double Metadata
  addMetaDataType(DoubleMetadataValue::k_TypeName, [](const std::string& json) {
    auto metaData = std::make_unique<DoubleMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // Integer Metadata
  addMetaDataType(IntMetadataValue::k_TypeName, [](const std::string& json) {
    auto metaData = std::make_unique<IntMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
  // String Metadata
  addMetaDataType(StringMetadataValue::k_TypeName, [](const std::string& json) {
    auto metaData = std::make_unique<StringMetadataValue>();
    metaData->fromJson(json);
    return metaData;
  });
}

void MetaDataList::addMetaDataType(const KeyType& name, MetaDataCreationFnc constructorFnc)
{
  m_CreationMap[name] = constructorFnc;
}

std::unique_ptr<BaseMetadataValue> MetaDataList::createValueFromJson(const std::string& jsonStr) const
{
  nlohmann::json json(jsonStr);
  std::string type = json[BaseMetadataValue::k_ValueTypeKey].get<std::string>();

  return m_CreationMap.at(type)(json);
}
} // namespace nx::core
