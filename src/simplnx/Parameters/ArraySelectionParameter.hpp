#pragma once

#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/Filter/MutableDataParameter.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/simplnx_export.hpp"

#include <set>
#include <string>

namespace nx::core
{
/**
 * @brief This Filter Parameter describes a specific DataPath where the last DataObject in
 * the path is a DataArray Object.
 */
class SIMPLNX_EXPORT ArraySelectionParameter : public MutableDataParameter
{
public:
  using ValueType = DataPath;
  using AllowedTypes = std::set<DataType>;
  using AllowedComponentShapes = std::vector<IArray::ShapeType>;

  enum class DataLocation : uint8
  {
    Any = 0,
    InMemory = 1,
    OutOfCore = 2
  };

  ArraySelectionParameter() = delete;
  ArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const AllowedTypes& allowedTypes,
                          AllowedComponentShapes requiredComps = {}, DataLocation location = DataLocation::Any);
  ~ArraySelectionParameter() override = default;

  ArraySelectionParameter(const ArraySelectionParameter&) = delete;
  ArraySelectionParameter(ArraySelectionParameter&&) noexcept = delete;

  ArraySelectionParameter& operator=(const ArraySelectionParameter&) = delete;
  ArraySelectionParameter& operator=(ArraySelectionParameter&&) noexcept = delete;

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
   * @brief Returns the required number of components. If return value is empty, then no component requirements.
   * @return
   */
  AllowedComponentShapes requiredComponentShapes() const;

  /**
   * @brief Checks and returns if out-of-core data is allowed.
   * @return
   */
  bool allowsOutOfCore() const;

  /**
   * @brief Checks and returns if in-memory data is allowed.
   * @return
   */
  bool allowsInMemory() const;

  /**
   * @brief Returns an enum value containing the allowed data locations.
   * @return DataLocation flags
   */
  DataLocation allowedDataLocations() const;

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
  AllowedComponentShapes m_RequiredComponentShapes = {};
  DataLocation m_Location = DataLocation::Any;
};
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::ArraySelectionParameter, "ab047a7f-f9ab-4e6f-99b5-610e7b69fc5b");
