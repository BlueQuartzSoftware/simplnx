#include "FileSystemPathParameter.hpp"

#include "complex/Common/StringLiteral.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

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
    return MakeErrorResult(-2, fmt::format("Path \"{}\" does not exist", path.string()));
  }

  if(!fs::is_regular_file(path))
  {
    return MakeErrorResult(-3, fmt::format("Path \"{}\" is not a file", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> ValidateInputDir(const FileSystemPathParameter::ValueType& path)
{
  if(!fs::exists(path))
  {
    return MakeErrorResult(-4, fmt::format("Path \"{}\" does not exist", path.string()));
  }
  if(!fs::is_directory(path))
  {
    return MakeErrorResult(-5, fmt::format("Path \"{}\" is not a file", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> ValidateOutputFile(const FileSystemPathParameter::ValueType& path)
{
  if(!fs::exists(path))
  {
    return MakeWarningVoidResult(-6, fmt::format("Path \"{}\" does not exist. It will be created during execution.", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> ValidateOutputDir(const FileSystemPathParameter::ValueType& path)
{
  if(!fs::exists(path))
  {
    return MakeWarningVoidResult(-7, fmt::format("Path \"{}\" does not exist. It will be created during execution.", path.string()));
  }
  return {};
}

} // namespace

namespace complex
{
//-----------------------------------------------------------------------------
FileSystemPathParameter::FileSystemPathParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, PathType pathType)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_PathType(pathType)
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
  ValueType path = std::any_cast<ValueType>(value);
  nlohmann::json json;
  json[k_PathKey.c_str()] = path.string();
  return json;
}

//-----------------------------------------------------------------------------
Result<std::any> FileSystemPathParameter::fromJson(const nlohmann::json& json) const
{
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonValue = json.at(key);
  if(!jsonValue.is_string())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not a string", key)}})};
  }
  auto pathString = jsonValue.get<std::string>();
  std::filesystem::path path = pathString;
  return {path};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointer FileSystemPathParameter::clone() const
{
  return std::make_unique<FileSystemPathParameter>(name(), humanName(), helpText(), m_DefaultValue, m_PathType);
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

void FileSystemPathParameter::setPathType(FileSystemPathParameter::PathType pathType)
{
  m_PathType = pathType;
}

FileSystemPathParameter::PathType FileSystemPathParameter::getPathType() const
{
  return m_PathType;
}

//-----------------------------------------------------------------------------
Result<> FileSystemPathParameter::validate(const std::any& value) const
{
  auto path = std::any_cast<ValueType>(value);
  return validatePath(path);
}

//-----------------------------------------------------------------------------
Result<> FileSystemPathParameter::validatePath(const ValueType& path) const
{
  if(path.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "Path must not be empty"}})};
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
