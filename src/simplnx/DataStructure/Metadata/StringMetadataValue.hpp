#pragma once

#include "AbstractMetadataValue.hpp"

#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
/**
 * @brief String metadata class implementation for reading and writing string-based metadata.
 */
class SIMPLNX_EXPORT StringMetadataValue : public AbstractMetadataValue<std::string>
{
public:
  using ParentType = AbstractMetadataValue<std::string>;
  using ValueType = typename ParentType::ValueType;

  static constexpr StringLiteral k_TypeName = "string";

  StringMetadataValue();
  StringMetadataValue(const ValueType& value);
  StringMetadataValue(const StringMetadataValue& other) = default;
  StringMetadataValue(StringMetadataValue&& other) = default;
  ~StringMetadataValue() = default;

  /**
   * @brief Default cast to the type in question
   * @return metadata value
   */
  operator ValueType() const override;

  /**
   * @brief Returns the stored value.
   * @return metadata value
   */
  ValueType getValue() const override;

  /**
   * @brief Sets the stored value.
   * @param value
   */
  void setValue(const ValueType& value) override;

  /**
   * @brief Default equality operator
   * @param rhs value to compare against
   * @return is equal
   */
  bool operator==(const ValueType& rhs) const override;

  /**
   * @brief Assignment operator
   * @param rhs
   */
  ParentType& operator=(const ValueType& rhs) override;

protected:
  /**
   * @brief Returns the typename for the StringMetadata
   * @return typename string
   */
  std::string getTypeNameImpl() const override;

  nlohmann::json toJsonImpl() const override;

  void fromJsonImpl(const nlohmann::json& json) override;

private:
  std::string m_Value;
};
} // namespace nx::core
