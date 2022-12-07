#pragma once

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/MutableDataParameter.hpp"
#include "complex/Filter/ParameterTraits.hpp"
#include "complex/complex_export.hpp"

#include <string>

namespace complex
{
class COMPLEX_EXPORT DataGroupCreationParameter : public MutableDataParameter
{
public:
  using ValueType = DataPath;
  using AllowedParentGroupType = std::set<BaseGroup::GroupType>;

  DataGroupCreationParameter() = delete;
  DataGroupCreationParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const AllowedParentGroupType& allowedParentGroup);
  ~DataGroupCreationParameter() override = default;

  DataGroupCreationParameter(const DataGroupCreationParameter&) = delete;
  DataGroupCreationParameter(DataGroupCreationParameter&&) noexcept = delete;

  DataGroupCreationParameter& operator=(const DataGroupCreationParameter&) = delete;
  DataGroupCreationParameter& operator=(DataGroupCreationParameter&&) noexcept = delete;

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
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

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
   * @brief Returns the user defined default path.
   * @return
   */
  ValueType defaultPath() const;

  /**
   * @brief Returns to the user the allowed group types of the parent group
   * @return std::set<BaseGroup::GroupType>
   */
  AllowedParentGroupType allowedParentGroupType() const;

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

private:
  ValueType m_DefaultValue = {};
  AllowedParentGroupType m_AllowedParentGroupType = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::DataGroupCreationParameter, "bff2d4ac-04a6-5251-b188-4f83f7865074");
