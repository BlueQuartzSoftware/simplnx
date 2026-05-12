#pragma once

#include "AbstractMetadataValue.hpp"

#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Metadata class implementation for reading and writing floating-point metadata.
 */
class SIMPLNX_EXPORT DoubleMetadataValue : public AbstractMetadataValue<float64>
{
public:
  using ParentType = AbstractMetadataValue<float64>;
  using ValueType = ParentType::ValueType;

  static constexpr StringLiteral k_TypeName = "double";

  DoubleMetadataValue(ValueType value = 0);
  DoubleMetadataValue(const DoubleMetadataValue& other) = default;
  DoubleMetadataValue(DoubleMetadataValue&& other) = default;
  ~DoubleMetadataValue() = default;

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
  ValueType m_Value;
};
} // namespace nx::core
