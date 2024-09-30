#pragma once

#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/Filter/MutableDataParameter.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"

namespace nx::core
{
/**
 * @brief This Filter Parameter represents multiple DataArrays as input.
 */
class SIMPLNX_EXPORT MultiArraySelectionParameter : public MutableDataParameter
{
public:
  using ValueType = std::vector<DataPath>;
  using AllowedTypes = std::set<IArray::ArrayType>;
  using AllowedDataTypes = std::set<DataType>;
  using AllowedComponentShapes = std::vector<IArray::ShapeType>;

  MultiArraySelectionParameter() = delete;
  MultiArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const AllowedTypes& allowedTypes,
                               const AllowedDataTypes& allowedDataTypes, AllowedComponentShapes requiredComps = {});
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
   * @brief
   * @return
   */
  ValueType defaultPath() const;

  /**
   * @brief Returns the set of allowed IArray types. An empty set means all are allowed.
   * @return
   */
  AllowedTypes allowedTypes() const;

  /**
   * @brief Returns the set of allowed DataTypes. An empty set means all are allowed.
   * @return
   */
  AllowedDataTypes allowedDataTypes() const;

  /**
   * @brief Returns the required number of components. If return value is empty, then no component requirements.
   * @return
   */
  AllowedComponentShapes requiredComponentShapes() const;

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
  AllowedDataTypes m_AllowedDataTypes = {};
  AllowedComponentShapes m_RequiredComponentShapes = {};
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT MultiDataArraySelectionFilterParameterConverter
{
  using ParameterType = MultiArraySelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::MultiArraySelectionParameter, "d11e0bd8-f227-4fd1-b618-b6f16b259fc8");
