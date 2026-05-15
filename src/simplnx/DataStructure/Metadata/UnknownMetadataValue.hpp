#pragma once

#include "BaseMetadataValue.hpp"

namespace nx::core
{
/**
 * @brief Metadata class implementation for reading and writing unkown metadata.
 * This is used when reading metadata of a type that is not known to the MetaDataList
 * and preserves the typename of the desired type.
 */
class SIMPLNX_EXPORT UnknownMetadataValue : public BaseMetadataValue
{
public:
  using ParentType = BaseMetadataValue;

  static constexpr StringLiteral k_TypeName = "[unknown]";

  /**
   * @brief Constructs an UnknownMetadataValue object with the specified json.
   * @param jsonStr json for unknown metadata value type
   */
  UnknownMetadataValue(const nlohmann::json& json);
  UnknownMetadataValue(const UnknownMetadataValue& other) = default;
  UnknownMetadataValue(UnknownMetadataValue&& other) = default;
  ~UnknownMetadataValue() = default;

  /**
   * @brief Returns the typename for the UnknownMetadataValue
   * @return typename string
   */
  std::string getTypeName() const override;

  /**
   * @brief Returns the unknown metadata json.
   * @return json
   */
  nlohmann::json toJson() const override;

  /**
   * Saves the unknown metadata's json for later use.
   * @param json
   */
  void fromJson(const nlohmann::json& json) override;

private:
  nlohmann::json m_Json;
};
} // namespace nx::core
