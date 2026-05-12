#pragma once

#include "BaseMetadataValue.hpp"

#include "simplnx/Common/StringLiteral.hpp"

#include <string>

namespace nx::core
{
/**
* @brief Base class for meta data values.
*/
class BaseMetadataValue
{
public:
  static constexpr StringLiteral k_ValueTypeKey = "type";
  static constexpr StringLiteral k_ValueKey = "value";

  BaseMetadataValue(const BaseMetadataValue& other) = default;
  BaseMetadataValue(BaseMetadataValue&& other) = default;
  ~BaseMetadataValue() noexcept = default;

  virtual std::string toJson() const;

  virtual void fromJson(const std::string& json);

protected:
  BaseMetadataValue() = default;
};
} // namespace nx::core
