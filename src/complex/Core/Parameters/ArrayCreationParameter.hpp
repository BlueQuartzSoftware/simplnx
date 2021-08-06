#pragma once

#include <string>

#include "complex/Filter/MutableDataParameter.hpp"

namespace complex
{
class ArrayCreationParameter : public MutableDataParameter
{
public:
  using ValueType = DataPath;

  ArrayCreationParameter() = delete;
  ArrayCreationParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~ArrayCreationParameter() override = default;

  ArrayCreationParameter(const ArrayCreationParameter&) = delete;
  ArrayCreationParameter(ArrayCreationParameter&&) noexcept = delete;

  ArrayCreationParameter& operator=(const ArrayCreationParameter&) = delete;
  ArrayCreationParameter& operator=(ArrayCreationParameter&&) noexcept = delete;

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
  [[nodiscard]] ValueType defaultPath() const;

  /**
   * @brief
   * @param value
   * @return
   */
  [[nodiscard]] Result<> validate(const DataStructure& dataStructure, const std::any& value) const override;

  /**
   * @brief
   * @param value
   * @return
   */
  [[nodiscard]] Result<> validatePath(const DataStructure& dataStructure, const DataPath& value) const;

  /**
   * @brief
   * @param value
   * @param data
   * @return
   */
  [[nodiscard]] Result<std::any> resolve(DataStructure& dataStructure, const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex
