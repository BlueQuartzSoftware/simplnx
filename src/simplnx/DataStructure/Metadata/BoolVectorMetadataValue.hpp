#pragma once

#include "AbstractVectorMetadataValue.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT BoolVectorMetadataValue : public AbstractVectorMetadataValue<bool>
{
public:
  using ParentType = AbstractVectorMetadataValue<bool>;
  using ValueType = ParentType::ValueType;
  using AssignmentReturnType = ParentType::AssignmentReturnType;

  static constexpr StringLiteral k_TypeName = "vec<bool>";

  BoolVectorMetadataValue();
  BoolVectorMetadataValue(const BoolVectorMetadataValue& rhs);
  BoolVectorMetadataValue(BoolVectorMetadataValue&& rhs) noexcept;
  ~BoolVectorMetadataValue() noexcept = default;

  BoolVectorMetadataValue& operator=(const BoolVectorMetadataValue& rhs);
  BoolVectorMetadataValue& operator=(BoolVectorMetadataValue&& rhs) noexcept;

  /**
   * @brief Assignment operator
   * @param rhs
   */
  AssignmentReturnType& operator=(const ValueType& rhs) override;

  /**
   * @brief Default equality operator
   * @param rhs value to compare against
   * @return is equal
   */
  bool operator==(const ValueType& rhs) const override;

protected:
  std::string getTypeNameImpl() const override;
};
} // namespace nx::core
