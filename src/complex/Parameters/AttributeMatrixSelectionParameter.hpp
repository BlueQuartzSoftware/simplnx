#pragma once

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/MutableDataParameter.hpp"
#include "complex/Filter/ParameterTraits.hpp"
#include "complex/complex_export.hpp"

#include <set>
#include <string>

namespace complex
{
class COMPLEX_EXPORT AttributeMatrixSelectionParameter : public MutableDataParameter
{
public:
  using ValueType = DataPath;

  using AllowedTypes = std::set<DataObject::Type>;

  AttributeMatrixSelectionParameter() = delete;
  AttributeMatrixSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~AttributeMatrixSelectionParameter() override = default;

  AttributeMatrixSelectionParameter(const AttributeMatrixSelectionParameter&) = delete;
  AttributeMatrixSelectionParameter(AttributeMatrixSelectionParameter&&) noexcept = delete;

  AttributeMatrixSelectionParameter& operator=(const AttributeMatrixSelectionParameter&) = delete;
  AttributeMatrixSelectionParameter& operator=(AttributeMatrixSelectionParameter&&) noexcept = delete;

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
};
} // namespace complex
COMPLEX_DEF_PARAMETER_TRAITS(complex::AttributeMatrixSelectionParameter, "a3619d74-a1d9-4bc2-9e03-ca001d65b119");
