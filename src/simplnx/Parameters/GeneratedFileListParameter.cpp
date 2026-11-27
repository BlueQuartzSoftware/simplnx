#include "GeneratedFileListParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Common/TypeTraits.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <type_traits>
#include <utility>

namespace fs = std::filesystem;

namespace nx::core
{
namespace
{
using OrderingUnderlyingTType = std::underlying_type_t<GeneratedFileListParameter::Ordering>;

constexpr StringLiteralType k_StartIndex = "start_index";
constexpr StringLiteralType k_EndIndex = "end_index";
constexpr StringLiteralType k_PaddingDigits = "padding_digits";
constexpr StringLiteralType k_Ordering = "ordering";
constexpr StringLiteralType k_IncrementIndex = "increment_index";
constexpr StringLiteralType k_InputPath = "input_path";
constexpr StringLiteralType k_FilePrefix = "file_prefix";
constexpr StringLiteralType k_FileSuffix = "file_suffix";
constexpr StringLiteralType k_FileExtension = "file_extension";
} // namespace

//-----------------------------------------------------------------------------
GeneratedFileListParameter::GeneratedFileListParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(std::move(defaultValue))
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

//------------------------------------------------------------------------------
IParameter::VersionType GeneratedFileListParameter::getVersion() const
{
  return 1;
}

//-----------------------------------------------------------------------------
nlohmann::json GeneratedFileListParameter::toJsonImpl(const std::any& value) const
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
Result<std::any> GeneratedFileListParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'GeneratedFileListParameter' Error: ";

  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Object, fmt::format("{}The JSON data entry for key '{}' is not an object.", k_Prefix.view(), name()));
  }

  std::vector<const char*> keys = {k_StartIndex.c_str(), k_EndIndex.c_str(), k_PaddingDigits.c_str(), k_Ordering.c_str(), k_IncrementIndex.c_str()};
  for(const char* keyPtr : keys)
  {
    if(!json.contains(keyPtr))
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", k_Prefix.view(), keyPtr));
    }
    if(!json[keyPtr].is_number_integer())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Integer, fmt::format("{}JSON value for key '{}' is not an integer", k_Prefix.view(), keyPtr));
    }
  }

  keys = {k_InputPath.c_str(), k_FilePrefix.c_str(), k_FileSuffix.c_str(), k_FileExtension.c_str()};
  for(const char* keyPtr : keys)
  {
    if(!json.contains(keyPtr))
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", k_Prefix.view(), keyPtr));
    }
    if(!json[keyPtr].is_string())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not a string", k_Prefix.view(), keyPtr));
    }
  }

  auto orderingCheck = json[k_Ordering].get<OrderingUnderlyingTType>();
  if(orderingCheck != to_underlying(Ordering::LowToHigh) && orderingCheck != to_underlying(Ordering::HighToLow))
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Enumeration,
                                     fmt::format("{}JSON value for key '{}' was not a valid ordering Value. [{}|{}] allowed.", k_Prefix.view(), k_Ordering.view(), to_underlying(Ordering::LowToHigh),
                                                 to_underlying(Ordering::HighToLow)));
  }

  if(!json[k_PaddingDigits].is_number_unsigned())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Unsigned, fmt::format("{}JSON value for key '{}' is not an unsigned int", k_Prefix.view(), k_PaddingDigits.view()));
  }

  ValueType value;

  value.paddingDigits = json[k_PaddingDigits].get<int32>();
  value.ordering = static_cast<Ordering>(json[k_Ordering].get<OrderingUnderlyingTType>());
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
IParameter::UniquePointerType GeneratedFileListParameter::clone() const
{
  return std::make_unique<GeneratedFileListParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

//-----------------------------------------------------------------------------
std::any GeneratedFileListParameter::defaultValue() const
{
  return m_DefaultValue;
}

//-----------------------------------------------------------------------------
std::any GeneratedFileListParameter::construct(const Arguments& args, const ExecutionContext& executionContext) const
{
  auto value = args.value<ValueType>(name());
  std::filesystem::path absolutePath = executionContext.getAbsolutePath(value.inputPath);
  value.inputPath = absolutePath.string();
  return value;
}

//-----------------------------------------------------------------------------
Result<> GeneratedFileListParameter::validate(const std::any& valueRef) const
{
  try
  {
    const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

    const auto& value = GetAnyRef<ValueType>(valueRef);

    if(value.inputPath.empty())
    {
      return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}Input Path cannot be empty.", prefix));
    }

    if(value.startIndex > value.endIndex)
    {
      return nx::core::MakeErrorResult(-4002, fmt::format("{}startIndex ({}) must be less than or equal to endIndex ({}).", prefix, value.startIndex, value.endIndex));
    }
    // Generate the file list
    auto fileList = value.generate();
    // Validate that they all exist
    std::vector<Error> errors;
    for(const auto& currentFilePath : fileList)
    {
      if(!fs::exists(currentFilePath))
      {
        errors.push_back({-4003, fmt::format("{}FILE DOES NOT EXIST: '{}'", prefix, currentFilePath)});
      }
    }

    if(!errors.empty())
    {
      return {nonstd::make_unexpected(std::move(errors))};
    }
  } catch(const fs::filesystem_error& exception)
  {
    return MakeErrorResult(-4004, fmt::format("Filesystem exception: {}", exception.what()));
  }
  return {};
}

