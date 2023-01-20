#include "FileSystemPathParameter.hpp"

#include "complex/Common/Any.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <stdexcept>
#ifdef _WIN32
#include <io.h>
#define FSPP_ACCESS_FUNC_NAME _access
#else
#include <unistd.h>
#define FSPP_ACCESS_FUNC_NAME access
#endif

namespace fs = std::filesystem;

using namespace complex;

namespace
{
#ifdef _WIN32
constexpr int k_CheckWritable = 2;
#else
constexpr int k_CheckWritable = W_OK;
#endif

constexpr StringLiteral k_PathKey = "path";
constexpr int k_HasAccess = 0;

//-----------------------------------------------------------------------------
bool HasWriteAccess(const std::string& path)
{
  return FSPP_ACCESS_FUNC_NAME(path.c_str(), k_CheckWritable) == k_HasAccess;
}

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
Result<> ValidateDirectoryWritePermission(const FileSystemPathParameter::ValueType& path)
{
  const FileSystemPathParameter::ValueType emptyPath("");

  auto checkedPath = path;
  while(!fs::exists(checkedPath))
  {
    checkedPath = checkedPath.parent_path();
    if(checkedPath == emptyPath)
    {
      return MakeErrorResult(-9, fmt::format("Empty path encountered while trying to compute full path '{}'", path.string()));
    }
  }
  // We should be at the top of the tree with an existing directory.
  if(HasWriteAccess(checkedPath.string()))
  {
    return {};
  }
  return MakeErrorResult(-8, fmt::format("User does not have write permissions to path '{}'", path.string()));
}

//-----------------------------------------------------------------------------
Result<> ValidateOutputFile(const FileSystemPathParameter::ValueType& path)
{
  auto result = ValidateDirectoryWritePermission(path);
  if(result.invalid())
  {
    return result;
  }
  if(!fs::exists(path))
  {
    return MakeWarningVoidResult(-6, fmt::format("File System Path '{}' does not exist. It will be created during execution.", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> ValidateOutputDir(const FileSystemPathParameter::ValueType& path)
{
  auto result = ValidateDirectoryWritePermission(path);
  if(result.invalid())
  {
    return result;
  }
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
                                                 const ExtensionsType& extensionsType, PathType pathType, bool acceptAllExtensions)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_PathType(pathType)
, m_AvailableExtensions(extensionsType)
, m_acceptAllExtensions(acceptAllExtensions)
{
  ExtensionsType validatedExtensions;
  for(const auto& ext : m_AvailableExtensions)
  {
    if(ext.empty())
    {
      throw std::runtime_error("FileSystemPathParameter: One of the given extensions was empty. The filter is required to use non-emtpy extensions");
    }
    if(ext.at(0) != '.')
    {
      validatedExtensions.insert('.' + complex::StringUtilities::toLower(ext));
    }
    else
    {
      validatedExtensions.insert(complex::StringUtilities::toLower(ext));
    }
  }
  m_AvailableExtensions = validatedExtensions;
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
bool FileSystemPathParameter::acceptAllExtensions() const
{
  return m_acceptAllExtensions;
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
  return std::make_unique<FileSystemPathParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AvailableExtensions, m_PathType, m_acceptAllExtensions);
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
    return {nonstd::make_unexpected(std::vector<Error>{{-3001, "File System Path must not be empty"}})};
  }

  if(!m_acceptAllExtensions)
  {
    if(!path.has_extension())
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-3002, "File System Path must include a file extension"}})};
    }
    std::string lowerExtension = complex::StringUtilities::toLower(path.extension().string());
    if(path.has_extension() && !m_AvailableExtensions.empty() && m_AvailableExtensions.find(lowerExtension) == m_AvailableExtensions.end())
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-3003, fmt::format("File extension '{}' is not a valid file extension", path.extension().string())}})};
    }
  }

  if(!path.is_absolute() && !fs::exists(path))
  {
    return MakeErrorResult(-3004, fmt::format("Relative Path given does not exist.\n    Relative Path:'{}'\n    Current Working Path: '{}'\n    Computed Path: '{}/{}", path.string(),
                                              std::filesystem::current_path().string(), std::filesystem::current_path().string(), path.string()));
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
