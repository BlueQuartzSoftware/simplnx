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

  /**
   * @brief Returns the name of the Parameter.
   * @return
   */
  [[nodiscard]] std::string name() const final;

  /**
   * @brief Returns a string containing a string label or short description that
   * might appear in a user interface.
   * @return
   */
  [[nodiscard]] std::string humanName() const final;

  /**
   * @brief Returns a longer string that describes the parameter. This could be used
   * in a tooltip or pop-up help in a user interface.
   * @return
   */
  [[nodiscard]] std::string helpText() const final;

protected:
  AbstractParameter() = delete;

  AbstractParameter(const std::string& name, const std::string& humanName, const std::string& helpText);

private:
  std::string m_Name;
  std::string m_HumanName;
  std::string m_HelpText;
};
} // namespace complex
