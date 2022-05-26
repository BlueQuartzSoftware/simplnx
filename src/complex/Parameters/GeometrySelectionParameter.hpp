#pragma once

#include <set>
#include <string>

#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/Filter/MutableDataParameter.hpp"
#include "complex/Filter/ParameterTraits.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief This Filter Parameter describes a specific DataPath where the last DataObject in
 * the path is a geometry of the specified type.
 */
class COMPLEX_EXPORT GeometrySelectionParameter : public MutableDataParameter
{
public:
  using ValueType = DataPath;
  using AllowedType = IGeometry::Type;
  using AllowedTypes = std::set<AllowedType>;

  GeometrySelectionParameter() = delete;
  GeometrySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const AllowedTypes& allowedTypes);
  ~GeometrySelectionParameter() override = default;

  GeometrySelectionParameter(const GeometrySelectionParameter&) = delete;
  GeometrySelectionParameter(GeometrySelectionParameter&&) noexcept = delete;

  GeometrySelectionParameter& operator=(const GeometrySelectionParameter&) = delete;
  GeometrySelectionParameter& operator=(GeometrySelectionParameter&&) noexcept = delete;

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
  const ValueType& defaultPath() const;

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
   * @brief Validates the given value against the given DataStructure. Returns warnings/errors.
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
  AllowedTypes m_AllowedTypes = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::GeometrySelectionParameter, "3804cd7f-4ee4-400f-80ad-c5af17735de2");
