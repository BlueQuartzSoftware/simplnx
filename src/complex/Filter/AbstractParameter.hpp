#pragma once

#include "complex/Filter/IParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief AbstractParameter stores name, human name, and help text for classes that want to inherit from IParameter.
 */
class COMPLEX_EXPORT AbstractParameter : public IParameter
{
public:
  ~AbstractParameter() noexcept override = default;

  AbstractParameter(const AbstractParameter& other) = delete;
  AbstractParameter(AbstractParameter&& other) noexcept = delete;

  AbstractParameter& operator=(const AbstractParameter& other) = delete;
  AbstractParameter& operator=(AbstractParameter&&) noexcept = delete;

  /**
   * @brief Returns the user defined name.
   * @return
   */
  std::string name() const final;

  /**
   * @brief Returns the user defined human readable name.
   * @return
   */
  std::string humanName() const final;

  /**
   * @brief Returns the user defined help text.
   * @return
   */
  std::string helpText() const final;

protected:
  AbstractParameter() = delete;

  AbstractParameter(const std::string& name, const std::string& humanName, const std::string& helpText);

private:
  std::string m_Name;
  std::string m_HumanName;
  std::string m_HelpText;
};
} // namespace complex
