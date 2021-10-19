#pragma once

#include "complex/Common/Types.hpp"
#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT NumericTypeParameter : public ValueParameter
{
public:
  using ValueType = NumericType;

  NumericTypeParameter() = delete;
  NumericTypeParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue);
  ~NumericTypeParameter() override = default;

  NumericTypeParameter(const NumericTypeParameter&) = delete;
  NumericTypeParameter(NumericTypeParameter&&) noexcept = delete;

  NumericTypeParameter& operator=(const NumericTypeParameter&) = delete;
  NumericTypeParameter& operator=(NumericTypeParameter&&) noexcept = delete;

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
  ValueType defaultNumericType() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::NumericTypeParameter, "a8ff9dbd-45e7-4ed6-8537-12dd53069bce");
