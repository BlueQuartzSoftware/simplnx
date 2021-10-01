#pragma once

#include <string>

#include "complex/Filter/MutableDataParameter.hpp"
#include "complex/Filter/ParameterTraits.hpp"

namespace complex
{
class ArraySelectionParameter : public MutableDataParameter
{
public:
  using ValueType = DataPath;

  ArraySelectionParameter() = delete;
  ArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~ArraySelectionParameter() override = default;

  ArraySelectionParameter(const ArraySelectionParameter&) = delete;
  ArraySelectionParameter(ArraySelectionParameter&&) noexcept = delete;

  ArraySelectionParameter& operator=(const ArraySelectionParameter&) = delete;
  ArraySelectionParameter& operator=(ArraySelectionParameter&&) noexcept = delete;

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
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::ArraySelectionParameter, "ab047a7f-f9ab-4e6f-99b5-610e7b69fc5b");
