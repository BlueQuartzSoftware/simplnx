#pragma once

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"

namespace complex
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
  [[nodiscard]] ValueType defaultNumber() const;

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
  [[nodiscard]] Result<> validateNumber(ValueType value) const;

private:
  ValueType m_DefaultValue = {};
};

extern template class NumberParameter<i8>;
extern template class NumberParameter<u8>;

extern template class NumberParameter<i16>;
extern template class NumberParameter<u16>;

extern template class NumberParameter<i32>;
extern template class NumberParameter<u32>;

extern template class NumberParameter<i64>;
extern template class NumberParameter<u64>;

extern template class NumberParameter<f32>;
extern template class NumberParameter<f64>;

using Int8Parameter = NumberParameter<i8>;
using UInt8Parameter = NumberParameter<u8>;

using Int16Parameter = NumberParameter<i16>;
using UInt16Parameter = NumberParameter<u16>;

using Int32Parameter = NumberParameter<i32>;
using UInt32Parameter = NumberParameter<u32>;

using Int64Parameter = NumberParameter<i64>;
using UInt64Parameter = NumberParameter<u64>;

using Float32Parameter = NumberParameter<f32>;
using Float64Parameter = NumberParameter<f64>;
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::Int8Parameter, "cae73834-68f8-4235-b010-8bea87d8ff7a");
COMPLEX_DEF_PARAMETER_TRAITS(complex::UInt8Parameter, "6c3efeff-ce8f-47c0-83d1-262f2b2dd6cc");

COMPLEX_DEF_PARAMETER_TRAITS(complex::Int16Parameter, "44ae56e8-e6e7-4e4d-8128-dd3dc2c6696e");
COMPLEX_DEF_PARAMETER_TRAITS(complex::UInt16Parameter, "156a6f46-77e5-41d8-8f5a-65ba1da52f2a");

COMPLEX_DEF_PARAMETER_TRAITS(complex::Int32Parameter, "21acff45-a653-45db-a0d1-f43cd344b93a");
COMPLEX_DEF_PARAMETER_TRAITS(complex::UInt32Parameter, "e9521130-276c-40c7-95d7-0b4cb4f80649");

COMPLEX_DEF_PARAMETER_TRAITS(complex::Int64Parameter, "b2039349-bd3a-4dbb-93d2-b4b5c633e697");
COMPLEX_DEF_PARAMETER_TRAITS(complex::UInt64Parameter, "36d91b23-5500-4ed4-bdf3-d680f54ee5d1");

COMPLEX_DEF_PARAMETER_TRAITS(complex::Float32Parameter, "e4452dfe-2f70-4833-819e-0cbbec21289b");
COMPLEX_DEF_PARAMETER_TRAITS(complex::Float64Parameter, "f2a18fff-a095-47d7-b436-ede41b5ea21a");
