#include "FileSystemPathParameter.hpp"

#include "complex/Common/StringLiteral.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"
#include "complex/Utilities/StringUtilities.hpp"

namespace fs = std::filesystem;

using namespace complex;

namespace
{
constexpr StringLiteral k_PathKey = "path";

//-----------------------------------------------------------------------------
Result<> ValidateInputFile(const FileSystemPathParameter::ValueType& path)
{
  if(!fs::exists(path))
  {
    return MakeErrorResult(-2, fmt::format("File System Path '{}' does not exist", path.string()));
  }

  if(!fs::is_regular_file(path))
  {
    return MakeErrorResult(-3, fmt::format("File System Path '{}' is not a file", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> ValidateInputDir(const FileSystemPathParameter::ValueType& path)
{
  if(!fs::exists(path))
  {
    return MakeErrorResult(-4, fmt::format("File System Path '{}' does not exist", path.string()));
  }
  if(!fs::is_directory(path))
  {
    return MakeErrorResult(-5, fmt::format("File System Path '{}' is not a file", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> ValidateOutputFile(const FileSystemPathParameter::ValueType& path)
{
  if(!fs::exists(path))
  {
    return MakeWarningVoidResult(-6, fmt::format("File System Path '{}' does not exist. It will be created during execution.", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> ValidateOutputDir(const FileSystemPathParameter::ValueType& path)
{
  if(!fs::exists(path))
  {
    return MakeWarningVoidResult(-7, fmt::format("File System Path '{}' does not exist. It will be created during execution.", path.string()));
  }
  return {};
}

} // namespace

namespace complex
{
//-----------------------------------------------------------------------------
FileSystemPathParameter::FileSystemPathParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                 const ExtensionsType& extensionsType, PathType pathType, bool shouldValidateExtension)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_PathType(pathType)
, m_AvailableExtensions(extensionsType)
, m_ShouldValidateExtension(shouldValidateExtension)
{
}

//-----------------------------------------------------------------------------
Uuid FileSystemPathParameter::uuid() const
{
  return ParameterTraits<FileSystemPathParameter>::uuid;
}

//-----------------------------------------------------------------------------
IParameter::AcceptedTypes FileSystemPathParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//-----------------------------------------------------------------------------
nlohmann::json FileSystemPathParameter::toJson(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.string();
  return json;
}

//-----------------------------------------------------------------------------
Result<std::any> FileSystemPathParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'FileSystemPathParameter' JSON Error: ";

  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a string", prefix, name()));
  }
  auto pathString = json.get<std::string>();
  std::filesystem::path path = pathString;
  return {{path}};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointer FileSystemPathParameter::clone() const
{
  return std::make_unique<FileSystemPathParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AvailableExtensions, m_PathType);
}

//-----------------------------------------------------------------------------
std::any FileSystemPathParameter::defaultValue() const
{
  return defaultPath();
}

//-----------------------------------------------------------------------------
typename FileSystemPathParameter::ValueType FileSystemPathParameter::defaultPath() const
{
  return m_DefaultValue;
}

//-----------------------------------------------------------------------------
FileSystemPathParameter::PathType FileSystemPathParameter::getPathType() const
{
  return m_PathType;
}

//-----------------------------------------------------------------------------
FileSystemPathParameter::ExtensionsType FileSystemPathParameter::getAvailableExtensions() const
{
  return m_AvailableExtensions;
}

//-----------------------------------------------------------------------------
Result<> FileSystemPathParameter::validate(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  return validatePath(path);
}

//-----------------------------------------------------------------------------
Result<> FileSystemPathParameter::validatePath(const ValueType& path) const
{
  if(path.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "File System Path must not be empty"}})};
  }

  if(m_ShouldValidateExtension)
  {
    if(!m_AvailableExtensions.empty() && !path.has_extension())
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-2, "File System Path must include a file extension"}})};
    }

    if(path.has_extension() && !m_AvailableExtensions.empty())
    {
      bool validExtension = false;
      for(const auto& ext : m_AvailableExtensions)
      {
        std::string inputExt = complex::StringUtilities::toLower(path.extension().string());
        std::string compareExt = complex::StringUtilities::toLower(ext);
        if(compareExt.at(0) == '.' && inputExt[0] != '.')
        {
          compareExt = complex::StringUtilities::chopr(compareExt, 1);
        }
        if(compareExt == inputExt)
        {
          validExtension = true;
          break;
        }
      }
      if(!validExtension)
      {
        return {nonstd::make_unexpected(std::vector<Error>{{-3, fmt::format("File extension '{}' is not a valid file extension", path.extension().string())}})};
      }
    }
  }

  switch(m_PathType)
  {
  case complex::FileSystemPathParameter::PathType::InputFile:
    return ValidateInputFile(path);
  case complex::FileSystemPathParameter::PathType::InputDir:
    return ValidateInputDir(path);
  case complex::FileSystemPathParameter::PathType::OutputFile:
    return ValidateOutputFile(path);
  case complex::FileSystemPathParameter::PathType::OutputDir:
    return ValidateOutputDir(path);
  }

  return {};
}
} // namespace complex
