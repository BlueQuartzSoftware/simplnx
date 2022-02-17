#pragma once

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/Parameters/util/DynamicTableData.hpp"
#include "complex/complex_export.hpp"

#include <string>

namespace complex
{
/**
 * @class CalculatorParameter
 * @brief
 */
class COMPLEX_EXPORT CalculatorParameter : public ValueParameter
{
public:
  using ValueType = std::string;

  CalculatorParameter() = delete;
  CalculatorParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~CalculatorParameter() override = default;

  CalculatorParameter(const CalculatorParameter&) = delete;
  CalculatorParameter(CalculatorParameter&&) noexcept = delete;

  CalculatorParameter& operator=(const CalculatorParameter&) = delete;
  CalculatorParameter& operator=(CalculatorParameter&&) noexcept = delete;

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
   * @brief Validates the given value. Returns warnings/errors.
   * @param value
   * @return Result<>
   */
  Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::CalculatorParameter, "7b480491-da09-4aad-94b4-7d7ba51ec660");
