#pragma once

#include "complex/Filter/IParameter.hpp"

namespace complex
{
class COMPLEX_EXPORT AbstractParameter : public IParameter
{
public:
  ~AbstractParameter() noexcept override = default;

  AbstractParameter(const AbstractParameter& other) = delete;
  AbstractParameter(AbstractParameter&& other) noexcept = delete;

  AbstractParameter& operator=(const AbstractParameter& other) = delete;
  AbstractParameter& operator=(AbstractParameter&&) noexcept = delete;

  [[nodiscard]] std::string name() const final;

  [[nodiscard]] std::string humanName() const final;

  [[nodiscard]] std::string helpText() const final;

  [[nodiscard]] std::optional<std::any> defaultValue() const final;

protected:
  AbstractParameter() = delete;

  AbstractParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const std::optional<std::any>& defaultValue = {});

private:
  std::string m_Name;
  std::string m_HumanName;
  std::string m_HelpText;
  std::optional<std::any> m_DefaultValue;
};
} // namespace complex
