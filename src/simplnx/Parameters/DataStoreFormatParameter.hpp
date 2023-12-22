#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>
#include <vector>

namespace nx::core
{
class SIMPLNX_EXPORT DataStoreFormatParameter : public ValueParameter
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
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::DataStoreFormatParameter, "cfd5c150-2938-42a7-b023-4a9288fb6899");
