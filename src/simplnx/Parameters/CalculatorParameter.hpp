#pragma once

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/MutableDataParameter.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
class SIMPLNX_EXPORT CalculatorParameter : public MutableDataParameter
{
public:
  enum AngleUnits : uint8
  {
    Radians = 0,
    Degrees = 1
  };

  struct ValueType
  {
    DataPath m_SelectedGroup;
    std::string m_Equation = "";
    AngleUnits m_Units = Radians;
  };

  CalculatorParameter() = delete;
  CalculatorParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~CalculatorParameter() override = default;

  CalculatorParameter(const CalculatorParameter&) = delete;
  CalculatorParameter(CalculatorParameter&&) noexcept = delete;

  CalculatorParameter& operator=(const CalculatorParameter&) = delete;
  CalculatorParameter& operator=(CalculatorParameter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief
   * @param value
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief
   * @return
   */
  std::any defaultValue() const override;

  /**
   * @brief
   * @return
   */
  ValueType defaultString() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const DataStructure& dataStructure, const std::any& value) const override;

  /**
   * @brief Takes the value and a mutable DataStructure and attempts store the actual derived DataObject in the std::any.
   * Returns any warnings/errors.
   * @param dataStructure
   * @param value
   * @return
   */
  Result<std::any> resolve(DataStructure& dataStructure, const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT CalculatorFilterParameterConverter
{
  using ParameterType = CalculatorParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::CalculatorParameter, "ba2d4937-dbec-5536-8c5c-c0a406e80f77");
