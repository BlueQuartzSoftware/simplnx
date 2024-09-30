#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/Parameters/util/DynamicTableInfo.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
/**
 * @class DynamicTableParameter
 * @brief
 */
class SIMPLNX_EXPORT DynamicTableParameter : public ValueParameter
{
public:
  using ValueType = DynamicTableInfo::TableDataType;

  DynamicTableParameter() = delete;
  DynamicTableParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const DynamicTableInfo& tableInfo);
  DynamicTableParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const DynamicTableInfo& tableInfo);
  ~DynamicTableParameter() override = default;

  DynamicTableParameter(const DynamicTableParameter&) = delete;
  DynamicTableParameter(DynamicTableParameter&&) noexcept = delete;

  DynamicTableParameter& operator=(const DynamicTableParameter&) = delete;
  DynamicTableParameter& operator=(DynamicTableParameter&&) noexcept = delete;

  /**
   * @brief Returns the parameter's uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns a list of accpeted input types.
   * @return AcceptedTypes
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief Creates a copy of the parameter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns the user defined default value.
   * @return std::any
   */
  std::any defaultValue() const override;

  /**
   * @brief Returns version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the json format changes.
   * @return uint64
   */
  VersionType getVersion() const override;

  /**
   * @brief Returns the user defined default table.
   * @return ValueType
   */
  ValueType defaultTable() const;

  /**
   * @brief Returns the table info.
   * @return
   */
  const DynamicTableInfo& tableInfo() const;

  /**
   * @brief Validates the given value. Returns warnings/errors.
   * @param value
   * @return Result<>
   */
  Result<> validate(const std::any& value) const override;

protected:
  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   * @return nlohmann::json
   */
  nlohmann::json toJsonImpl(const std::any& value) const override;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return Result<std::any>
   */
  Result<std::any> fromJsonImpl(const nlohmann::json& json, VersionType version) const override;

private:
  ValueType m_DefaultValue = {};
  DynamicTableInfo m_TableInfo;
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT DynamicTableFilterParameterConverter
{
  using ParameterType = DynamicTableParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

struct SIMPLNX_EXPORT ArrayToDynamicTableFilterParameterConverter
{
  using ParameterType = DynamicTableParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::DynamicTableParameter, "eea76f1a-fab9-4704-8da5-4c21057cf44e");
