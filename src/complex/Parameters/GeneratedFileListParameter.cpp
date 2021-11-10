#include "GeneratedFileListParameter.hpp"

#include "complex/Common/StringLiteral.hpp"

#include <filesystem>
#include <type_traits>

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace complex
{
namespace
{
using OrderingUnderlyingT = std::underlying_type_t<GeneratedFileListParameter::Ordering>;

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
  static constexpr StringLiteral prefix = "FilterParameter 'GeneratedFileListParameter' Error: ";

  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Object, fmt::format("{}The JSON data entry for key \"{}\" is not an object.", prefix.view(), name()));
  }

  std::vector<const char*> keys = {k_StartIndex.c_str(), k_EndIndex.c_str(), k_PaddingDigits.c_str(), k_Ordering.c_str(), k_IncrementIndex.c_str()};
  for(const char* key : keys)
  {
    if(!json.contains(key))
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data does not contain an entry with a key of \"{}\"", prefix.view(), key));
    }
    if(!json[key].is_number_integer())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Integer, fmt::format("{}JSON value for key \"{}\" is not an integer", prefix.view(), key));
    }
  }

  keys = {k_InputPath.c_str(), k_FilePrefix.c_str(), k_FileSuffix.c_str(), k_FileExtension.c_str()};
  for(const char* key : keys)
  {
    if(!json.contains(key))
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data does not contain an entry with a key of \"{}\"", prefix.view(), key));
    }
    if(!json[key].is_string())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key \"{}\" is not a string", prefix.view(), key));
    }
  }

  auto ordering_check = json[k_Ordering].get<OrderingUnderlyingT>();
  if(ordering_check != static_cast<OrderingUnderlyingT>(Ordering::LowToHigh) && ordering_check != static_cast<OrderingUnderlyingT>(Ordering::HighToLow))
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Enumeration, fmt::format("{}JSON value for key \"{}\" was not a valid ordering Value. [{}|{}] allowed.",
                                                                                                           prefix.view(), k_Ordering.view(), Ordering::LowToHigh, Ordering::HighToLow));
  }

  if(!json[k_PaddingDigits].is_number_unsigned())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Unsigned, fmt::format("{}JSON value for key \"{}\" is not an unsigned int", prefix.view(), k_PaddingDigits.view()));
  }

  ValueType value;

  value.paddingDigits = json[k_PaddingDigits].get<int32>();
  value.ordering = static_cast<Ordering>(json[k_Ordering].get<OrderingUnderlyingT>());
  value.incrementIndex = json[k_IncrementIndex].get<int32>();
  value.inputPath = json[k_InputPath].get<std::string>();
  value.filePrefix = json[k_FilePrefix].get<std::string>();
  value.fileSuffix = json[k_FileSuffix].get<std::string>();
  value.fileExtension = json[k_FileExtension].get<std::string>();
  value.startIndex = json[k_StartIndex].get<int32>();
  value.endIndex = json[k_EndIndex].get<int32>();

  return {std::move(value)};
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
  auto value = std::any_cast<ValueType>(valueRef);
  if(value.startIndex >= value.endIndex)
  {
    return MakeErrorResult(-1, "startIndex must be less than endIndex");
  }
  // Generate the file lsit
  auto&& [fileList, missingFiles] = value.generate();
  // Validate that they all exist
  std::vector<Error> errors;
  for(const auto& currentFilePath : fileList)
  {
    if(!fs::exists(currentFilePath))
    {
      errors.push_back({-2, fmt::format("FILE DOES NOT EXIST: \"{}\"", currentFilePath)});
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {};
}
} // namespace complex
