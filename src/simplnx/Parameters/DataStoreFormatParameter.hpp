#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>
#include <utility>
#include <vector>

namespace nx::core
{

/**
 * @class DataStoreFormatParameter
 * @brief Selects a registered data-store format or automatic selection.
 *
 * An empty value selects automatic format resolution. A non-empty value must
 * match a format currently registered with Application.
 */
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

  Uuid uuid() const override;

  AcceptedTypes acceptedTypes() const override;

  UniquePointer clone() const override;

  std::any defaultValue() const override;

  VersionType getVersion() const override;

  ValueType defaultString() const;

  /**
   * @brief Returns registered format identifiers.
   * @return Format identifiers without display names.
   *
   * The result queries Application each call so plugin registrations are visible.
   */
  AvailableValuesType availableValues() const;

  /**
   * @brief Returns registered format identifiers with display names.
   *
   * Application provides Automatic, In Memory, and registered plugin formats.
   * @return Format identifier and display-name pairs.
   */
  std::vector<std::pair<std::string, std::string>> availableFormatsWithDisplayNames() const;

  /**
   * @brief Validates a selected data-store format.
   * @param value ValueType stored in std::any.
   * @return Error when value is not empty and is not registered.
   *
   * An empty value requests automatic selection and is always valid.
   */
  Result<> validate(const std::any& value) const override;

protected:
  nlohmann::json toJsonImpl(const std::any& value) const override;

  Result<std::any> fromJsonImpl(const nlohmann::json& json, VersionType version) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::DataStoreFormatParameter, "cfd5c150-2938-42a7-b023-4a9288fb6899");