namespace simpl_conversion
{
namespace
{
constexpr StringLiteralType k_EndIndex = "EndIndex";
constexpr StringLiteralType k_FileExtension = "FileExtension";
constexpr StringLiteralType k_FilePrefix = "FilePrefix";
constexpr StringLiteralType k_FileSuffix = "FileSuffix";
constexpr StringLiteralType k_IncrementIndex = "IncrementIndex";
constexpr StringLiteralType k_InputPath = "InputPath";
constexpr StringLiteralType k_Ordering = "Ordering";
constexpr StringLiteralType k_PaddingDigits = "PaddingDigits";
constexpr StringLiteralType k_StartIndex = "StartIndex";
} // namespace

Result<FileListInfoFilterParameterConverter::ValueType> FileListInfoFilterParameterConverter::Convert(const nlohmann::json& json)
{
  if(!json.contains(k_EndIndex))
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_EndIndex));
  }

  if(!json[k_EndIndex].is_number_integer())
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an integer '{}'", json.dump(), k_EndIndex));
  }

  ValueType value;
  value.endIndex = json[k_EndIndex].get<int32>();

  {
    if(!json.contains(k_FileExtension))
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_FileExtension));
    }

    if(!json[k_FileExtension].is_string())
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("FileListInfoFilterParameterConverter json '{}' is not a string '{}'", json.dump(), k_FileExtension));
    }

    value.fileExtension = json[k_FileExtension].get<std::string>();
  }

  {
    if(!json.contains(k_FilePrefix))
    {
      return MakeErrorResult<ValueType>(-5, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_FilePrefix));
    }

    if(!json[k_FilePrefix].is_string())
    {
      return MakeErrorResult<ValueType>(-6, fmt::format("FileListInfoFilterParameterConverter json '{}' is not a string '{}'", json.dump(), k_FilePrefix));
    }

    value.filePrefix = json[k_FilePrefix].get<std::string>();
  }

  {
    if(!json.contains(k_FileSuffix))
    {
      return MakeErrorResult<ValueType>(-7, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_FileSuffix));
    }

    if(!json[k_FileSuffix].is_string())
    {
      return MakeErrorResult<ValueType>(-8, fmt::format("FileListInfoFilterParameterConverter json '{}' is not a string '{}'", json.dump(), k_FileSuffix));
    }

    value.fileSuffix = json[k_FileSuffix].get<std::string>();
  }

  {
    if(!json.contains(k_IncrementIndex))
    {
      return MakeErrorResult<ValueType>(-9, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_IncrementIndex));
    }

    if(!json[k_IncrementIndex].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-10, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an integer '{}'", json.dump(), k_IncrementIndex));
    }

    value.incrementIndex = json[k_IncrementIndex].get<int32>();
  }

  {
    if(!json.contains(k_InputPath))
    {
      return MakeErrorResult<ValueType>(-11, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_InputPath));
    }

    if(!json[k_InputPath].is_string())
    {
      return MakeErrorResult<ValueType>(-12, fmt::format("FileListInfoFilterParameterConverter json '{}' is not a string '{}'", json.dump(), k_InputPath));
    }

    value.inputPath = json[k_InputPath].get<std::string>();
  }

  {
    if(!json.contains(k_Ordering))
    {
      return MakeErrorResult<ValueType>(-13, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_Ordering));
    }

    if(!json[k_Ordering].is_number_unsigned())
    {
      return MakeErrorResult<ValueType>(-14, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an unsigned integer '{}'", json.dump(), k_Ordering));
    }

    value.ordering = static_cast<ParameterType::Ordering>(json[k_Ordering].get<uint8>());
  }

  {
    if(!json.contains(k_PaddingDigits))
    {
      return MakeErrorResult<ValueType>(-15, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_PaddingDigits));
    }

    if(!json[k_PaddingDigits].is_number_unsigned())
    {
      return MakeErrorResult<ValueType>(-16, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an unsigned integer '{}'", json.dump(), k_PaddingDigits));
    }

    value.paddingDigits = json[k_PaddingDigits].get<uint32>();
  }

  {
    if(!json.contains(k_StartIndex))
    {
      return MakeErrorResult<ValueType>(-17, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_StartIndex));
    }

    if(!json[k_StartIndex].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-18, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an integer '{}'", json.dump(), k_StartIndex));
    }

    value.startIndex = json[k_StartIndex].get<int32>();
  }

  return {std::move(value)};
}
} // namespace simpl_conversion
} // namespace nx::core
