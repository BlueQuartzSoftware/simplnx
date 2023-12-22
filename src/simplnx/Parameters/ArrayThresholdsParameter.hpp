#pragma once

#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/Filter/MutableDataParameter.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Utilities/ArrayThreshold.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
class SIMPLNX_EXPORT ArrayThresholdsParameter : public MutableDataParameter
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
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::ArrayThresholdsParameter, "e93251bc-cdad-44c2-9332-58fe26aedfbe");
