#pragma once

#include <string>

#include "complex/DataStructure/IArray.hpp"
#include "complex/Filter/MutableDataParameter.hpp"
#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Utilities/ArrayThreshold.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT ArrayThresholdsParameter : public MutableDataParameter
{
public:
  using ValueType = ArrayThresholdSet;
  using AllowedComponentShapes = std::vector<IArray::ShapeType>;

  ArrayThresholdsParameter() = delete;
  ArrayThresholdsParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, AllowedComponentShapes requiredComps = {});
  ~ArrayThresholdsParameter() override = default;

  ArrayThresholdsParameter(const ArrayThresholdsParameter&) = delete;
  ArrayThresholdsParameter(ArrayThresholdsParameter&&) noexcept = delete;

  ArrayThresholdsParameter& operator=(const ArrayThresholdsParameter&) = delete;
  ArrayThresholdsParameter& operator=(ArrayThresholdsParameter&&) noexcept = delete;

  /**
   * @brief
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return AcceptedTypes
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief
   * @param value
   * @return nlohmann::json
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief
   * @return Result<std::any>
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief
   * @return UniquePointer
   */
  UniquePointer clone() const override;

  /**
   * @brief
   * @return std::any
   */
  std::any defaultValue() const override;

  /**
   * @brief
   * @return ValueType
   */
  ValueType defaultPath() const;

  /**
   * @brief Returns the required number of components. If return value is empty, then no component requirements.
   * @return
   */
  AllowedComponentShapes requiredComponentShapes() const;

  /**
   * @brief
   * @param value
   * @return Result<>
   */
  Result<> validate(const DataStructure& dataStructure, const std::any& value) const override;

  /**
   * @brief
   * @param value
   * @return Result<>
   */
  Result<> validatePath(const DataStructure& dataStructure, const DataPath& value) const;

  /**
   * @brief
   * @param value
   * @param data
   * @return Result<std::any>
   */
  Result<std::any> resolve(DataStructure& dataStructure, const std::any& value) const override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param value
   * @return Result<>
   */
  Result<> validatePaths(const DataStructure& dataStructure, const ValueType& value) const;

private:
  ValueType m_DefaultValue = {};
  AllowedComponentShapes m_RequiredComponentShapes = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::ArrayThresholdsParameter, "e93251bc-cdad-44c2-9332-58fe26aedfbe");
