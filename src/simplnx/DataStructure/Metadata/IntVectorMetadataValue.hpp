#pragma once

#include "AbstractVectorMetadataValue.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT Int32VectorMetadataValue : public AbstractVectorMetadataValue<int32>
{
public:
  using ParentType = AbstractVectorMetadataValue<int32>;
  using ValueType = ParentType::ValueType;
  using AssignmentReturnType = ParentType::AssignmentReturnType;

  static constexpr StringLiteral k_TypeName = "vec<int32>";

  Int32VectorMetadataValue();
  Int32VectorMetadataValue(const Int32VectorMetadataValue& rhs);
  Int32VectorMetadataValue(Int32VectorMetadataValue&& rhs) noexcept;
  ~Int32VectorMetadataValue() noexcept = default;

  Int32VectorMetadataValue& operator=(const Int32VectorMetadataValue& rhs) = default;
  Int32VectorMetadataValue& operator=(Int32VectorMetadataValue&& rhs) noexcept = default;

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
