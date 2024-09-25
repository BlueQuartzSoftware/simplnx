#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"

#include <vector>

namespace nx::core
{
class VectorParameterBase : public ValueParameter
{
public:
  using NamesType = std::vector<std::string>;

  /**
   * @brief
   * @return
   */
  virtual const NamesType& names() const = 0;

  /**
   * @brief
   * @return
   */
  virtual usize size() const = 0;

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
  VectorParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~VectorParameter() override = default;

  VectorParameter(const VectorParameter&) = delete;
  VectorParameter(VectorParameter&&) noexcept = delete;

  VectorParameter& operator=(const VectorParameter&) = delete;
  VectorParameter& operator=(VectorParameter&&) noexcept = delete;

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
  const ValueType& defaultVector() const;

  /**
   * @brief
   * @return
   */
  const NamesType& names() const override;

  /**
   * @brief
   * @return
   */
  usize size() const override;

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
  Result<> validateVector(const ValueType& value) const;

private:
  ValueType m_DefaultValue = {};
  NamesType m_Names;
};

extern template class VectorParameter<int8>;
extern template class VectorParameter<uint8>;

extern template class VectorParameter<int16>;
extern template class VectorParameter<uint16>;

extern template class VectorParameter<int32>;
extern template class VectorParameter<uint32>;

extern template class VectorParameter<int64>;
extern template class VectorParameter<uint64>;

extern template class VectorParameter<float32>;
extern template class VectorParameter<float64>;

using VectorInt8Parameter = VectorParameter<int8>;
using VectorUInt8Parameter = VectorParameter<uint8>;

using VectorInt16Parameter = VectorParameter<int16>;
using VectorUInt16Parameter = VectorParameter<uint16>;

using VectorInt32Parameter = VectorParameter<int32>;
using VectorUInt32Parameter = VectorParameter<uint32>;

using VectorInt64Parameter = VectorParameter<int64>;
using VectorUInt64Parameter = VectorParameter<uint64>;

using VectorFloat32Parameter = VectorParameter<float32>;
using VectorFloat64Parameter = VectorParameter<float64>;

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT RangeFilterParameterConverter
{
  using ParameterType = VectorParameter<float64>;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

template <typename T>
struct MultiToVec3FilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2, const nlohmann::json& json3);
};

extern template struct MultiToVec3FilterParameterConverter<int8>;
extern template struct MultiToVec3FilterParameterConverter<uint8>;

extern template struct MultiToVec3FilterParameterConverter<int16>;
extern template struct MultiToVec3FilterParameterConverter<uint16>;

extern template struct MultiToVec3FilterParameterConverter<int32>;
extern template struct MultiToVec3FilterParameterConverter<uint32>;

extern template struct MultiToVec3FilterParameterConverter<int64>;
extern template struct MultiToVec3FilterParameterConverter<uint64>;

extern template struct MultiToVec3FilterParameterConverter<float32>;
extern template struct MultiToVec3FilterParameterConverter<float64>;

using UInt64ToVec3FilterParameterConverter = MultiToVec3FilterParameterConverter<uint64>;
using FloatToVec3FilterParameterConverter = MultiToVec3FilterParameterConverter<float32>;
using DoubleToVec3FilterParameterConverter = MultiToVec3FilterParameterConverter<float64>;

template <typename T>
struct Vec3FilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

extern template struct Vec3FilterParameterConverter<int8>;
extern template struct Vec3FilterParameterConverter<uint8>;

extern template struct Vec3FilterParameterConverter<int16>;
extern template struct Vec3FilterParameterConverter<uint16>;

extern template struct Vec3FilterParameterConverter<int32>;
extern template struct Vec3FilterParameterConverter<uint32>;

extern template struct Vec3FilterParameterConverter<int64>;
extern template struct Vec3FilterParameterConverter<uint64>;

extern template struct Vec3FilterParameterConverter<float32>;
extern template struct Vec3FilterParameterConverter<float64>;

using IntVec3FilterParameterConverter = Vec3FilterParameterConverter<int32>;
using UInt32Vec3FilterParameterConverter = Vec3FilterParameterConverter<uint32>;
using UInt64Vec3FilterParameterConverter = Vec3FilterParameterConverter<uint64>;
using FloatVec3FilterParameterConverter = Vec3FilterParameterConverter<float32>;
using DoubleVec3FilterParameterConverter = Vec3FilterParameterConverter<float64>;

template <typename T>
struct Vec4FilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

extern template struct Vec4FilterParameterConverter<int8>;
extern template struct Vec4FilterParameterConverter<uint8>;

extern template struct Vec4FilterParameterConverter<int16>;
extern template struct Vec4FilterParameterConverter<uint16>;

extern template struct Vec4FilterParameterConverter<int32>;
extern template struct Vec4FilterParameterConverter<uint32>;

extern template struct Vec4FilterParameterConverter<int64>;
extern template struct Vec4FilterParameterConverter<uint64>;

extern template struct Vec4FilterParameterConverter<float32>;
extern template struct Vec4FilterParameterConverter<float64>;

using FloatVec4FilterParameterConverter = Vec4FilterParameterConverter<float32>;

template <typename T>
struct AxisAngleFilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

extern template struct AxisAngleFilterParameterConverter<int8>;
extern template struct AxisAngleFilterParameterConverter<uint8>;

extern template struct AxisAngleFilterParameterConverter<int16>;
extern template struct AxisAngleFilterParameterConverter<uint16>;

extern template struct AxisAngleFilterParameterConverter<int32>;
extern template struct AxisAngleFilterParameterConverter<uint32>;

extern template struct AxisAngleFilterParameterConverter<int64>;
extern template struct AxisAngleFilterParameterConverter<uint64>;

extern template struct AxisAngleFilterParameterConverter<float32>;
extern template struct AxisAngleFilterParameterConverter<float64>;

template <typename T>
struct Vec3p1FilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2);
};

extern template struct Vec3p1FilterParameterConverter<int8>;
extern template struct Vec3p1FilterParameterConverter<uint8>;

extern template struct Vec3p1FilterParameterConverter<int16>;
extern template struct Vec3p1FilterParameterConverter<uint16>;

extern template struct Vec3p1FilterParameterConverter<int32>;
extern template struct Vec3p1FilterParameterConverter<uint32>;

extern template struct Vec3p1FilterParameterConverter<int64>;
extern template struct Vec3p1FilterParameterConverter<uint64>;

extern template struct Vec3p1FilterParameterConverter<float32>;
extern template struct Vec3p1FilterParameterConverter<float64>;

using FloatVec3p1FilterParameterConverter = Vec3p1FilterParameterConverter<float32>;
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorInt8Parameter, "9f5f9683-e492-4a79-8378-79d727b2356a");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorUInt8Parameter, "bff78ff3-35ef-482a-b3b1-df8806e7f7ef");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorInt16Parameter, "43810a29-1a5f-4472-bec6-41de9ffe27f7");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorUInt16Parameter, "2f1ba2f4-c5d5-403c-8b90-0bf60d2bde9b");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorInt32Parameter, "d3188e18-e383-4727-ab32-88b5fda56ae8");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorUInt32Parameter, "37322aa6-1a2f-4ecb-9aa1-8922d7ac1e49");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorInt64Parameter, "4ceaffc1-7326-4f65-a33a-eae263dc22d1");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorUInt64Parameter, "17309744-c4e8-4d1e-807e-e7012387f1ec");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorFloat32Parameter, "88f231a1-7956-41f5-98b7-4471705d2805");

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::VectorFloat64Parameter, "57cbdfdf-9d1a-4de8-95d7-71d0c01c5c96");
