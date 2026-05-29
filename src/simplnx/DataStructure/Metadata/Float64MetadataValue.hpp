#pragma once

#include "AbstractMetadataValue.hpp"

namespace nx::core
{
/**
 * @brief Metadata class implementation for reading and writing floating-point metadata.
 */
class SIMPLNX_EXPORT Float64MetadataValue : public AbstractMetadataValue<float64>
{
public:
  using ParentType = AbstractMetadataValue<float64>;
  using ValueType = typename ParentType::ValueType;

  static constexpr StringLiteral k_TypeName = "float64";

  Float64MetadataValue(ValueType value = 0.0);
  Float64MetadataValue(const Float64MetadataValue& other) = default;
  Float64MetadataValue(Float64MetadataValue&& other) = default;
  ~Float64MetadataValue() = default;

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
  ValueType m_Value;
};
} // namespace nx::core
