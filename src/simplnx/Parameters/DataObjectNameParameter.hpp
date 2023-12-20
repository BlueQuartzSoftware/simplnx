#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
class SIMPLNX_EXPORT DataObjectNameParameter : public ValueParameter
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
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::DataObjectNameParameter, "fbc89375-3ca4-4eb2-8257-aad9bf8e1c94");
