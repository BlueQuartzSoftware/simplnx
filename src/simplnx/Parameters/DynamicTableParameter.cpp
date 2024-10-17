#include "DynamicTableParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
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

//------------------------------------------------------------------------------
IParameter::VersionType DynamicTableParameter::getVersion() const
{
  return 1;
}

nlohmann::json DynamicTableParameter::toJsonImpl(const std::any& value) const
{
  const auto& table = GetAnyRef<ValueType>(value);
  return DynamicTableInfo::WriteData(table);
}

Result<std::any> DynamicTableParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
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

namespace SIMPLConversion
{
namespace
{
constexpr StringLiteral k_TableDataKey = "Table Data";
} // namespace

Result<DynamicTableFilterParameterConverter::ValueType> DynamicTableFilterParameterConverter::convert(const nlohmann::json& json)
{
  if(!json.contains(k_TableDataKey))
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("DynamicTableFilterParameter json '{}' does not contain '{}'", json.dump(), k_TableDataKey));
  }

  const auto& tableDataJson = json[k_TableDataKey];

  if(!tableDataJson.is_array())
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("DynamicTableFilterParameter '{}' value '{}' is not an array", k_TableDataKey, json.dump()));
  }

  DynamicTableInfo::TableDataType table;

  for(usize i = 0; i < tableDataJson.size(); i++)
  {
    const auto& jsonValue = tableDataJson.at(i);

    if(!jsonValue.is_array())
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("DynamicTableFilterParameter '{}' index {} with value '{}' is not an array", k_TableDataKey, i, jsonValue.dump()));
    }

    DynamicTableInfo::RowType row;

    for(usize j = 0; j < jsonValue.size(); j++)
    {
      const auto& elementValue = jsonValue.at(j);

      if(!elementValue.is_number())
      {
        return MakeErrorResult<ValueType>(-4, fmt::format("DynamicTableFilterParameter '{}' index ({}, {}) with value '{}' is not a number", k_TableDataKey, i, j, jsonValue.dump()));
      }

      auto value = elementValue.get<float64>();
      row.push_back(value);
    }

    table.push_back(row);
  }

  return {std::move(table)};
}

Result<ArrayToDynamicTableFilterParameterConverter::ValueType> ArrayToDynamicTableFilterParameterConverter::convert(const nlohmann::json& json)
{
  const auto& tableDataJson = json;
  if(!tableDataJson.is_array())
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("ArrayToDynamicTableFilterParameterConverter '{}' value is not an array", json.dump()));
  }

  DynamicTableInfo::TableDataType table;
  DynamicTableInfo::RowType row;

  for(usize j = 0; j < tableDataJson.size(); j++)
  {
    const auto& elementValue = tableDataJson.at(j);

    if(!elementValue.is_number())
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("ArrayToDynamicTableFilterParameterConverter index ({}, {}) with value '{}' is not a number", 0, j, tableDataJson.dump()));
    }

    auto value = elementValue.get<float64>();
    row.push_back(value);
  }

  table.push_back(row);

  return {std::move(table)};
}
} // namespace SIMPLConversion
} // namespace nx::core
