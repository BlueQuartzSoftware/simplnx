#pragma once

#include <filesystem>
#include <string>

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT InputFileParameter : public ValueParameter
{
public:
  using ValueType = std::filesystem::path;

  InputFileParameter() = delete;
  InputFileParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~InputFileParameter() override = default;

  InputFileParameter(const InputFileParameter&) = delete;
  InputFileParameter(InputFileParameter&&) noexcept = delete;

  InputFileParameter& operator=(const InputFileParameter&) = delete;
  InputFileParameter& operator=(InputFileParameter&&) noexcept = delete;

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
  [[nodiscard]] ValueType defaultPath() const;

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
  [[nodiscard]] Result<> validatePath(const ValueType& path) const;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::InputFileParameter, "f9a93f3d-21ef-43a1-a958-e57cbf3b2909");
