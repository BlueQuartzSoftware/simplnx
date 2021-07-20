#pragma once

#include <string>
#include <vector>

#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT ChoicesParameter : public ValueParameter
{
public:
  using Choices = std::vector<std::string>;
  using ValueType = u64;

  ChoicesParameter() = delete;
  ChoicesParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue, const Choices& choices);
  ~ChoicesParameter() override = default;

  ChoicesParameter(const ChoicesParameter&) = delete;
  ChoicesParameter(ChoicesParameter&&) noexcept = delete;

  ChoicesParameter& operator=(const ChoicesParameter&) = delete;
  ChoicesParameter& operator=(ChoicesParameter&&) noexcept = delete;

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
  [[nodiscard]] ValueType defaultIndex() const;

  /**
   * @brief
   * @param value
   * @return
   */
  [[nodiscard]] Result<> validate(const std::any& value) const override;

  /**
   * @brief
   * @param value
   * @return
   */
  [[nodiscard]] Result<> validateIndex(ValueType index) const;

private:
  ValueType m_DefaultValue = {};
  Choices m_Choices = {};
};
} // namespace complex
