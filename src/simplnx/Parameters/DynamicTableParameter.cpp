#include "DynamicTableParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

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
  auto result = m_TableInfo.validate(m_DefaultValue);
  if(result.invalid())
  {
    std::vector<std::string_view> errMsgs;
    std::transform(result.errors().cbegin(), result.errors().cend(), std::back_inserter(errMsgs), [](const Error& err) { return err.message; });
    std::string errMsgsStr = StringUtilities::join(errMsgs, "\n\n");
    throw std::runtime_error(fmt::format("DynamicTableParameter: The default value is invalid:\n\n{}", errMsgsStr));
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
