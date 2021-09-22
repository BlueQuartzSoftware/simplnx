#include "GeneratedFileListParameter.hpp"

#include "complex/Core/Parameters/utils/FilePathGenerator.hpp"

#include <fmt/core.h>

namespace fs = std::filesystem;

namespace complex
{

//-----------------------------------------------------------------------------
// Define Constants here
namespace GeneratedFileListParameterConstants
{
constexpr const char k_StartIndex[] = "StartIndex";
constexpr const char k_EndIndex[] = "EndIndex";
constexpr const char k_PaddingDigits[] = "PaddingDigits";
constexpr const char k_Ordering[] = "Ordering";
constexpr const char k_IncrementIndex[] = "IncrementIndex";
constexpr const char k_InputPath[] = "InputPath";
constexpr const char k_FilePrefix[] = "FilePrefix";
constexpr const char k_FileSuffix[] = "FileSuffix";
constexpr const char k_FileExtension[] = "FileExtension";
} // namespace GeneratedFileListParameterConstants

using namespace GeneratedFileListParameterConstants;

//-----------------------------------------------------------------------------
GeneratedFileListParameter::GeneratedFileListParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

//-----------------------------------------------------------------------------
Uuid GeneratedFileListParameter::uuid() const
{
  return ParameterTraits<GeneratedFileListParameter>::uuid;
}

//-----------------------------------------------------------------------------
IParameter::AcceptedTypes GeneratedFileListParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//-----------------------------------------------------------------------------
nlohmann::json GeneratedFileListParameter::toJson(const std::any& value) const
{
  auto data = std::any_cast<ValueType>(value);
  nlohmann::json json;
  json[k_StartIndex] = static_cast<int32_t>(data.m_StartIndex);
  json[k_EndIndex] = static_cast<int32_t>(data.m_EndIndex);
  json[k_PaddingDigits] = static_cast<int32_t>(data.m_PaddingDigits);
  json[k_Ordering] = static_cast<int32_t>(data.m_Ordering);
  json[k_IncrementIndex] = static_cast<int32_t>(data.m_IncrementIndex);
  json[k_InputPath] = data.m_InputPath;
  json[k_FilePrefix] = data.m_FilePrefix;
  json[k_FileSuffix] = data.m_FileSuffix;
  json[k_FileExtension] = data.m_FileExtension;
  return json;
}

//-----------------------------------------------------------------------------
Result<std::any> GeneratedFileListParameter::fromJson(const nlohmann::json& json) const
{
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonValue = json.at(key);
  if(!jsonValue.is_object())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not a string", key)}})};
  }

  ValueType value;
  if(json[k_StartIndex].is_number_float() && json[k_EndIndex].is_number_float() && json[k_PaddingDigits].is_number_float() && json[k_Ordering].is_number_float() &&
     json[k_IncrementIndex].is_number_float() && json[k_InputPath].is_string() && json[k_FilePrefix].is_string() && json[k_FileSuffix].is_string() && json[k_FileExtension].is_string())
  {
    value.m_PaddingDigits = static_cast<int32_t>(json[k_PaddingDigits].get<int32_t>());
    value.m_Ordering = static_cast<Ordering>(json[k_Ordering].get<uint32_t>());
    value.m_IncrementIndex = static_cast<int32_t>(json[k_IncrementIndex].get<int32_t>());
    value.m_InputPath = json[k_InputPath].get<std::string>();
    value.m_FilePrefix = json[k_FilePrefix].get<std::string>();
    value.m_FileSuffix = json[k_FileSuffix].get<std::string>();
    value.m_FileExtension = json[k_FileExtension].get<std::string>();
    value.m_StartIndex = static_cast<int32_t>(json[k_StartIndex].get<int32_t>());
    value.m_EndIndex = static_cast<int32_t>(json[k_EndIndex].get<int32_t>());
  }
  return {value};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointer GeneratedFileListParameter::clone() const
{
  return std::make_unique<GeneratedFileListParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

//-----------------------------------------------------------------------------
std::any GeneratedFileListParameter::defaultValue() const
{
  return m_DefaultValue;
}

//-----------------------------------------------------------------------------
Result<> GeneratedFileListParameter::validate(const std::any& valueRef) const
{
  Result<> results;

  auto value = std::any_cast<ValueType>(valueRef);
  if(value.m_StartIndex < value.m_EndIndex)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "StartIndex must be greater than EndIndex"}})};
  }
  if(value.m_Ordering != Ordering::LowToHigh && value.m_Ordering != Ordering::HighToLow)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "Ordering must be ZERO (0: Low To High) or ONE (1: High To Low)"}})};
  }
  bool missingFiles = false;
  // Generate the file lsit
  std::vector<std::string> fileList =
      FilePathGenerator::GenerateFileList(value.m_StartIndex, value.m_EndIndex, value.m_IncrementIndex, missingFiles, (value.m_Ordering == complex::GeneratedFileListParameter::Ordering::LowToHigh),
                                          value.m_InputPath, value.m_FilePrefix, value.m_FileSuffix, value.m_FileExtension, value.m_PaddingDigits);
  // Validate that they all exist
  for(auto& currentFilePath : fileList)
  {
    if(!fs::exists(currentFilePath))
    {
      results.errors().push_back({-2, fmt::format("FILE DOES NOT EXIST:{}", currentFilePath)});
    }
  }
  return results;
}

} // namespace complex
