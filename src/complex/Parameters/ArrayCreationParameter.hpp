#pragma once

#include <string>

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Filter/MutableDataParameter.hpp"
#include "complex/Filter/ParameterTraits.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT ArrayCreationParameter : public MutableDataParameter
{
public:
  using ValueType = DataPath;
  using AllowedParentGroupType = std::set<BaseGroup::GroupType>;

  ArrayCreationParameter() = delete;
  ArrayCreationParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const AllowedParentGroupType& allowedParentGroup);
  ~ArrayCreationParameter() override = default;

  ArrayCreationParameter(const ArrayCreationParameter&) = delete;
  ArrayCreationParameter(ArrayCreationParameter&&) noexcept = delete;

  ArrayCreationParameter& operator=(const ArrayCreationParameter&) = delete;
  ArrayCreationParameter& operator=(ArrayCreationParameter&&) noexcept = delete;

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
  ValueType defaultPath() const;

  /**
   * @brief
   * @return
   */
  AllowedParentGroupType allowedParentGroupType() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const DataStructure& dataStructure, const std::any& value) const override;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validatePath(const DataStructure& dataStructure, const DataPath& value) const;

  /**
   * @brief
   * @param value
   * @param data
   * @return
   */
  Result<std::any> resolve(DataStructure& dataStructure, const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
  AllowedParentGroupType m_AllowedParentGroupType = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::ArrayCreationParameter, "ab047a7d-f81b-4e6f-99b5-610e7b69fc5b");
