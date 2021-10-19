#pragma once

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>

#include "nonstd/span.hpp"

namespace complex
{
/**
 * @brief This FilterParameter allows for the selection of both an HDF5 file
 * to import a DataStructure from and the DataPaths to import.
 */
class COMPLEX_EXPORT H5DataStructureImportParameter : public ValueParameter
{
public:
  struct ImportData
  {
    std::filesystem::path FilePath;
    nonstd::span<complex::DataPath> DataPaths = {};
  };

  using ValueType = ImportData;

  H5DataStructureImportParameter() = delete;
  H5DataStructureImportParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~H5DataStructureImportParameter() override = default;

  H5DataStructureImportParameter(const H5DataStructureImportParameter&) = delete;
  H5DataStructureImportParameter(H5DataStructureImportParameter&&) noexcept = delete;

  H5DataStructureImportParameter& operator=(const H5DataStructureImportParameter&) = delete;
  H5DataStructureImportParameter& operator=(H5DataStructureImportParameter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief
   * @param value
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief
   * @return
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
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::H5DataStructureImportParameter, "170a257d-5952-4854-9a91-4281cd06f4f5");
