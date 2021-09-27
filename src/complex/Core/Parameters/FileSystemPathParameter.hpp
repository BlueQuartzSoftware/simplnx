#pragma once

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>

namespace complex
{
/**
 * @brief This FilterParameter can represent an input or output file or folder on the
 * local file system. If your filter needs to gather a single input/output path, where
 * path can be a file or folder, this is the FilterParameter to use. The data is
 * held in a std::filesystem::path.
 */
class COMPLEX_EXPORT FileSystemPathParameter : public ValueParameter
{
public:
  enum class PathType : uint32_t
  {
    InputFile = 0,
    InputDir = 1,
    OutputFile = 2,
    OutputDir = 3
  };

  using ValueType = std::filesystem::path;

  FileSystemPathParameter() = delete;
  FileSystemPathParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, PathType pathType);
  ~FileSystemPathParameter() override = default;

  FileSystemPathParameter(const FileSystemPathParameter&) = delete;
  FileSystemPathParameter(FileSystemPathParameter&&) noexcept = delete;

  FileSystemPathParameter& operator=(const FileSystemPathParameter&) = delete;
  FileSystemPathParameter& operator=(FileSystemPathParameter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] AcceptedTypes acceptedTypes() const override;

  /**
   * @brief
   * @param value
   */
  [[nodiscard]] nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] UniquePointer clone() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::any defaultValue() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] ValueType defaultPath() const;

  /**
   * @brief sets the type of path that the parameter holds. File or Directory
   * @param pathType
   */
  void setPathType(PathType pathType);

  /**
   * @brief Returns the type of Path that the parameter represents. File or Directory.
   * @return
   */
  [[nodiscard]] PathType getPathType() const;

  /**
   * @brief
   * @param value
   * @return
   */
  [[nodiscard]] Result<> validate(const std::any& value) const override;

  /**
   * @brief
   * @param value
   * @return
   */
  [[nodiscard]] Result<> validatePath(const ValueType& path) const;

private:
  ValueType m_DefaultValue;
  PathType m_PathType;
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::FileSystemPathParameter, "f9a93f3d-21ef-43a1-a958-e57cbf3b2909");
