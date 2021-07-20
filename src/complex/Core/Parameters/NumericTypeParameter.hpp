#pragma once

#include "complex/Common/Types.hpp"
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
  [[nodiscard]] ValueType defaultNumericType() const;

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
