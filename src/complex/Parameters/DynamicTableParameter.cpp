#include "DynamicTableParameter.hpp"

#include "complex/DataStructure/DataGroup.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

namespace complex
{
DynamicTableParameter::DynamicTableParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
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
  nlohmann::json json = nlohmann::json::object();
  table.writeJson(json);
  return json;
}

Result<std::any> DynamicTableParameter::fromJson(const nlohmann::json& json) const
{
  ValueType table;
  table.readJson(json);
  return {{table}};
}

IParameter::UniquePointer DynamicTableParameter::clone() const
{
  return std::make_unique<DynamicTableParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any DynamicTableParameter::defaultValue() const
{
  return defaultTable();
}

typename DynamicTableParameter::ValueType DynamicTableParameter::defaultTable() const
{
  return m_DefaultValue;
}

Result<> DynamicTableParameter::validate(const std::any& value) const
{
  if(value.type() == typeid(DynamicTableData))
  {
    return {};
  }

  return {nonstd::make_unexpected(std::vector<Error>{Error{-667, "Bad parameter type"}})};
}
} // namespace complex
