#pragma once

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nonstd/span.hpp>

#include <filesystem>
#include <optional>

namespace nx::core
{
/**
 * @brief This FilterParameter allows for the selection of both an HDF5 file
 * to import a DataStructure from and the DataPaths to import.
 */
class SIMPLNX_EXPORT Dream3dImportParameter : public ValueParameter
{
public:
  struct ImportData
  {
    /**
     * @brief The path to the .dream3d file to import.
     */
    std::filesystem::path FilePath;

    /**
     * @brief Holds an optional vector of the DataPaths to import. If this
     * value is missing, all available DataPaths will be imported. Otherwise,
     * only the paths provided will be imported.
     */
    std::optional<std::vector<nx::core::DataPath>> DataPaths = std::nullopt;
  };

  using ValueType = ImportData;

  Dream3dImportParameter() = delete;
  Dream3dImportParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~Dream3dImportParameter() override = default;

  Dream3dImportParameter(const Dream3dImportParameter&) = delete;
  Dream3dImportParameter(Dream3dImportParameter&&) noexcept = delete;

  Dream3dImportParameter& operator=(const Dream3dImportParameter&) = delete;
  Dream3dImportParameter& operator=(Dream3dImportParameter&&) noexcept = delete;

  /**
   * @brief Returns the parameter class's Uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns a vector of accepted value types.
   * @return AcceptedTypes
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief Writes the provided value to JSON.
   * @param value
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief Reads and returns the parameter's values from JSON.
   * @return Result<std::any>
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief Creates and returns a copy of the parameter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

  /**
   * @brief
   * @return
   */
  std::any defaultValue() const override;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

  /**
   * @brief
   * @param importData
   * @return Result<>
   */
  Result<> validatePath(const ValueType& importData) const;

private:
  ValueType m_DefaultValue;
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT DataContainerReaderFilterParameterConverter
{
  using ParameterType = Dream3dImportParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::Dream3dImportParameter, "170a257d-5952-4854-9a91-4281cd06f4f5");
