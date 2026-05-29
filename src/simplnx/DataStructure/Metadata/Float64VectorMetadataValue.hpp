#pragma once

#include "AbstractVectorMetadataValue.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT Float64VectorMetadataValue : public AbstractVectorMetadataValue<float64>
{
public:
  using ParentType = AbstractVectorMetadataValue<float64>;
  using ValueType = typename ParentType::ValueType;
  using AssignmentReturnType = typename ParentType::AssignmentReturnType;

  static constexpr StringLiteral k_TypeName = "vec<float64>";

  Float64VectorMetadataValue();
  Float64VectorMetadataValue(const Float64VectorMetadataValue& rhs);
  Float64VectorMetadataValue(Float64VectorMetadataValue&& rhs) noexcept;
  ~Float64VectorMetadataValue() noexcept = default;

  Float64VectorMetadataValue& operator=(const Float64VectorMetadataValue& rhs) = default;
  Float64VectorMetadataValue& operator=(Float64VectorMetadataValue&& rhs) noexcept = default;

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
