#include "StringVectorMetadataValue.hpp"

namespace nx::core
{
StringVectorMetadataValue::StringVectorMetadataValue()
: ParentType()
{
}

StringVectorMetadataValue::StringVectorMetadataValue(const StringVectorMetadataValue& rhs)
: ParentType(rhs)
{
}
StringVectorMetadataValue::StringVectorMetadataValue(StringVectorMetadataValue&& rhs) noexcept
: ParentType(rhs)
{
}

std::string StringVectorMetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

StringVectorMetadataValue::AssignmentReturnType& StringVectorMetadataValue::operator=(const ValueType& rhs)
{
  return ParentType::operator=(rhs);
}

bool StringVectorMetadataValue::operator==(const ValueType& rhs) const
{
  return ParentType::operator==(rhs);
}
} // namespace nx::core
