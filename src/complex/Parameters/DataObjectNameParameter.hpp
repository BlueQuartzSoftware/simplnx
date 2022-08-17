#pragma once

#include <string>

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT DataObjectNameParameter : public ValueParameter
{
public:
  using ValueType = std::string;

  DataObjectNameParameter() = delete;
  DataObjectNameParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~DataObjectNameParameter() override = default;

  DataObjectNameParameter(const DataObjectNameParameter&) = delete;
  DataObjectNameParameter(DataObjectNameParameter&&) noexcept = delete;

  DataObjectNameParameter& operator=(const DataObjectNameParameter&) = delete;
  DataObjectNameParameter& operator=(DataObjectNameParameter&&) noexcept = delete;

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
  ValueType defaultName() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validateName(const std::string& value) const;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::DataObjectNameParameter, "fbc89375-3ca4-4eb2-8257-aad9bf8e1c94");
