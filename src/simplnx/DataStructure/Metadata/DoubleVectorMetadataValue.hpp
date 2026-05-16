#pragma once

#include "AbstractVectorMetadataValue.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT DoubleVectorMetadataValue : public AbstractVectorMetadataValue<float64>
{
public:
  using ParentType = AbstractVectorMetadataValue<float64>;
  using ValueType = ParentType::ValueType;
  using AssignmentReturnType = ParentType::AssignmentReturnType;

  static constexpr StringLiteral k_TypeName = "vec<float64>";

  DoubleVectorMetadataValue();
  DoubleVectorMetadataValue(const DoubleVectorMetadataValue& rhs);
  DoubleVectorMetadataValue(DoubleVectorMetadataValue&& rhs) noexcept;
  ~DoubleVectorMetadataValue() noexcept = default;

  DoubleVectorMetadataValue& operator=(const DoubleVectorMetadataValue& rhs) = default;
  DoubleVectorMetadataValue& operator=(DoubleVectorMetadataValue&& rhs) noexcept = default;

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
