#include "DoubleVectorMetadataValue.hpp"

namespace nx::core
{
DoubleVectorMetadataValue::DoubleVectorMetadataValue()
: ParentType()
{
}

DoubleVectorMetadataValue::DoubleVectorMetadataValue(const DoubleVectorMetadataValue& rhs)
: ParentType(rhs)
{
}
DoubleVectorMetadataValue::DoubleVectorMetadataValue(DoubleVectorMetadataValue&& rhs) noexcept
: ParentType(rhs)
{
}

std::string DoubleVectorMetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

DoubleVectorMetadataValue::AssignmentReturnType& DoubleVectorMetadataValue::operator=(const ValueType& rhs)
{
  return ParentType::operator=(rhs);
}

bool DoubleVectorMetadataValue::operator==(const ValueType& rhs) const
{
  return ParentType::operator==(rhs);
}
} // namespace nx::core
