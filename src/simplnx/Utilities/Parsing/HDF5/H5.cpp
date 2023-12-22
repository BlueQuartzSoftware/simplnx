#include "H5.hpp"

#include <fmt/core.h>

#include <stdexcept>
#include <vector>

#include <H5Apublic.h>

std::optional<nx::core::DataType> nx::core::HDF5::toCommonType(Type typeEnum)
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

nx::core::HDF5::Type nx::core::HDF5::getTypeFromId(IdType typeId)
{
  if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    return Type::int8;
  }
  if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    return Type::int16;
  }
  if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    return Type::int32;
  }
  if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    return Type::int64;
  }
  if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    return Type::uint8;
  }
  if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    return Type::uint16;
  }
  if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    return Type::uint32;
  }
  if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    return Type::uint64;
  }
  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    return Type::float32;
  }
  if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    return Type::float64;
  }

  return Type::unknown;
}

nx::core::HDF5::IdType nx::core::HDF5::getIdForType(Type type)
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

std::string nx::core::HDF5::GetNameFromBuffer(std::string_view buffer)
{
  if(buffer.empty())
  {
    throw std::runtime_error(fmt::format("nx::core::HDF5::GetNameFromBuffer : HDF5 data path cannot be empty."));
  }

  size_t substrIndex = buffer.find_last_of('/');
  if(substrIndex == std::string::npos)
  {
    return std::string(buffer);
  }
  if(substrIndex == buffer.size() - 1)
  {
    if(buffer.size() == 1)
    {
      return std::string(buffer);
    }
    throw std::runtime_error(fmt::format("nx::core::HDF5::GetNameFromBuffer : Invalid HDF5 data path '{}'. Path cannot end with a /", buffer));
  }
  return std::string(buffer.substr(++substrIndex));
}

std::string nx::core::HDF5::GetPathFromId(IdType id)
{
  ssize_t nameLength = H5Iget_name(id, nullptr, 0);
  if(nameLength <= 0)
  {
    throw std::runtime_error("nx::core::HDF5::GetName failed: H5Iget_name call failed to get name length");
  }
  std::string buffer(nameLength, 'A');
  ssize_t size = H5Iget_name(id, buffer.data(), buffer.size() + 1);
  if(size <= 0)
  {
    throw std::runtime_error("nx::core::HDF5::GetName failed: H5Iget_name call failed to read buffer");
  }
  return buffer;
}

std::string nx::core::HDF5::GetNameFromId(IdType id)
{
  return GetNameFromBuffer(GetPathFromId(id));
}

std::string nx::core::HDF5::GetParentPath(const std::string& objectPath)
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
