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
  using MetaDataCreationFnc = std::function<CreatedValueType(const nlohmann::json&)>;
  using ContainerType = std::map<KeyType, MetaDataCreationFnc>;

  MetaDataList();
  MetaDataList(const MetaDataList& other) = default;
  MetaDataList(MetaDataList&& other) = default;
  ~MetaDataList() = default;

  /**
   * @brief Add a meta data value type for creation from json.
   * @param name Metadata type name
   * @param constructorFnc Function to create and return a metadata value pointer.
   */
  void addMetaDataType(const KeyType& name, MetaDataCreationFnc constructorFnc);

  /**
   * @brief Creates and returns a metadata value from the provided json.
   * @param json
   * @return std::unique_ptr<BaseMetadataValue>
   */
  std::unique_ptr<BaseMetadataValue> createValueFromJson(const nlohmann::json& json) const;

protected:
  /**
   * @brief Adds default simplnx metadata value types.
   */
  void addDefaultTypes();

private:
  ContainerType m_CreationMap;
};
} // namespace nx::core
