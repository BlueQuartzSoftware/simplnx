#include "BoolVectorMetadataValue.hpp"

namespace nx::core
{
BoolVectorMetadataValue::BoolVectorMetadataValue()
: ParentType()
{
}

BoolVectorMetadataValue::BoolVectorMetadataValue(const BoolVectorMetadataValue& rhs)
: ParentType(rhs)
{
}
BoolVectorMetadataValue::BoolVectorMetadataValue(BoolVectorMetadataValue&& rhs) noexcept
: ParentType(rhs)
{
}

BoolVectorMetadataValue& BoolVectorMetadataValue::operator=(const BoolVectorMetadataValue& rhs)
{
  setValue(rhs.getValue());
  return *this;
}
BoolVectorMetadataValue& BoolVectorMetadataValue::operator=(BoolVectorMetadataValue&& rhs) noexcept
{
  ParentType::operator=(rhs);
  return *this;
}

std::string BoolVectorMetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

BoolVectorMetadataValue::AssignmentReturnType& BoolVectorMetadataValue::operator=(const ValueType& rhs)
{
  return ParentType::operator=(rhs);
}

bool BoolVectorMetadataValue::operator==(const ValueType& rhs) const
{
  return ParentType::operator==(rhs);
}
} // namespace nx::core
