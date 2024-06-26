#include "GeneratedFileListParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/TypeTraits.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <type_traits>

namespace fs = std::filesystem;

namespace nx::core
{
namespace
{
using OrderingUnderlyingT = std::underlying_type_t<GeneratedFileListParameter::Ordering>;

constexpr StringLiteral k_StartIndex = "start_index";
constexpr StringLiteral k_EndIndex = "end_index";
constexpr StringLiteral k_PaddingDigits = "padding_digits";
constexpr StringLiteral k_Ordering = "ordering";
constexpr StringLiteral k_IncrementIndex = "increment_index";
constexpr StringLiteral k_InputPath = "input_path";
constexpr StringLiteral k_FilePrefix = "file_prefix";
constexpr StringLiteral k_FileSuffix = "file_suffix";
constexpr StringLiteral k_FileExtension = "file_extension";
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
  const auto& data = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[k_StartIndex] = data.startIndex;
  json[k_EndIndex] = data.endIndex;
  json[k_PaddingDigits] = data.paddingDigits;
  json[k_Ordering] = to_underlying(data.ordering);
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
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Object, fmt::format("{}The JSON data entry for key '{}' is not an object.", prefix.view(), name()));
  }

  std::vector<const char*> keys = {k_StartIndex.c_str(), k_EndIndex.c_str(), k_PaddingDigits.c_str(), k_Ordering.c_str(), k_IncrementIndex.c_str()};
  for(const char* key : keys)
  {
    if(!json.contains(key))
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", prefix.view(), key));
    }
    if(!json[key].is_number_integer())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Integer, fmt::format("{}JSON value for key '{}' is not an integer", prefix.view(), key));
    }
  }

  keys = {k_InputPath.c_str(), k_FilePrefix.c_str(), k_FileSuffix.c_str(), k_FileExtension.c_str()};
  for(const char* key : keys)
  {
    if(!json.contains(key))
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", prefix.view(), key));
    }
    if(!json[key].is_string())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not a string", prefix.view(), key));
    }
  }

  auto ordering_check = json[k_Ordering].get<OrderingUnderlyingT>();
  if(ordering_check != to_underlying(Ordering::LowToHigh) && ordering_check != to_underlying(Ordering::HighToLow))
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Enumeration, fmt::format("{}JSON value for key '{}' was not a valid ordering Value. [{}|{}] allowed.", prefix.view(),
                                                                                                           k_Ordering.view(), to_underlying(Ordering::LowToHigh), to_underlying(Ordering::HighToLow)));
  }

  if(!json[k_PaddingDigits].is_number_unsigned())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Unsigned, fmt::format("{}JSON value for key '{}' is not an unsigned int", prefix.view(), k_PaddingDigits.view()));
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

  return {{std::move(value)}};
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
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  const auto& value = GetAnyRef<ValueType>(valueRef);

  if(value.inputPath.empty())
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}Input Path cannot be empty.", prefix));
  }

  if(value.startIndex > value.endIndex)
  {
    return nx::core::MakeErrorResult(-4002, fmt::format("{}startIndex must be less than or equal to endIndex.", prefix));
  }
  // Generate the file list
  auto fileList = value.generate();
  // Validate that they all exist
  std::vector<Error> errors;
  for(const auto& currentFilePath : fileList)
  {
    if(!fs::exists(currentFilePath))
    {
      errors.push_back({-4003, fmt::format("FILE DOES NOT EXIST: '{}'", currentFilePath)});
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {};
}
} // namespace nx::core
