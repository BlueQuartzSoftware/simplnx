#pragma once

#include <vector>

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"

namespace complex
{
class VectorParameterBase : public ValueParameter
{
public:
  using NamesType = std::vector<std::string>;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual const NamesType& names() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual usize size() const = 0;

protected:
  using ValueParameter::ValueParameter;
};

template <class T>
class VectorParameter : public VectorParameterBase
{
public:
  using ValueType = std::vector<T>;
  using ElementType = typename ValueType::value_type;

  VectorParameter() = delete;
  VectorParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const NamesType& names);
  ~VectorParameter() override = default;

  VectorParameter(const VectorParameter&) = delete;
  VectorParameter(VectorParameter&&) noexcept = delete;

  VectorParameter& operator=(const VectorParameter&) = delete;
  VectorParameter& operator=(VectorParameter&&) noexcept = delete;

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
  [[nodiscard]] const ValueType& defaultVector() const;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] const NamesType& names() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] usize size() const override;

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
  [[nodiscard]] Result<> validateVector(const ValueType& value) const;

private:
  ValueType m_DefaultValue = {};
  NamesType m_Names;
};

extern template class VectorParameter<i8>;
extern template class VectorParameter<u8>;

extern template class VectorParameter<i16>;
extern template class VectorParameter<u16>;

extern template class VectorParameter<i32>;
extern template class VectorParameter<u32>;

extern template class VectorParameter<i64>;
extern template class VectorParameter<u64>;

extern template class VectorParameter<f32>;
extern template class VectorParameter<f64>;

using VectorInt8Parameter = VectorParameter<i8>;
using VectorUInt8Parameter = VectorParameter<u8>;

using VectorInt16Parameter = VectorParameter<i16>;
using VectorUInt16Parameter = VectorParameter<u16>;

using VectorInt32Parameter = VectorParameter<i32>;
using VectorUInt32Parameter = VectorParameter<u32>;

using VectorInt64Parameter = VectorParameter<i64>;
using VectorUInt64Parameter = VectorParameter<u64>;

using VectorFloat32Parameter = VectorParameter<f32>;
using VectorFloat64Parameter = VectorParameter<f64>;
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorInt8Parameter, "9f5f9683-e492-4a79-8378-79d727b2356a");

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorUInt8Parameter, "bff78ff3-35ef-482a-b3b1-df8806e7f7ef");

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorInt16Parameter, "43810a29-1a5f-4472-bec6-41de9ffe27f7");

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorUInt16Parameter, "2f1ba2f4-c5d5-403c-8b90-0bf60d2bde9b");

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorInt32Parameter, "d3188e18-e383-4727-ab32-88b5fda56ae8");

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorUInt32Parameter, "37322aa6-1a2f-4ecb-9aa1-8922d7ac1e49");

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorInt64Parameter, "4ceaffc1-7326-4f65-a33a-eae263dc22d1");

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorUInt64Parameter, "17309744-c4e8-4d1e-807e-e7012387f1ec");

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorFloat32Parameter, "88f231a1-7956-41f5-98b7-4471705d2805");

COMPLEX_DEF_PARAMETER_TRAITS(complex::VectorFloat64Parameter, "57cbdfdf-9d1a-4de8-95d7-71d0c01c5c96");
