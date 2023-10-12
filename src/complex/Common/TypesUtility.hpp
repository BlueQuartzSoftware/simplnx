#pragma once

#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Common/Types.hpp"

#include <optional>
#include <stdexcept>
#include <vector>

namespace complex
{
/**
 * @brief Returns the NumericType associated with T.
 * @tparam T
 * @return
 */
template <class T>
constexpr NumericType GetNumericType() noexcept
{
  if constexpr(std::is_same_v<T, int8>)
  {
    return NumericType::int8;
  }
  else if constexpr(std::is_same_v<T, uint8>)
  {
    return NumericType::uint8;
  }
  else if constexpr(std::is_same_v<T, int16>)
  {
    return NumericType::int16;
  }
  else if constexpr(std::is_same_v<T, uint16>)
  {
    return NumericType::uint16;
  }
  else if constexpr(std::is_same_v<T, int32>)
  {
    return NumericType::int32;
  }
  else if constexpr(std::is_same_v<T, uint32>)
  {
    return NumericType::uint32;
  }
  else if constexpr(std::is_same_v<T, int64>)
  {
    return NumericType::int64;
  }
  else if constexpr(std::is_same_v<T, uint64>)
  {
    return NumericType::uint64;
  }
  else if constexpr(std::is_same_v<T, float32>)
  {
    return NumericType::float32;
  }
  else if constexpr(std::is_same_v<T, float64>)
  {
    return NumericType::float64;
  }
  else
  {
    static_assert(dependent_false<T>, "complex::GetNumericType: Unsupported type");
  }
}

/**
 * @brief Returns the DataType associated with T.
 * @tparam T
 * @return
 */
template <class T>
constexpr DataType GetDataType() noexcept
{
  if constexpr(std::is_same_v<T, int8>)
  {
    return DataType::int8;
  }
  else if constexpr(std::is_same_v<T, uint8>)
  {
    return DataType::uint8;
  }
  else if constexpr(std::is_same_v<T, int16>)
  {
    return DataType::int16;
  }
  else if constexpr(std::is_same_v<T, uint16>)
  {
    return DataType::uint16;
  }
  else if constexpr(std::is_same_v<T, int32>)
  {
    return DataType::int32;
  }
  else if constexpr(std::is_same_v<T, uint32>)
  {
    return DataType::uint32;
  }
  else if constexpr(std::is_same_v<T, int64>)
  {
    return DataType::int64;
  }
  else if constexpr(std::is_same_v<T, uint64>)
  {
    return DataType::uint64;
  }
  else if constexpr(std::is_same_v<T, float32>)
  {
    return DataType::float32;
  }
  else if constexpr(std::is_same_v<T, float64>)
  {
    return DataType::float64;
  }
  else if constexpr(std::is_same_v<T, bool>)
  {
    return DataType::boolean;
  }
  else
  {
    static_assert(dependent_false<T>, "complex::GetDataType: Unsupported type");
  }
}

/**
 * @brief Returns sizeof(T) for the T associated with the given NumericType.
 * @param numericType
 * @return
 */
inline constexpr usize GetNumericTypeSize(NumericType numericType)
{
  switch(numericType)
  {
  case NumericType::int8:
    return sizeof(int8);
  case NumericType::uint8:
    return sizeof(uint8);
  case NumericType::int16:
    return sizeof(int16);
  case NumericType::uint16:
    return sizeof(uint16);
  case NumericType::int32:
    return sizeof(int32);
  case NumericType::uint32:
    return sizeof(uint32);
  case NumericType::int64:
    return sizeof(int64);
  case NumericType::uint64:
    return sizeof(uint64);
  case NumericType::float32:
    return sizeof(float32);
  case NumericType::float64:
    return sizeof(float64);
  default:
    throw std::runtime_error("complex::GetNumericTypeSize: Unsupported type");
  }
}

/**
 * @brief Returns a string representation of the passed in NumericType
 * @param numericType
 * @return
 */
inline constexpr StringLiteral NumericTypeToString(NumericType numericType)
{
  switch(numericType)
  {
  case NumericType::int8: {
    return "int8";
  }
  case NumericType::uint8: {
    return "uint8";
  }
  case NumericType::int16: {
    return "int16";
  }
  case NumericType::uint16: {
    return "uint16";
  }
  case NumericType::int32: {
    return "int32";
  }
  case NumericType::uint32: {
    return "uint32";
  }
  case NumericType::int64: {
    return "int64";
  }
  case NumericType::uint64: {
    return "uint64";
  }
  case NumericType::float32: {
    return "float32";
  }
  case NumericType::float64: {
    return "float64";
  }
  default:
    throw std::runtime_error("complex::NumericTypeToString: Unknown NumericType");
  }
}

/**
 * @brief Returns a string representation of the passed in DataType
 * @param dataType
 * @return
 */
inline constexpr StringLiteral DataTypeToString(DataType dataType)
{
  switch(dataType)
  {
  case DataType::int8: {
    return "int8";
  }
  case DataType::uint8: {
    return "uint8";
  }
  case DataType::int16: {
    return "int16";
  }
  case DataType::uint16: {
    return "uint16";
  }
  case DataType::int32: {
    return "int32";
  }
  case DataType::uint32: {
    return "uint32";
  }
  case DataType::int64: {
    return "int64";
  }
  case DataType::uint64: {
    return "uint64";
  }
  case DataType::float32: {
    return "float32";
  }
  case DataType::float64: {
    return "float64";
  }
  case DataType::boolean: {
    return "boolean";
  }
  default:
    throw std::runtime_error("complex::DataTypeToString: Unknown DataType");
  }
}

/**
 * @brief Returns string representations for all DataTypes.
 * @return
 */
inline const std::vector<std::string>& GetAllDataTypesAsStrings()
{
  static const std::vector<std::string> dataTypes = {DataTypeToString(complex::DataType::int8),    DataTypeToString(complex::DataType::uint8),  DataTypeToString(complex::DataType::int16),
                                                     DataTypeToString(complex::DataType::uint16),  DataTypeToString(complex::DataType::int32),  DataTypeToString(complex::DataType::uint32),
                                                     DataTypeToString(complex::DataType::int64),   DataTypeToString(complex::DataType::uint64), DataTypeToString(complex::DataType::float32),
                                                     DataTypeToString(complex::DataType::float64), DataTypeToString(complex::DataType::boolean)};
  return dataTypes;
}

inline constexpr StringLiteral DataTypeToHumanString(DataType dataType)
{
  switch(dataType)
  {
  case DataType::int8: {
    return "signed int 8 bit";
  }
  case DataType::uint8: {
    return "unsigned int 8 bit";
  }
  case DataType::int16: {
    return "signed int 16 bit";
  }
  case DataType::uint16: {
    return "unsigned int 16 bit";
  }
  case DataType::int32: {
    return "signed int 32 bit";
  }
  case DataType::uint32: {
    return "unsigned int 32 bit";
  }
  case DataType::int64: {
    return "signed int 64 bit";
  }
  case DataType::uint64: {
    return "unsigned int 64 bit";
  }
  case DataType::float32: {
    return "float 32";
  }
  case DataType::float64: {
    return "double 64";
  }
  case DataType::boolean: {
    return "boolean";
  }
  default:
    throw std::runtime_error("complex::DataTypeToString: Unknown DataType");
  }
}

/**
 *
 * @param humanReadable Strings that would be good for a User interface
 * @return
 */
inline const std::vector<std::string>& GetAllDataTypesAsHumanStrings()
{
  static const std::vector<std::string> dataTypes = {
      DataTypeToHumanString(complex::DataType::int8),    DataTypeToHumanString(complex::DataType::uint8),  DataTypeToHumanString(complex::DataType::int16),
      DataTypeToHumanString(complex::DataType::uint16),  DataTypeToHumanString(complex::DataType::int32),  DataTypeToHumanString(complex::DataType::uint32),
      DataTypeToHumanString(complex::DataType::int64),   DataTypeToHumanString(complex::DataType::uint64), DataTypeToHumanString(complex::DataType::float32),
      DataTypeToHumanString(complex::DataType::float64), DataTypeToHumanString(complex::DataType::boolean)};
  return dataTypes;
}

/**
 * @brief Returns a DataType for the passed in string representation
 * @param dataTypeString
 * @return
 */
inline constexpr DataType StringToDataType(std::string_view dataTypeString)
{
  if(dataTypeString == DataTypeToString(DataType::int8).view())
  {
    return DataType::int8;
  }
  else if(dataTypeString == DataTypeToString(DataType::uint8).view())
  {
    return DataType::uint8;
  }
  else if(dataTypeString == DataTypeToString(DataType::int16).view())
  {
    return DataType::int16;
  }
  else if(dataTypeString == DataTypeToString(DataType::uint16).view())
  {
    return DataType::uint16;
  }
  else if(dataTypeString == DataTypeToString(DataType::int32).view())
  {
    return DataType::int32;
  }
  else if(dataTypeString == DataTypeToString(DataType::uint32).view())
  {
    return DataType::uint32;
  }
  else if(dataTypeString == DataTypeToString(DataType::int64).view())
  {
    return DataType::int64;
  }
  else if(dataTypeString == DataTypeToString(DataType::uint64).view())
  {
    return DataType::uint64;
  }
  else if(dataTypeString == DataTypeToString(DataType::float32).view())
  {
    return DataType::float32;
  }
  else if(dataTypeString == DataTypeToString(DataType::float64).view())
  {
    return DataType::float64;
  }
  else if(dataTypeString == DataTypeToString(DataType::boolean).view())
  {
    return DataType::boolean;
  }
  else
  {
    throw std::runtime_error("complex::StringToDataType: No known DataType matches the given string value.");
  }
}

/**
 * @brief Converts DataType to NumericType. Fails on DataType::bool and DataType::error.
 * @param dataType
 * @return
 */
inline constexpr std::optional<NumericType> ConvertDataTypeToNumericType(DataType dataType) noexcept
{
  switch(dataType)
  {
  case DataType::int8: {
    return NumericType::int8;
  }
  case DataType::uint8: {
    return NumericType::uint8;
  }
  case DataType::int16: {
    return NumericType::int16;
  }
  case DataType::uint16: {
    return NumericType::uint16;
  }
  case DataType::int32: {
    return NumericType::int32;
  }
  case DataType::uint32: {
    return NumericType::uint32;
  }
  case DataType::int64: {
    return NumericType::int64;
  }
  case DataType::uint64: {
    return NumericType::uint64;
  }
  case DataType::float32: {
    return NumericType::float32;
  }
  case DataType::float64: {
    return NumericType::float64;
  }
  default: {
    return {};
  }
  }
}

/**
 * @brief Converts NumericType to DataType. NumericType is a subset of DataType so this function cannot fail.
 * @param numericType
 * @return
 */
inline constexpr DataType ConvertNumericTypeToDataType(NumericType numericType)
{
  switch(numericType)
  {
  case NumericType::int8: {
    return DataType::int8;
  }
  case NumericType::int16: {
    return DataType::int16;
  }
  case NumericType::int32: {
    return DataType::int32;
  }
  case NumericType::int64: {
    return DataType::int64;
  }
  case NumericType::uint8: {
    return DataType::uint8;
  }
  case NumericType::uint16: {
    return DataType::uint16;
  }
  case NumericType::uint32: {
    return DataType::uint32;
  }
  case NumericType::uint64: {
    return DataType::uint64;
  }
  case NumericType::float32: {
    return DataType::float32;
  }
  case NumericType::float64: {
    return DataType::float64;
  }
  default: {
    throw std::runtime_error("complex::ConvertNumericTypeToDataType: Invalid NumericType");
  }
  }
}
} // namespace complex
