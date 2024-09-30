#pragma once

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/MutableDataParameter.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/simplnx_export.hpp"

#include <set>
#include <string>

namespace nx::core
{
class SIMPLNX_EXPORT DataGroupSelectionParameter : public MutableDataParameter
{
public:
  using ValueType = DataPath;

  using AllowedTypes = std::set<BaseGroup::GroupType>;

  DataGroupSelectionParameter() = delete;
  DataGroupSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const AllowedTypes& allowedTypes);
  ~DataGroupSelectionParameter() override = default;

  DataGroupSelectionParameter(const DataGroupSelectionParameter&) = delete;
  DataGroupSelectionParameter(DataGroupSelectionParameter&&) noexcept = delete;

  DataGroupSelectionParameter& operator=(const DataGroupSelectionParameter&) = delete;
  DataGroupSelectionParameter& operator=(DataGroupSelectionParameter&&) noexcept = delete;

  /**
   * @brief Returns the parameter's uuid.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns a list of accpeted input types.
   * @return
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief Creates a copy of the parameter.
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns the user defined default value.
   * @return
   */
  std::any defaultValue() const override;

  /**
   * @brief Returns version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the json format changes.
   * @return uint64
   */
  VersionType getVersion() const override;

  /**
   * @brief Returns the user defined default path.
   * @return
   */
  ValueType defaultPath() const;

  /**
   * @brief Returns the list of allowed DataObject types.
   * @return
   */
  const AllowedTypes& allowedTypes() const;

  /**
   * @brief Validates the given value against the given DataStructure. Returns warnings/errors.
   * @param dataStructure The active DataStructure to use during validation
   * @param value The value to validate
   * @return
   */
  Result<> validate(const DataStructure& dataStructure, const std::any& value) const override;

  /**
   * @brief Validates the given path against the given DataStructure. Returns warnings/errors.
   * @param dataStructure The active DataStructure to use during validation
   * @param value The value to validate
   * @return
   */
  Result<> validatePath(const DataStructure& dataStructure, const DataPath& value) const;

  /**
   * @brief Takes the value and a mutable DataStructure and attempts store the actual derived DataObject in the std::any.
   * Returns any warnings/errors.
   * @param dataStructure
   * @param value
   * @return
   */
  Result<std::any> resolve(DataStructure& dataStructure, const std::any& value) const override;

protected:
  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   */
  nlohmann::json toJsonImpl(const std::any& value) const override;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return
   */
  Result<std::any> fromJsonImpl(const nlohmann::json& json, VersionType version) const override;

private:
  ValueType m_DefaultValue = {};
  AllowedTypes m_AllowedTypes = {};
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT DataContainerSelectionFilterParameterConverter
{
  using ParameterType = DataGroupSelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

struct SIMPLNX_EXPORT DataContainerFromMultiSelectionFilterParameterConverter
{
  using ParameterType = DataGroupSelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core
SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::DataGroupSelectionParameter, "bff3d4ac-04a6-5251-b178-4f83f7865074");
