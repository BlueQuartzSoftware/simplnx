#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT BoolParameter : public ValueParameter
{
public:
  using ValueType = bool;

  BoolParameter() = delete;
  BoolParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue);
  ~BoolParameter() override = default;

  BoolParameter(const BoolParameter&) = delete;
  BoolParameter(BoolParameter&&) noexcept = delete;

  BoolParameter& operator=(const BoolParameter&) = delete;
  BoolParameter& operator=(BoolParameter&&) noexcept = delete;

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
  ValueType defaultBool() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

  /**
   * @brief Checks whether another parameter would be active when this parameter has the given value.
   * @param parameterValue The current value corresponding to this parameter.
   * @param associatedValue The value under which the grouped parameter is active.
   * @return
   */
  bool checkActive(const std::any& parameterValue, const std::any& associatedValue) const;

private:
  ValueType m_DefaultValue = {};
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT BooleanFilterParameterConverter
{
  using ParameterType = BoolParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

struct SIMPLNX_EXPORT InvertedBooleanFilterParameterConverter
{
  using ParameterType = BoolParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

struct SIMPLNX_EXPORT LinkedBooleanFilterParameterConverter
{
  using ParameterType = BoolParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::BoolParameter, "b6936d18-7476-4855-9e13-e795d717c50f");
