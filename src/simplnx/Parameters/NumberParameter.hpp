#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"

namespace nx::core
{
template <class T>
class NumberParameter : public ValueParameter
{
public:
  using ValueType = T;

  NumberParameter() = delete;
  NumberParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue);
  ~NumberParameter() override = default;

  NumberParameter(const NumberParameter&) = delete;
  NumberParameter(NumberParameter&&) noexcept = delete;

  NumberParameter& operator=(const NumberParameter&) = delete;
  NumberParameter& operator=(NumberParameter&&) noexcept = delete;

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
  ValueType defaultNumber() const;

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
  Result<> validateNumber(ValueType value) const;

private:
  ValueType m_DefaultValue = {};
};

extern template class NumberParameter<int8>;
extern template class NumberParameter<uint8>;

extern template class NumberParameter<int16>;
extern template class NumberParameter<uint16>;

extern template class NumberParameter<int32>;
extern template class NumberParameter<uint32>;

extern template class NumberParameter<int64>;
extern template class NumberParameter<uint64>;

extern template class NumberParameter<float32>;
extern template class NumberParameter<float64>;

using Int8Parameter = NumberParameter<int8>;
using UInt8Parameter = NumberParameter<uint8>;

using Int16Parameter = NumberParameter<int16>;
using UInt16Parameter = NumberParameter<uint16>;

using Int32Parameter = NumberParameter<int32>;
using UInt32Parameter = NumberParameter<uint32>;

using Int64Parameter = NumberParameter<int64>;
using UInt64Parameter = NumberParameter<uint64>;

using Float32Parameter = NumberParameter<float32>;
using Float64Parameter = NumberParameter<float64>;

namespace SIMPLConversion
{
template <typename T>
struct IntFilterParameterConverter
{
  using ParameterType = NumberParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

extern template struct IntFilterParameterConverter<int8>;
extern template struct IntFilterParameterConverter<uint8>;

extern template struct IntFilterParameterConverter<int16>;
extern template struct IntFilterParameterConverter<uint16>;

extern template struct IntFilterParameterConverter<int32>;
extern template struct IntFilterParameterConverter<uint32>;

extern template struct IntFilterParameterConverter<int64>;
extern template struct IntFilterParameterConverter<uint64>;

using UInt64FilterParameterConverter = IntFilterParameterConverter<uint64>;

template <typename T>
struct StringToIntFilterParameterConverter
{
  using ParameterType = NumberParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

extern template struct StringToIntFilterParameterConverter<int8>;
extern template struct StringToIntFilterParameterConverter<uint8>;

extern template struct StringToIntFilterParameterConverter<int16>;
extern template struct StringToIntFilterParameterConverter<uint16>;

extern template struct StringToIntFilterParameterConverter<int32>;
extern template struct StringToIntFilterParameterConverter<uint32>;

extern template struct StringToIntFilterParameterConverter<int64>;
extern template struct StringToIntFilterParameterConverter<uint64>;

template <class T = float32>
struct FloatFilterParameterConverter
{
  using ParameterType = NumberParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

extern template struct FloatFilterParameterConverter<float32>;
extern template struct FloatFilterParameterConverter<float64>;

using DoubleFilterParameterConverter = FloatFilterParameterConverter<float64>;
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::Int8Parameter, "cae73834-68f8-4235-b010-8bea87d8ff7a");
SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::UInt8Parameter, "6c3efeff-ce8f-47c0-83d1-262f2b2dd6cc");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::Int16Parameter, "44ae56e8-e6e7-4e4d-8128-dd3dc2c6696e");
SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::UInt16Parameter, "156a6f46-77e5-41d8-8f5a-65ba1da52f2a");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::Int32Parameter, "21acff45-a653-45db-a0d1-f43cd344b93a");
SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::UInt32Parameter, "e9521130-276c-40c7-95d7-0b4cb4f80649");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::Int64Parameter, "b2039349-bd3a-4dbb-93d2-b4b5c633e697");
SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::UInt64Parameter, "36d91b23-5500-4ed4-bdf3-d680f54ee5d1");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::Float32Parameter, "e4452dfe-2f70-4833-819e-0cbbec21289b");
SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::Float64Parameter, "f2a18fff-a095-47d7-b436-ede41b5ea21a");
