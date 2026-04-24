#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

namespace nx::core
{
/**
 * @brief This FilterParameter can represent an input or output file or folder on the
 * local file system. If your filter needs to gather a single input/output path, where
 * path can be a file or folder, this is the FilterParameter to use. The data is
 * held in a std::filesystem::path.
 */
class SIMPLNX_EXPORT FileSystemPathParameter : public ValueParameter
{
public:
  enum class PathType : uint32
  {
    InputFile = 0,
    InputDir = 1,
    OutputFile = 2,
    OutputDir = 3
  };

  using ExtensionsType = std::unordered_set<std::string>;

  using ValueType = std::filesystem::path;

  FileSystemPathParameter() = delete;

  /**
   * @param name
   * @param humanName
   * @param helpText
   * @param defaultValue
   * @param extensionsType
   * @param pathType
   * @param acceptAllExtensions
   */
  FileSystemPathParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const ExtensionsType& extensionsType, PathType pathType,
                          bool acceptAllExtensions = false);

  ~FileSystemPathParameter() override = default;

  FileSystemPathParameter(const FileSystemPathParameter&) = delete;
  FileSystemPathParameter(FileSystemPathParameter&&) noexcept = delete;

  FileSystemPathParameter& operator=(const FileSystemPathParameter&) = delete;
  FileSystemPathParameter& operator=(FileSystemPathParameter&&) noexcept = delete;

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
   * @return
   */
  bool acceptAllExtensions() const;

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
   * @brief
   * @return
   */
  ValueType defaultPath() const;

  /**
   * @brief Returns the type of Path that the parameter represents. File or directory and input or output.
   * @return
   */
  PathType getPathType() const;

  /**
   * @brief Returns all of the valid extension types that can be used.
   * @return
   */
  ExtensionsType getAvailableExtensions() const;

  /**
   * @brief Finds the longest accepted file extension that matches @p filename
   * as a case-insensitive suffix.
   *
   * Unlike `std::filesystem::path::extension()`, which only returns the last
   * dot-delimited token, this helper treats each entry of @p accepted as a
   * literal suffix of the filename. That allows compound extensions like
   * ".nii.gz" to be matched correctly: for a file named "foo.nii.gz" with
   * accepted = { ".gz", ".nii.gz" }, this returns ".nii.gz" because the
   * longer match wins.
   *
   * Accepted extensions are expected to include the leading '.'. Matching
   * is case-insensitive on both sides. If @p accepted is empty, or no entry
   * is a suffix of @p filename, returns std::nullopt.
   *
   * @param filename The filename to test. Callers should pass a bare
   *                 filename (no directory component) to avoid false matches
   *                 against parent-directory names.
   * @param accepted The set of accepted extensions.
   * @return The matched accepted extension (with its original casing) or
   *         std::nullopt.
   */
  static std::optional<std::string> MatchExtension(std::string_view filename, const ExtensionsType& accepted);

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validatePath(const ValueType& path) const;

protected:
  /**
   * @brief
   * @param value
   */
  nlohmann::json toJsonImpl(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  Result<std::any> fromJsonImpl(const nlohmann::json& json, VersionType version) const override;

private:
  ValueType m_DefaultValue;
  PathType m_PathType;
  ExtensionsType m_AvailableExtensions;
  bool m_acceptAllExtensions = false;
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT InputFileFilterParameterConverter
{
  using ParameterType = FileSystemPathParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};

struct SIMPLNX_EXPORT OutputFileFilterParameterConverter
{
  using ParameterType = FileSystemPathParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::FileSystemPathParameter, "f9a93f3d-21ef-43a1-a958-e57cbf3b2909");
