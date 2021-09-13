#pragma once

#include "complex/Core/Parameters/utils/StackFileListInfo.hpp"
#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace complex
{
class COMPLEX_EXPORT StackFileListInfoParameter : public ValueParameter
{
public:
  using ValueType = complex::StackFileListInfo;

  StackFileListInfoParameter() = delete;
  /**
   *
   * @param name
   * @param humanName
   * @param helpText
   * @param defaultValue
   */
  StackFileListInfoParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);

  ~StackFileListInfoParameter() override = default;

  StackFileListInfoParameter(const StackFileListInfoParameter&) = delete;
  StackFileListInfoParameter(StackFileListInfoParameter&&) noexcept = delete;

  StackFileListInfoParameter& operator=(const StackFileListInfoParameter&) = delete;
  StackFileListInfoParameter& operator=(StackFileListInfoParameter&&) noexcept = delete;

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
   * @param value
   * @return
   */
  [[nodiscard]] Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::StackFileListInfoParameter, "aac15aa6-b367-508e-bf73-94ab6be0058b");
