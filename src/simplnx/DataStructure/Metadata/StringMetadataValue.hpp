#pragma once

#include "AbstractMetadataValue.hpp"

#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief String metadata class implementation for reading and writing string-based metadata.
 */
class SIMPLNX_EXPORT StringMetadataValue : public AbstractMetadataValue<std::string>
{
public:
  using ParentType = AbstractMetadataValue<std::string>;
  using ValueType = ParentType::ValueType;

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
   * @brief Assignment operator
   * @param rhs
   */
  ParentType& operator=(const ValueType& rhs) override;

  /**
   * @brief Returns the typename for the StringMetadata
   * @return typename string
   */
  std::string getTypeName() const override;

protected:
  std::string toJsonImpl() const override;

  void fromJsonImpl(const std::string& json) override;

private:
  std::string m_Value;
};
} // namespace nx::core
