#pragma once

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/Parameters/util/DynamicTableInfo.hpp"
#include "complex/complex_export.hpp"

#include <string>

namespace complex
{
/**
 * @class DynamicTableParameter
 * @brief
 */
class COMPLEX_EXPORT DynamicTableParameter : public ValueParameter
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
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   * @return nlohmann::json
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return Result<std::any>
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

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

private:
  ValueType m_DefaultValue = {};
  DynamicTableInfo m_TableInfo;
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::DynamicTableParameter, "eea76f1a-fab9-4704-8da5-4c21057cf44e");
