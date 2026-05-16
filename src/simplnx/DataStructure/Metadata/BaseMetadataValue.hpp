#pragma once

#include "simplnx/Common/StringLiteral.hpp"

#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace nx::core
{
/**
 * @brief Base class for meta data values.
 */
class SIMPLNX_EXPORT BaseMetadataValue
{
public:
  static constexpr StringLiteral k_ValueTypeKey = "type";
  static constexpr StringLiteral k_ValueKey = "value";

  BaseMetadataValue(const BaseMetadataValue& other) = default;
  BaseMetadataValue(BaseMetadataValue&& other) noexcept = default;
  ~BaseMetadataValue() noexcept = default;

  /**
   * @brief Returns the metadata type name.
   * @return std::string
   */
  virtual std::string getTypeName() const;

  /**
   * @brief Returns the metadata's json representation.
   * @return json
   */
  virtual nlohmann::json toJson() const;

  /**
   * @brief Updates the metadata from the provided json value.
   * @param json metadata json representation
   */
  virtual void fromJson(const nlohmann::json& json);

protected:
  BaseMetadataValue() = default;
};
} // namespace nx::core
