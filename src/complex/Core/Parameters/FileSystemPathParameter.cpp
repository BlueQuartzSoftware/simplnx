#include "FileSystemPathParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace
{
constexpr const char k_PathKey[] = "path";
constexpr const char k_TypeKey[] = "type";

//-----------------------------------------------------------------------------
complex::Result<> validateInputFile(const complex::FileSystemPathParameter::ValueType& value)
{
  const std::filesystem::path& path = value.m_Path;

  if(!fs::exists(path))
  {
    return {nonstd::make_unexpected(std::vector<complex::Error>{{-1, fmt::format("Path \"{}\" does not exist", path.string())}})};
  }

  if(!fs::is_regular_file(path))
  {
    return {nonstd::make_unexpected(std::vector<complex::Error>{{-1, fmt::format("Path \"{}\" is not a file", path.string())}})};
  }
  return {};
}

//-----------------------------------------------------------------------------
complex::Result<> validateInputPath(const complex::FileSystemPathParameter::ValueType& value)
{
  const std::filesystem::path& path = value.m_Path;

  if(!fs::exists(path))
  {
    return {nonstd::make_unexpected(std::vector<complex::Error>{{-1, fmt::format("Path \"{}\" does not exist", path.string())}})};
  }
  if(fs::is_regular_file(path))
  {
    return {nonstd::make_unexpected(std::vector<complex::Error>{{-1, fmt::format("Path \"{}\" is not a file", path.string())}})};
  }
  return {};
}

//-----------------------------------------------------------------------------
complex::Result<> validateOutputFile(const complex::FileSystemPathParameter::ValueType& value)
{
  const std::filesystem::path& path = value.m_Path;
  if(!fs::exists(path))
  {
    return {nonstd::make_unexpected(std::vector<complex::Error>{{-1, fmt::format("Path \"{}\" does not exist. It will be created during execution.", path.string())}})};
  }
  return {};
}

//-----------------------------------------------------------------------------
complex::Result<> validateOutputPath(const complex::FileSystemPathParameter::ValueType& value)
{
  const std::filesystem::path& path = value.m_Path;
  if(!fs::exists(path))
  {
    return {nonstd::make_unexpected(std::vector<complex::Error>{{-1, fmt::format("Path \"{}\" does not exist. It will be created during execution.", path.string())}})};
  }
  return {};
}

} // namespace

namespace complex
{
//-----------------------------------------------------------------------------
FileSystemPathParameter::FileSystemPathParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
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
  ValueType fileSystemPathInfo = std::any_cast<ValueType>(value);
  nlohmann::json json;
  json[::k_PathKey] = fileSystemPathInfo.m_Path.string();
  json[::k_TypeKey] = fileSystemPathInfo.m_PathType;
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
  return std::make_unique<FileSystemPathParameter>(name(), humanName(), helpText(), m_DefaultValue);
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
Result<> FileSystemPathParameter::validate(const std::any& value) const
{
  auto path = std::any_cast<ValueType>(value);
  return validatePath(path);
}

//-----------------------------------------------------------------------------
Result<> FileSystemPathParameter::validatePath(const ValueType& value) const
{
  const std::filesystem::path& path = value.m_Path;
  // No matter the type, there must be something in the 'path'
  if(path.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "Path must not be empty"}})};
  }

  switch(value.m_PathType)
  {
  case complex::FileSystemPathParameter::PathType::InputFile:
    return validateInputFile(value);
  case complex::FileSystemPathParameter::PathType::InputPath:
    return validateInputPath(value);
  case complex::FileSystemPathParameter::PathType::OutputFile:
    return validateOutputFile(value);
  case complex::FileSystemPathParameter::PathType::OutputPath:
    return validateOutputPath(value);
  case complex::FileSystemPathParameter::PathType::Unknown:
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "PathType is Unknown"}})};
  }

  return {};
}
} // namespace complex
