#pragma once

#include "AbstractMetadataValue.hpp"

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

  IntMetadataValue(ValueType value = 0);
  IntMetadataValue(const IntMetadataValue& other) = default;
  IntMetadataValue(IntMetadataValue&& other) = default;
  ~IntMetadataValue() = default;

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
  std::string getTypeNameImpl() const override;

  nlohmann::json toJsonImpl() const override;

  void fromJsonImpl(const nlohmann::json& json) override;

private:
  int32 m_Value;
};
} // namespace nx::core
