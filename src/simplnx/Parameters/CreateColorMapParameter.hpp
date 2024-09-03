#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace nx::core
{
/**
 * @brief This FilterParameter represents a color preset, and works specifically
 * with the CreateColorMap filter. The data is held in an nlohmann::json object.
 */
class SIMPLNX_EXPORT CreateColorMapParameter : public ValueParameter
{
public:
  using ValueType = std::string;

  CreateColorMapParameter() = delete;
  CreateColorMapParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~CreateColorMapParameter() override = default;

  CreateColorMapParameter(const CreateColorMapParameter&) = delete;
  CreateColorMapParameter(CreateColorMapParameter&&) noexcept = delete;

  CreateColorMapParameter& operator=(const CreateColorMapParameter&) = delete;
  CreateColorMapParameter& operator=(CreateColorMapParameter&&) noexcept = delete;

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
  ValueType defaultJson() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT CreateColorMapFilterParameterConverter
{
  using ParameterType = CreateColorMapParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::CreateColorMapParameter, "7b0e5b25-564e-4797-b154-4324ef276bf0");
