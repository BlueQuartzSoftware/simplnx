#pragma once

#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/Filter/MutableDataParameter.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"

namespace nx::core
{
/**
 * @brief This Filter Parameter represents multiple DataObjects as input.
 */
class SIMPLNX_EXPORT MultiPathSelectionParameter : public MutableDataParameter
{
public:
  using ValueType = std::vector<DataPath>;

  MultiPathSelectionParameter() = delete;
  MultiPathSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~MultiPathSelectionParameter() override = default;

  MultiPathSelectionParameter(const MultiPathSelectionParameter&) = delete;
  MultiPathSelectionParameter(MultiPathSelectionParameter&&) noexcept = delete;

  MultiPathSelectionParameter& operator=(const MultiPathSelectionParameter&) = delete;
  MultiPathSelectionParameter& operator=(MultiPathSelectionParameter&&) noexcept = delete;

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
   * @brief Returns the default path.
   * @return
   */
  ValueType defaultPath() const;

  /**
   * @brief Validates the given value against the given DataStructure. Returns warnings/errors.
   * @param dataStructure The active DataStructure to use during validation
   * @param value The value to validate
   * @return
   */
  Result<> validate(const DataStructure& dataStructure, const std::any& value) const override;

  /**
   * @brief Validates the given paths against the given DataStructure. Returns warnings/errors.
   * @param dataStructure The active DataStructure to use during validation
   * @param value The value to validate
   * @return
   */
  Result<> validatePaths(const DataStructure& dataStructure, const ValueType& value) const;

  /**
   * @brief Takes the value and a mutable DataStructure and attempts store the actual derived DataObject in the std::any.
   * Returns any warnings/errors.
   * @param dataStructure The active DataStructure to use during resolution
   * @param value The value to resolve
   * @return
   */
  Result<std::any> resolve(DataStructure& dataStructure, const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::MultiPathSelectionParameter, "b5632f4f-fc13-4234-beb2-8fd8820eb6b6");
