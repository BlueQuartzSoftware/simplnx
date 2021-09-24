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
  json[k_StartIndex] = static_cast<int32_t>(data.StartIndex);
  json[k_EndIndex] = static_cast<int32_t>(data.EndIndex);
  json[k_PaddingDigits] = static_cast<int32_t>(data.PaddingDigits);
  json[k_Ordering] = static_cast<int32_t>(data.Ordering);
  json[k_IncrementIndex] = static_cast<int32_t>(data.IncrementIndex);
  json[k_InputPath] = data.InputPath;
  json[k_FilePrefix] = data.FilePrefix;
  json[k_FileSuffix] = data.FileSuffix;
  json[k_FileExtension] = data.FileExtension;
  return json;
}

//-----------------------------------------------------------------------------
Result<std::any> GeneratedFileListParameter::fromJson(const nlohmann::json& json) const
{
  const std::string prefix("FilterParameter 'GeneratedFileListParameter' Error: ");

  if(!json.contains(name()))
  {
    return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}JSON Data does not contain an entry with a key of \"{}\"", prefix, name()));
  }
  auto jsonValue = json.at(name());
  if(!jsonValue.is_object())
  {
    return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Value_Not_Object,
                                              fmt::format("{}}The JSON data entry for key \"{}\" is not in the form of a JSON Object.", prefix, name()));
  }

  uint32_t ordering_check = json[k_Ordering].get<uint32_t>();
  if(ordering_check != static_cast<uint32_t>(Ordering::LowToHigh) && ordering_check != static_cast<uint32_t>(Ordering::HighToLow))
  {
    return complex::MakeErrorResult<std::any>(
        complex::FilterParameter::Constants::k_Json_Value_Not_Enumeration,
        fmt::format("{}JSON value for key \"{}\" was not a valid Ordering Value. [{}|{}] allowed.", prefix, k_Ordering, Ordering::LowToHigh, Ordering::HighToLow));
  }

  if(!json[k_PaddingDigits].is_number_unsigned())
  {
    return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Value_Not_Unsigned,
                                              fmt::format("{}JSON value for key \"{}\" is not an unsigned int", prefix, k_PaddingDigits));
  }

  std::vector<std::string> keys = {k_StartIndex, k_EndIndex, k_PaddingDigits, k_Ordering, k_IncrementIndex};
  for(const auto& key : keys)
  {
    if(!json[key].is_number_integer())
    {
      return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Value_Not_Integer, fmt::format("{}JSON value for key \"{}\" is not an integer", prefix, key));
    }
  }

  keys = {k_InputPath, k_FilePrefix, k_FileSuffix, k_FileExtension};
  for(const auto& key : keys)
  {
    if(!json[key].is_string())
    {
      return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key \"{}\" is not a string", prefix, key));
    }
  }

  ValueType value;

  value.PaddingDigits = static_cast<int32_t>(json[k_PaddingDigits].get<int32_t>());
  value.Ordering = static_cast<Ordering>(json[k_Ordering].get<uint32_t>());
  value.IncrementIndex = static_cast<int32_t>(json[k_IncrementIndex].get<int32_t>());
  value.InputPath = json[k_InputPath].get<std::string>();
  value.FilePrefix = json[k_FilePrefix].get<std::string>();
  value.FileSuffix = json[k_FileSuffix].get<std::string>();
  value.FileExtension = json[k_FileExtension].get<std::string>();
  value.StartIndex = static_cast<int32_t>(json[k_StartIndex].get<int32_t>());
  value.EndIndex = static_cast<int32_t>(json[k_EndIndex].get<int32_t>());

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
  if(value.StartIndex < value.EndIndex)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "StartIndex must be greater than EndIndex"}})};
  }
  if(value.Ordering != Ordering::LowToHigh && value.Ordering != Ordering::HighToLow)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "Ordering must be ZERO (0: Low To High) or ONE (1: High To Low)"}})};
  }
  bool missingFiles = false;
  // Generate the file lsit
  std::vector<std::string> fileList =
      FilePathGenerator::GenerateFileList(value.StartIndex, value.EndIndex, value.IncrementIndex, missingFiles, (value.Ordering == complex::GeneratedFileListParameter::Ordering::LowToHigh),
                                          value.InputPath, value.FilePrefix, value.FileSuffix, value.FileExtension, value.PaddingDigits);
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
