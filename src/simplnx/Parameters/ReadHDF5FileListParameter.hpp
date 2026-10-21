#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/Utilities/FilePathGenerator.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>
#include <utility>
#include <vector>

namespace nx::core
{
/**
 * @class ReadHDF5FileListParameter
 * @brief Reads the same HDF5 dataset path out of a numbered stack of separate
 * HDF5 files (as opposed to ReadHDF5DataStackParameter, which reads a numbered
 * stack of dataset paths out of a single HDF5 file). The file list is generated
 * the same way GeneratedFileListParameter generates one; every generated file is
 * expected to contain a dataset at the same fixed datasetPath.
 */
class SIMPLNX_EXPORT ReadHDF5FileListParameter : public ValueParameter
{
public:
  using Ordering = FilePathGenerator::Ordering;

  struct SIMPLNX_EXPORT ValueType
  {
    std::string inputPath;
    std::string filePrefix;
    std::string fileSuffix;
    std::string fileExtension;
    int32 startIndex = 0;
    int32 endIndex = 0;
    int32 incrementIndex = 1;
    uint32 paddingDigits = 3;
    Ordering ordering = Ordering::LowToHigh;
    std::string datasetPath;

    static inline constexpr StringLiteral k_InputPath_Key = "input_path";
    static inline constexpr StringLiteral k_FilePrefix_Key = "file_prefix";
    static inline constexpr StringLiteral k_FileSuffix_Key = "file_suffix";
    static inline constexpr StringLiteral k_FileExtension_Key = "file_extension";
    static inline constexpr StringLiteral k_StartIndex_Key = "start_index";
    static inline constexpr StringLiteral k_EndIndex_Key = "end_index";
    static inline constexpr StringLiteral k_IncrementIndex_Key = "increment_index";
    static inline constexpr StringLiteral k_PaddingDigits_Key = "padding_digits";
    static inline constexpr StringLiteral k_Ordering_Key = "ordering";
    static inline constexpr StringLiteral k_DatasetPath_Key = "dataset_path";

    /**
     * @brief Generates the ordered list of HDF5 file paths for this numbered
     * file stack. Does not check that the files exist.
     */
    std::vector<std::string> generate() const
    {
      return FilePathGenerator::GenerateFileList(startIndex, endIndex, incrementIndex, ordering, inputPath, filePrefix, fileSuffix, fileExtension, paddingDigits);
    }

    /**
     * @brief Generates the ordered list of HDF5 file paths for this numbered
     * file stack, along with whether any of them are missing from disk.
     * @param failFast Stop at the first missing file instead of checking them all.
     */
    std::pair<std::vector<std::string>, bool> generateAndValidate(bool failFast) const
    {
      return FilePathGenerator::GenerateAndValidateFileList(startIndex, endIndex, incrementIndex, ordering, inputPath, filePrefix, fileSuffix, fileExtension, paddingDigits, failFast);
    }
  };

  ReadHDF5FileListParameter() = delete;
  ReadHDF5FileListParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~ReadHDF5FileListParameter() override = default;

  ReadHDF5FileListParameter(const ReadHDF5FileListParameter&) = delete;
  ReadHDF5FileListParameter(ReadHDF5FileListParameter&&) noexcept = delete;

  ReadHDF5FileListParameter& operator=(const ReadHDF5FileListParameter&) = delete;
  ReadHDF5FileListParameter& operator=(ReadHDF5FileListParameter&&) noexcept = delete;

  /**
   * @brief Returns the parameter's uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns a list of accepted input types.
   * @return AcceptedTypes
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief Creates a copy of the parameter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns the user defined default value.
   * @return std::any
   */
  std::any defaultValue() const override;

  /**
   * @brief Returns version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the json format changes.
   * @return uint64
   */
  VersionType getVersion() const override;

  /**
   * @brief Constructs an input value from the given arguments.
   * By default, accesses a singular value by key and returns that.
   * May be overriden by subclasses that depend on other parameters.
   * @param args
   * @param executionContext
   * @return
   */
  std::any construct(const Arguments& args, const ExecutionContext& executionContext) const override;

  /**
   * @brief Validates the given value. Returns warnings/errors.
   * @param value
   * @return Result<>
   */
  Result<> validate(const std::any& value) const override;

protected:
  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   * @return nlohmann::json
   */
  nlohmann::json toJsonImpl(const std::any& value) const override;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return Result<std::any>
   */
  Result<std::any> fromJsonImpl(const nlohmann::json& json, VersionType version) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::ReadHDF5FileListParameter, "a0e5c123-a007-468d-83c8-28345b23a812");
