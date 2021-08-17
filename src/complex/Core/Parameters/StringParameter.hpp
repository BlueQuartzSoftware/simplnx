#pragma once

#include <string>

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT StringParameter : public ValueParameter
{
public:
  using ValueType = std::string;

  StringParameter() = delete;
  StringParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~StringParameter() override = default;

  StringParameter(const StringParameter&) = delete;
  StringParameter(StringParameter&&) noexcept = delete;

  StringParameter& operator=(const StringParameter&) = delete;
  StringParameter& operator=(StringParameter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] AcceptedTypes acceptedTypes() const override;

  /**
   * @brief
   * @param value
   */
  [[nodiscard]] nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] UniquePointer clone() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::any defaultValue() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] ValueType defaultString() const;

  /**
   * @brief
   * @param value
   * @return
   */
  [[nodiscard]] Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::StringParameter, "ab047a7d-f81b-4e6f-99b5-610e7b69fc5b");
