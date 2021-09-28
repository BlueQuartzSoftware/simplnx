#include "GeneratedFileListParameter.hpp"

#include "complex/Common/StringLiteral.hpp"
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
constexpr StringLiteral k_StartIndex = "startIndex";
constexpr StringLiteral k_EndIndex = "endIndex";
constexpr StringLiteral k_PaddingDigits = "paddingDigits";
constexpr StringLiteral k_Ordering = "ordering";
constexpr StringLiteral k_IncrementIndex = "incrementIndex";
constexpr StringLiteral k_InputPath = "inputPath";
constexpr StringLiteral k_FilePrefix = "filePrefix";
constexpr StringLiteral k_FileSuffix = "fileSuffix";
constexpr StringLiteral k_FileExtension = "fileExtension";
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
  json[k_StartIndex.c_str()] = data.startIndex;
  json[k_EndIndex.c_str()] = data.endIndex;
  json[k_PaddingDigits.c_str()] = data.paddingDigits;
  json[k_Ordering.c_str()] = static_cast<std::underlying_type_t<Ordering>>(data.ordering);
  json[k_IncrementIndex.c_str()] = data.incrementIndex;
  json[k_InputPath.c_str()] = data.inputPath;
  json[k_FilePrefix.c_str()] = data.filePrefix;
  json[k_FileSuffix.c_str()] = data.fileSuffix;
  json[k_FileExtension.c_str()] = data.fileExtension;
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

  uint32 ordering_check = json[k_Ordering.c_str()].get<uint32>();
  if(ordering_check != static_cast<uint32>(Ordering::LowToHigh) && ordering_check != static_cast<uint32>(Ordering::HighToLow))
  {
    return complex::MakeErrorResult<std::any>(
        complex::FilterParameter::Constants::k_Json_Value_Not_Enumeration,
        fmt::format("{}JSON value for key \"{}\" was not a valid ordering Value. [{}|{}] allowed.", prefix, k_Ordering.view(), Ordering::LowToHigh, Ordering::HighToLow));
  }

  if(!json[k_PaddingDigits.c_str()].is_number_unsigned())
  {
    return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Value_Not_Unsigned,
                                              fmt::format("{}JSON value for key \"{}\" is not an unsigned int", prefix, k_PaddingDigits.view()));
  }

  std::vector<const char*> keys = {k_StartIndex.c_str(), k_EndIndex.c_str(), k_PaddingDigits.c_str(), k_Ordering.c_str(), k_IncrementIndex.c_str()};
  for(const auto& key : keys)
  {
    if(!json[key].is_number_integer())
    {
      return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Value_Not_Integer, fmt::format("{}JSON value for key \"{}\" is not an integer", prefix, key));
    }
  }

  keys = {k_InputPath.c_str(), k_FilePrefix.c_str(), k_FileSuffix.c_str(), k_FileExtension.c_str()};
  for(const auto& key : keys)
  {
    if(!json[key].is_string())
    {
      return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key \"{}\" is not a string", prefix, key));
    }
  }

  ValueType value;

  value.paddingDigits = json[k_PaddingDigits.c_str()].get<int32>();
  value.ordering = static_cast<Ordering>(json[k_Ordering.c_str()].get<uint32>());
  value.incrementIndex = json[k_IncrementIndex.c_str()].get<int32>();
  value.inputPath = json[k_InputPath.c_str()].get<std::string>();
  value.filePrefix = json[k_FilePrefix.c_str()].get<std::string>();
  value.fileSuffix = json[k_FileSuffix.c_str()].get<std::string>();
  value.fileExtension = json[k_FileExtension.c_str()].get<std::string>();
  value.startIndex = json[k_StartIndex.c_str()].get<int32>();
  value.endIndex = json[k_EndIndex.c_str()].get<int32>();

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
  // Generate the file lsit
  auto&& [fileList, missingFiles] = FilePathGenerator::GenerateFileList(value.startIndex, value.endIndex, value.incrementIndex, value.ordering, value.inputPath, value.filePrefix, value.fileSuffix,
                                                                        value.fileExtension, value.paddingDigits);
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
