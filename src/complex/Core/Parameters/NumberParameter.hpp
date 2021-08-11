#pragma once

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
