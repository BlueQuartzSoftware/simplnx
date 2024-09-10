
#include "DataTypeUtilities.hpp"

namespace nx::core
{
const std::set<DataType>& GetAllDataTypes()
{
  static const DataTypeSetType dataTypes = {nx::core::DataType::int8,    nx::core::DataType::uint8,   nx::core::DataType::int16,  nx::core::DataType::uint16,
                                            nx::core::DataType::int32,   nx::core::DataType::uint32,  nx::core::DataType::int64,  nx::core::DataType::uint64,
                                            nx::core::DataType::float32, nx::core::DataType::float64, nx::core::DataType::boolean};
  return dataTypes;
}

const std::set<DataType>& GetAllNumericTypes()
{
  static const DataTypeSetType dataTypes = {nx::core::DataType::int8,   nx::core::DataType::uint8, nx::core::DataType::int16,  nx::core::DataType::uint16,  nx::core::DataType::int32,
                                            nx::core::DataType::uint32, nx::core::DataType::int64, nx::core::DataType::uint64, nx::core::DataType::float32, nx::core::DataType::float64};
  return dataTypes;
}

const std::set<DataType>& GetIntegerDataTypes()
{
  static const DataTypeSetType dataTypes = {nx::core::DataType::int8,  nx::core::DataType::uint8,  nx::core::DataType::int16, nx::core::DataType::uint16,
                                            nx::core::DataType::int32, nx::core::DataType::uint32, nx::core::DataType::int64, nx::core::DataType::uint64};
  return dataTypes;
}

} // namespace nx::core
