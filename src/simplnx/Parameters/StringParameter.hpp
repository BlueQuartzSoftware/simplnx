#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
class SIMPLNX_EXPORT StringParameter : public ValueParameter
{
public:
  using ValueType = std::string;

  StringParameter() = delete;
  StringParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~StringParameter() override = default;

  StringParameter(const StringParameter&) = delete;
  StringParameter(StringParameter&&) noexcept = delete;

  StringParameter& operator=(const StringParameter&) = delete;
  StringParameter& operator=(StringParameter&&) noexcept = delete;

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
  ValueType defaultString() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

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
};

namespace SIMPLConversion
{
template <typename T>
struct NumberToStringFilterParameterConverter
{
  using ParameterType = StringParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

extern template struct NumberToStringFilterParameterConverter<int8>;
extern template struct NumberToStringFilterParameterConverter<uint8>;

extern template struct NumberToStringFilterParameterConverter<int16>;
extern template struct NumberToStringFilterParameterConverter<uint16>;

extern template struct NumberToStringFilterParameterConverter<int32>;
extern template struct NumberToStringFilterParameterConverter<uint32>;

extern template struct NumberToStringFilterParameterConverter<int64>;
extern template struct NumberToStringFilterParameterConverter<uint64>;

extern template struct NumberToStringFilterParameterConverter<float32>;
extern template struct NumberToStringFilterParameterConverter<float64>;

using DoubleToStringFilterParameterConverter = NumberToStringFilterParameterConverter<float64>;

struct SIMPLNX_EXPORT StringFilterParameterConverter
{
  using ParameterType = StringParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::StringParameter, "5d6d1868-05f8-11ec-9a03-0242ac130003");
