#pragma once

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

#include <string>
#include <vector>

namespace complex
{
class COMPLEX_EXPORT DataStoreFormatParameter : public ValueParameter
{
public:
  using ValueType = std::string;
  using AvailableValuesType = std::vector<std::string>;

  DataStoreFormatParameter() = delete;
  DataStoreFormatParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~DataStoreFormatParameter() override = default;

  DataStoreFormatParameter(const DataStoreFormatParameter&) = delete;
  DataStoreFormatParameter(DataStoreFormatParameter&&) noexcept = delete;

  DataStoreFormatParameter& operator=(const DataStoreFormatParameter&) = delete;
  DataStoreFormatParameter& operator=(DataStoreFormatParameter&&) noexcept = delete;

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
  ValueType defaultString() const;

  /**
   * @brief
   * @retrurn
   */
  AvailableValuesType availableValues() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::DataStoreFormatParameter, "cfd5c150-2938-42a7-b023-4a9288fb6899");
