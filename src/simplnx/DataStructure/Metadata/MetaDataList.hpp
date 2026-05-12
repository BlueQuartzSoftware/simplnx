#pragma once

#include "simplnx/DataStructure/Metadata/BaseMetadataValue.hpp"

#include "simplnx/simplnx_export.hpp"

#include <functional>
#include <map>
#include <memory>

namespace nx::core
{
class SIMPLNX_EXPORT MetaDataList
{
public:
  using KeyType = std::string;
  using CreatedValueType = std::unique_ptr<BaseMetadataValue>;
  using MetaDataCreationFnc = std::function<CreatedValueType(const std::string&)>;
  using ContainerType = std::map<KeyType, MetaDataCreationFnc>;

  MetaDataList();
  MetaDataList(const MetaDataList& other) = default;
  MetaDataList(MetaDataList&& other) = default;
  ~MetaDataList() = default;

  void addMetaDataType(const KeyType& name, MetaDataCreationFnc constructorFnc);

  std::unique_ptr<BaseMetadataValue> createValueFromJson(const std::string& jsonStr) const;

protected:
  void addDefaultTypes();

private:
  ContainerType m_CreationMap;
};
} // namespace nx::core
