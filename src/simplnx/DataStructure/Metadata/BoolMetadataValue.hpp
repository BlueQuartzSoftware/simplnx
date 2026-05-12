#pragma once

#include "AbstractMetadataValue.hpp"

#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Metadata class implementation for reading and writing boolean metadata.
 */
class SIMPLNX_EXPORT BoolMetadataValue : public AbstractMetadataValue<bool>
{
public:
  using ParentType = AbstractMetadataValue<bool>;
  using ValueType = ParentType::ValueType;

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
   * @brief Assignment operator
   * @param rhs
   */
  ParentType& operator=(const ValueType& rhs) override;

  std::string getTypeName() const override;

protected:
  std::string toJsonImpl() const override;

  void fromJsonImpl(const std::string& json) override;

private:
  bool m_Value;
};
} // namespace nx::core
