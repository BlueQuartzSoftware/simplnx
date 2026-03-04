#pragma once

#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief AbstractParameter stores name, human name, and help text for classes that want to inherit from IParameter.
 * Also provides the template-method implementations for toJson/fromJson/construct.
 */
class SIMPLNX_EXPORT AbstractParameter : public IParameter
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

  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief Constructs an input value from the given arguments.
   * By default, accesses a singular value by key and returns that.
   * May be overriden by subclasses that depend on other parameters.
   * @param args
   * @param executionContext
   * @return
   */
  std::any construct(const Arguments& args, const ExecutionContext& executionContext) const override;

protected:
  AbstractParameter() = delete;

  AbstractParameter(const std::string& name, const std::string& humanName, const std::string& helpText);

  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   */
  virtual nlohmann::json toJsonImpl(const std::any& value) const = 0;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return
   */
  virtual Result<std::any> fromJsonImpl(const nlohmann::json& json, uint64 version) const = 0;

private:
  std::string m_Name;
  std::string m_HumanName;
  std::string m_HelpText;
};
} // namespace nx::core
