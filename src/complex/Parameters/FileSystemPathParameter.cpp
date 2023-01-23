#include "FileSystemPathParameter.hpp"

#include "complex/Common/Any.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include <cctype>
#include <filesystem>
#include <iostream>
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
Result<> ValidateDirectoryWritePermission(const FileSystemPathParameter::ValueType& path, bool isFile)
{
  auto checkedPath = path;
  if(isFile)
  {
    checkedPath = checkedPath.parent_path();
  }
  std::error_code errorCode;
  // We now have the parent directory. Let us see if *any* part of the path exists

  // If the path is relative, then make it absolute
  if(!checkedPath.is_absolute())
  {
    checkedPath = fs::absolute(checkedPath, errorCode);
  }

  // The idea here is to start walking up from the deepest directory and hopefully
  // find an existing directory. If we get to the top if the path and we are still
  // empty then:
  //  On unix based systems not sure this would happen. Even if the user set a path
  // to another drive that didn't exist, at some point you hit the '/' and then you
  // can try to create the directories.
  //  On Windows the user put in a bogus drive letter which is just a hard failure
  // because we can't make up a new drive letter.
  while(!fs::exists(checkedPath) && !checkedPath.empty())
  {
    checkedPath = checkedPath.parent_path();

    // Check if we worked our way up to the root of the path
#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32) || defined(_MSC_VER)
    if(checkedPath.string().size() == 3 && checkedPath.string().at(1) == ':')
    {
      break;
    }
#endif
  }

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32) || defined(_MSC_VER)
  std::string pathAsString = path.string();
  if(isalpha(pathAsString.at(0)) != 0 && pathAsString.at(1) == ':')
  {
    // We have a valid Drive letter + ":" so we can check that
    fs::path driveLetterOnly = path.root_name();
    if(!fs::exists({driveLetterOnly}))
    {
      return MakeErrorResult(-11, fmt::format("The drive does not exist on this system: '{}'", driveLetterOnly.string()));
    }
    // If the drive exists then go to the next step of checking write permissions
    checkedPath = {driveLetterOnly};
  }
#else
  // if we hit here then we walked all the way up the path and hit the root, so
  // see if the user can write to the root level
  if(checkedPath.empty())
  {
    checkedPath = "/";
  }
#endif
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
  auto result = ValidateDirectoryWritePermission(path, true);
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
  auto result = ValidateDirectoryWritePermission(path, false);
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
