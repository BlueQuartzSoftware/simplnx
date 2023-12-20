#include "FileSystemPathParameter.hpp"

#include "complex/Common/Any.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/Utilities/FileUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

using namespace complex;

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
  const std::string prefix = fmt::format("\n    Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  if(path.empty())
  {
    return complex::MakeErrorResult(-3001, fmt::format("{} File System Path must not be empty", prefix));
  }

  if(!m_acceptAllExtensions && (m_PathType == complex::FileSystemPathParameter::PathType::InputFile || m_PathType == complex::FileSystemPathParameter::PathType::OutputFile))
  {
    if(!path.has_extension())
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-3002, fmt::format("{} File System Path must include a file extension", prefix)}})};
    }
    std::string lowerExtension = complex::StringUtilities::toLower(path.extension().string());
    if(path.has_extension() && !m_AvailableExtensions.empty() && m_AvailableExtensions.find(lowerExtension) == m_AvailableExtensions.end())
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-3003, fmt::format("{} File extension '{}' is not a valid file extension", prefix, path.extension().string())}})};
    }
  }

  switch(m_PathType)
  {
  case complex::FileSystemPathParameter::PathType::InputFile:
    return FileUtilities::ValidateInputFile(path);
  case complex::FileSystemPathParameter::PathType::InputDir:
    return FileUtilities::ValidateInputDir(path);
  case complex::FileSystemPathParameter::PathType::OutputFile:
    return FileUtilities::ValidateOutputFile(path);
  case complex::FileSystemPathParameter::PathType::OutputDir:
    return FileUtilities::ValidateOutputDir(path);
  }

  return {};
}
} // namespace complex
