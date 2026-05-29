#include "Float64VectorMetadataValue.hpp"

namespace nx::core
{
Float64VectorMetadataValue::Float64VectorMetadataValue()
: ParentType()
{
}

Float64VectorMetadataValue::Float64VectorMetadataValue(const Float64VectorMetadataValue& rhs)
: ParentType(rhs)
{
}
Float64VectorMetadataValue::Float64VectorMetadataValue(Float64VectorMetadataValue&& rhs) noexcept
: ParentType(rhs)
{
}

std::string Float64VectorMetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

Float64VectorMetadataValue::AssignmentReturnType& Float64VectorMetadataValue::operator=(const ValueType& rhs)
{
  return ParentType::operator=(rhs);
}

bool Float64VectorMetadataValue::operator==(const ValueType& rhs) const
{
  return ParentType::operator==(rhs);
}
} // namespace nx::core
