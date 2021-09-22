#pragma once

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace complex
{


class COMPLEX_EXPORT FileSystemPathParameter : public ValueParameter
{
public:

  enum class PathType : uint32_t
  {
    InputFile = 0,  //!<
    InputPath = 1,  //!<
    OutputFile = 2, //!<
    OutputPath = 3, //!<
    Unknown = 4     //!<
  };

  /**
   * @brief This
   */
  using ValueType = struct s_value_type
  {
    s_value_type(const std::filesystem::path& path, PathType pathType)
    : m_Path(path)
    , m_PathType(pathType){}
    std::filesystem::path m_Path;
    PathType m_PathType = {PathType::InputFile};
  };

  FileSystemPathParameter() = delete;
  FileSystemPathParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
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
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::FileSystemPathParameter, "f9a93f3d-21ef-43a1-a958-e57cbf3b2909");
