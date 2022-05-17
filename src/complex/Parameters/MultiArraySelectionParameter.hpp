#pragma once

#include "complex/Filter/MutableDataParameter.hpp"
#include "complex/Filter/ParameterTraits.hpp"

namespace complex
{
/**
 * @brief This Filter Parameter represents multiple DataArrays as input.
 */
class COMPLEX_EXPORT MultiArraySelectionParameter : public MutableDataParameter
{
public:
  using ValueType = std::vector<DataPath>;
  using AllowedTypes = std::set<DataType>;

  MultiArraySelectionParameter() = delete;
  MultiArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const AllowedTypes& allowedTypes);
  ~MultiArraySelectionParameter() override = default;

  MultiArraySelectionParameter(const MultiArraySelectionParameter&) = delete;
  MultiArraySelectionParameter(MultiArraySelectionParameter&&) noexcept = delete;

  MultiArraySelectionParameter& operator=(const MultiArraySelectionParameter&) = delete;
  MultiArraySelectionParameter& operator=(MultiArraySelectionParameter&&) noexcept = delete;

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
   * @brief
   * @return
   */
  ValueType defaultPath() const;

  /**
   * @brief Returns the set of allowed DataArray types. An empty set means all are allowed.
   * @return
   */
  AllowedTypes allowedTypes() const;

  /**
   * @brief Validates the given value against the given DataStructure. Returns warnings/errors.
   * @param dataStructure The active DataStructure to use during validation
   * @param value The value to validate
   * @return
   */
  Result<> validate(const DataStructure& dataStructure, const std::any& value) const override;

  /**
   * @brief Validates the given value against the given DataStructure. Returns warnings/errors.
   * @param dataStructure The active DataStructure to use during validation
   * @param value The value to validate
   * @return
   */
  Result<> validatePaths(const DataStructure& dataStructure, const ValueType& value) const;

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
  AllowedTypes m_AllowedTypes = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::MultiArraySelectionParameter, "d11e0bd8-f227-4fd1-b618-b6f16b259fc8");
