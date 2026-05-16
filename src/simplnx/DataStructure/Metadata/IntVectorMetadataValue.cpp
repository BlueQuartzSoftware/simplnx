#include "IntVectorMetadataValue.hpp"

namespace nx::core
{
Int32VectorMetadataValue::Int32VectorMetadataValue()
: ParentType()
{
}

Int32VectorMetadataValue::Int32VectorMetadataValue(const Int32VectorMetadataValue& rhs)
: ParentType(rhs)
{
}
Int32VectorMetadataValue::Int32VectorMetadataValue(Int32VectorMetadataValue&& rhs) noexcept
: ParentType(rhs)
{
}

std::string Int32VectorMetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

Int32VectorMetadataValue::AssignmentReturnType& Int32VectorMetadataValue::operator=(const ValueType& rhs)
{
  return ParentType::operator=(rhs);
}

bool Int32VectorMetadataValue::operator==(const ValueType& rhs) const
{
  return ParentType::operator==(rhs);
}
} // namespace nx::core
