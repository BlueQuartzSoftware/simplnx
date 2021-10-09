#include "H5.hpp"

#include <stdexcept>

#include <H5Apublic.h>

using namespace complex;

H5::Type H5::getTypeFromId(IdType typeId)
{
  if(H5Tequal(typeId, H5T_NATIVE_INT8))
  {
    return H5::Type::int8;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16))
  {
    return H5::Type::int16;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32))
  {
    return H5::Type::int32;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64))
  {
    return H5::Type::int64;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8))
  {
    return H5::Type::uint8;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16))
  {
    return H5::Type::uint16;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32))
  {
    return H5::Type::uint32;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64))
  {
    return H5::Type::uint64;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_FLOAT))
  {
    return H5::Type::float32;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE))
  {
    return H5::Type::float64;
  }

  return H5::Type::unknown;
}

H5::IdType H5::getIdForType(Type type)
{
  switch(type)
  {
  case Type::int8:
    return H5T_NATIVE_INT8;
  case Type::int16:
    return H5T_NATIVE_INT16;
  case Type::int32:
    return H5T_NATIVE_INT32;
  case Type::int64:
    return H5T_NATIVE_INT64;
  case Type::uint8:
    return H5T_NATIVE_UINT8;
  case Type::uint16:
    return H5T_NATIVE_UINT16;
  case Type::uint32:
    return H5T_NATIVE_UINT32;
  case Type::uint64:
    return H5T_NATIVE_UINT64;
  case Type::float32:
    return H5T_NATIVE_FLOAT;
  case Type::float64:
    return H5T_NATIVE_DOUBLE;
  default:
    throw std::runtime_error("H5::getIdForType does not support this type");
    return -1;
  }
}

std::string H5::GetNameFromBuffer(const char* buffer)
{
  std::string fullname(buffer);
  size_t substrIndex = fullname.find_last_of('/');
  if(substrIndex > 0)
  {
    substrIndex++;
  }
  return fullname.substr(substrIndex);
}
