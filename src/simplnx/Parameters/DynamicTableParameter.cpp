#include "DynamicTableParameter.hpp"

#include "simplnx/Common/Any.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace nx::core
{
DynamicTableParameter::DynamicTableParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const DynamicTableInfo& tableInfo)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_TableInfo(tableInfo)
{
  if(m_TableInfo.validate(m_DefaultValue).invalid())
  {
    throw std::runtime_error("DynamicTableParameter: The default value is invalid");
  }
}

DynamicTableParameter::DynamicTableParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const DynamicTableInfo& tableInfo)
: DynamicTableParameter(name, humanName, helpText, tableInfo.createDefault(), tableInfo)
{
}

Uuid DynamicTableParameter::uuid() const
{
  return ParameterTraits<DynamicTableParameter>::uuid;
}

IParameter::AcceptedTypes DynamicTableParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json DynamicTableParameter::toJson(const std::any& value) const
{
  const auto& table = GetAnyRef<ValueType>(value);
  return DynamicTableInfo::WriteData(table);
}

Result<std::any> DynamicTableParameter::fromJson(const nlohmann::json& json) const
{
  return {ConvertResultTo<std::any>(DynamicTableInfo::ReadData(json))};
}

IParameter::UniquePointer DynamicTableParameter::clone() const
{
  return std::make_unique<DynamicTableParameter>(name(), humanName(), helpText(), m_DefaultValue, m_TableInfo);
}

std::any DynamicTableParameter::defaultValue() const
{
  return defaultTable();
}

typename DynamicTableParameter::ValueType DynamicTableParameter::defaultTable() const
{
  return m_DefaultValue;
}

const DynamicTableInfo& DynamicTableParameter::tableInfo() const
{
  return m_TableInfo;
}

Result<> DynamicTableParameter::validate(const std::any& value) const
{
  const auto& table = GetAnyRef<ValueType>(value);
  return m_TableInfo.validate(table);
}
} // namespace nx::core
