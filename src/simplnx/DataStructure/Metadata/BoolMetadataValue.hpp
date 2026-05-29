#pragma once

#include "AbstractMetadataValue.hpp"

namespace nx::core
{
/**
 * @brief Metadata class implementation for reading and writing boolean metadata.
 */
class SIMPLNX_EXPORT BoolMetadataValue : public AbstractMetadataValue<bool>
{
public:
  using ParentType = AbstractMetadataValue<bool>;
  using ValueType = typename ParentType::ValueType;

  static constexpr StringLiteral k_TypeName = "bool";

  BoolMetadataValue(ValueType value = false);
  BoolMetadataValue(const BoolMetadataValue& other) = default;
  BoolMetadataValue(BoolMetadataValue&& other) = default;
  ~BoolMetadataValue() = default;

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
  bool m_Value;
};
} // namespace nx::core
