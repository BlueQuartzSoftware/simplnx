#include "H5.hpp"

#include <stdexcept>
#include <vector>

#include <H5Apublic.h>

std::optional<complex::DataType> complex::HDF5::toCommonType(Type typeEnum)
{
  switch(typeEnum)
  {
  case Type::int8:
    return DataType::int8;
  case Type::int16:
    return DataType::int16;
  case Type::int32:
    return DataType::int32;
  case Type::int64:
    return DataType::int64;
  case Type::uint8:
    return DataType::uint8;
  case Type::uint16:
    return DataType::uint16;
  case Type::uint32:
    return DataType::uint32;
  case Type::uint64:
    return DataType::uint64;
  case Type::float32:
    return DataType::float32;
  case Type::float64:
    return DataType::float64;
  case Type::string:
    [[fallthrough]];
  case Type::unknown:
    [[fallthrough]];
  default:
    return {};
  }
}

complex::HDF5::Type complex::HDF5::getTypeFromId(IdType typeId)
{
  if(H5Tequal(typeId, H5T_NATIVE_INT8))
  {
    return Type::int8;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16))
  {
    return Type::int16;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32))
  {
    return Type::int32;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64))
  {
    return Type::int64;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8))
  {
    return Type::uint8;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16))
  {
    return Type::uint16;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32))
  {
    return Type::uint32;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64))
  {
    return Type::uint64;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_FLOAT))
  {
    return Type::float32;
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE))
  {
    return Type::float64;
  }

  return Type::unknown;
}

complex::HDF5::IdType complex::HDF5::getIdForType(Type type)
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
    throw std::runtime_error("getIdForType does not support this type");
  }
}

std::string complex::HDF5::GetNameFromBuffer(std::string_view buffer)
{
  size_t substrIndex = buffer.find_last_of('/');
  if(substrIndex > 0)
  {
    substrIndex++;
  }
  return std::string(buffer.substr(substrIndex));
}

std::string complex::HDF5::GetPathFromId(IdType id)
{
  ssize_t nameLength = H5Iget_name(id, nullptr, 0);
  if(nameLength <= 0)
  {
    throw std::runtime_error("complex::HDF5::GetName failed: H5Iget_name call failed to get name length");
  }
  std::string buffer(nameLength, 'A');
  ssize_t size = H5Iget_name(id, buffer.data(), buffer.size() + 1);
  if(size <= 0)
  {
    throw std::runtime_error("complex::HDF5::GetName failed: H5Iget_name call failed to read buffer");
  }
  return buffer;
}

std::string complex::HDF5::GetNameFromId(IdType id)
{
  return GetNameFromBuffer(GetPathFromId(id));
}

std::string complex::HDF5::GetParentPath(const std::string& objectPath)
{
  if(objectPath.empty())
  {
    return "";
  }
  auto back = objectPath.find_last_not_of("/");
  if(back == std::string::npos)
  {
    return "";
  }
  return objectPath.substr(0, ++back);
}
