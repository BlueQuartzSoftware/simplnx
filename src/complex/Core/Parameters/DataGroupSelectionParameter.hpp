#pragma once

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/MutableDataParameter.hpp"
#include "complex/Filter/ParameterTraits.hpp"
#include "complex/complex_export.hpp"

#include <string>

namespace complex
{
class COMPLEX_EXPORT DataGroupSelectionParameter : public MutableDataParameter
{
public:
  using ValueType = DataPath;
  DataGroupSelectionParameter() = delete;
  DataGroupSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~DataGroupSelectionParameter() override = default;

  DataGroupSelectionParameter(const DataGroupSelectionParameter&) = delete;
  DataGroupSelectionParameter(DataGroupSelectionParameter&&) noexcept = delete;

  DataGroupSelectionParameter& operator=(const DataGroupSelectionParameter&) = delete;
  DataGroupSelectionParameter& operator=(DataGroupSelectionParameter&&) noexcept = delete;

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
COMPLEX_DEF_PARAMETER_TRAITS(complex::DataGroupSelectionParameter, "bff3d4ac-04a6-5251-b178-4f83f7865074");
