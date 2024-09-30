#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT DataTypeParameter : public ValueParameter
{
public:
  using ValueType = DataType;

  DataTypeParameter() = delete;
  DataTypeParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue);
  ~DataTypeParameter() override = default;

  DataTypeParameter(const DataTypeParameter&) = delete;
  DataTypeParameter(DataTypeParameter&&) noexcept = delete;

  DataTypeParameter& operator=(const DataTypeParameter&) = delete;
  DataTypeParameter& operator=(DataTypeParameter&&) noexcept = delete;

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
  ValueType defaultDataType() const;

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
struct SIMPLNX_EXPORT ScalarTypeParameterConverter
{
  using ParameterType = DataTypeParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::DataTypeParameter, "d31358d5-3253-4c69-aff0-eb98618f851b");
