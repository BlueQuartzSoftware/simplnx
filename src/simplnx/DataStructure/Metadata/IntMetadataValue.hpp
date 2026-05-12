#pragma once

#include "AbstractMetadataValue.hpp"

#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Metadata class implementation for reading and writing integer metadata.
 */
class SIMPLNX_EXPORT IntMetadataValue : public AbstractMetadataValue<int32>
{
public:
  using ParentType = AbstractMetadataValue<int32>;
  using ValueType = ParentType::ValueType;

  static constexpr StringLiteral k_TypeName = "int32";

  IntMetadataValue(ValueType value);
  IntMetadataValue(const IntMetadataValue& other) = default;
  IntMetadataValue(IntMetadataValue&& other) = default;
  ~IntMetadataValue() = default;

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

  std::string getTypeName() const override;

protected:
  std::string toJsonImpl() const override;

  void fromJsonImpl(const std::string& json) override;

private:
  int32 m_Value;
};
} // namespace nx::core
