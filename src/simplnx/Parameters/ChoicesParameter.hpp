#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>
#include <vector>

namespace nx::core
{
class SIMPLNX_EXPORT ChoicesParameter : public ValueParameter
{
public:
  using Choices = std::vector<std::string>;
  using ValueType = uint64;

  ChoicesParameter() = delete;
  ChoicesParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue, const Choices& choices);
  ~ChoicesParameter() override = default;

  ChoicesParameter(const ChoicesParameter&) = delete;
  ChoicesParameter(ChoicesParameter&&) noexcept = delete;

  ChoicesParameter& operator=(const ChoicesParameter&) = delete;
  ChoicesParameter& operator=(ChoicesParameter&&) noexcept = delete;

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
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief
   * @return
   */
  std::any defaultValue() const override;

  /**
   * @brief Returns version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the json format changes.
   * @return uint64
   */
  VersionType getVersion() const override;

  /**
   * @brief
   * @return
   */
  ValueType defaultIndex() const;

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
  Result<> validateIndex(ValueType index) const;

  /**
   * @brief
   * @return
   */
  Choices choices() const;

  /**
   * @brief Checks whether another parameter would be active when this parameter has the given value.
   * @param parameterValue The current value corresponding to this parameter.
   * @param associatedValue The value under which the grouped parameter is active.
   * @return
   */
  bool checkActive(const std::any& parameterValue, const std::any& associatedValue) const;

protected:
  /**
   * @brief
   * @param value
   */
  nlohmann::json toJsonImpl(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  Result<std::any> fromJsonImpl(const nlohmann::json& json, VersionType version) const override;

private:
  ValueType m_DefaultValue = {};
  Choices m_Choices = {};
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT LinkedChoicesFilterParameterConverter
{
  using ParameterType = ChoicesParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

struct SIMPLNX_EXPORT ChoiceFilterParameterConverter
{
  using ParameterType = ChoicesParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::ChoicesParameter, "ee4d5ce2-9582-48fa-b182-8a766ce0feff");
