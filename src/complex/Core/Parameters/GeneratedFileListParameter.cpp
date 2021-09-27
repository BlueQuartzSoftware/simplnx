#include "GeneratedFileListParameter.hpp"

#include "complex/Core/Parameters/utils/FilePathGenerator.hpp"

#include <filesystem>
#include <type_traits>

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace complex
{
namespace
{
constexpr const char k_StartIndex[] = "startIndex";
constexpr const char k_EndIndex[] = "endIndex";
constexpr const char k_PaddingDigits[] = "paddingDigits";
constexpr const char k_Ordering[] = "ordering";
constexpr const char k_IncrementIndex[] = "incrementIndex";
constexpr const char k_InputPath[] = "inputPath";
constexpr const char k_FilePrefix[] = "filePrefix";
constexpr const char k_FileSuffix[] = "fileSuffix";
constexpr const char k_FileExtension[] = "fileExtension";
} // namespace

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
  json[k_StartIndex] = data.startIndex;
  json[k_EndIndex] = data.endIndex;
  json[k_PaddingDigits] = data.paddingDigits;
  json[k_Ordering] = static_cast<std::underlying_type_t<Ordering>>(data.ordering);
  json[k_IncrementIndex] = data.incrementIndex;
  json[k_InputPath] = data.inputPath;
  json[k_FilePrefix] = data.filePrefix;
  json[k_FileSuffix] = data.fileSuffix;
  json[k_FileExtension] = data.fileExtension;
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
        fmt::format("{}JSON value for key \"{}\" was not a valid ordering Value. [{}|{}] allowed.", prefix, k_Ordering, Ordering::LowToHigh, Ordering::HighToLow));
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

  value.paddingDigits = json[k_PaddingDigits].get<int32_t>();
  value.ordering = static_cast<Ordering>(json[k_Ordering].get<uint32_t>());
  value.incrementIndex = json[k_IncrementIndex].get<int32_t>();
  value.inputPath = json[k_InputPath].get<std::string>();
  value.filePrefix = json[k_FilePrefix].get<std::string>();
  value.fileSuffix = json[k_FileSuffix].get<std::string>();
  value.fileExtension = json[k_FileExtension].get<std::string>();
  value.startIndex = json[k_StartIndex].get<int32_t>();
  value.endIndex = json[k_EndIndex].get<int32_t>();

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
  Result<> result;

  auto value = std::any_cast<ValueType>(valueRef);
  if(value.startIndex < value.endIndex)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "startIndex must be greater than endIndex"}})};
  }
  if(value.ordering != Ordering::LowToHigh && value.ordering != Ordering::HighToLow)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "ordering must be ZERO (0: Low To High) or ONE (1: High To Low)"}})};
  }
  bool missingFiles = false;
  // Generate the file lsit
  std::vector<std::string> fileList =
      FilePathGenerator::GenerateFileList(value.startIndex, value.endIndex, value.incrementIndex, missingFiles, (value.ordering == complex::GeneratedFileListParameter::Ordering::LowToHigh),
                                          value.inputPath, value.filePrefix, value.fileSuffix, value.fileExtension, value.paddingDigits);
  // Validate that they all exist
  for(auto& currentFilePath : fileList)
  {
    if(!fs::exists(currentFilePath))
    {
      result.errors().push_back({-2, fmt::format("FILE DOES NOT EXIST:{}", currentFilePath)});
    }
  }
  return result;
}
} // namespace complex
