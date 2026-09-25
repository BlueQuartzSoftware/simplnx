#include "ReadHDF5FileListParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <type_traits>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
using OrderingUnderlyingT = std::underlying_type_t<ReadHDF5FileListParameter::Ordering>;
}

namespace nx::core
{
// -----------------------------------------------------------------------------
ReadHDF5FileListParameter::ReadHDF5FileListParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

// -----------------------------------------------------------------------------
Uuid ReadHDF5FileListParameter::uuid() const
{
  return ParameterTraits<ReadHDF5FileListParameter>::uuid;
}

// -----------------------------------------------------------------------------
IParameter::AcceptedTypes ReadHDF5FileListParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType ReadHDF5FileListParameter::getVersion() const
{
  return 1;
}

//-----------------------------------------------------------------------------
std::any ReadHDF5FileListParameter::construct(const Arguments& args, const ExecutionContext& executionContext) const
{
  auto value = args.value<ValueType>(name());
  std::filesystem::path absolutePath = executionContext.getAbsolutePath(value.inputPath);
  value.inputPath = absolutePath.string();
  return value;
}

// -----------------------------------------------------------------------------
nlohmann::json ReadHDF5FileListParameter::toJsonImpl(const std::any& value) const
{
  const auto& data = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[ValueType::k_InputPath_Key] = data.inputPath;
  json[ValueType::k_FilePrefix_Key] = data.filePrefix;
  json[ValueType::k_FileSuffix_Key] = data.fileSuffix;
  json[ValueType::k_FileExtension_Key] = data.fileExtension;
  json[ValueType::k_StartIndex_Key] = data.startIndex;
  json[ValueType::k_EndIndex_Key] = data.endIndex;
  json[ValueType::k_IncrementIndex_Key] = data.incrementIndex;
  json[ValueType::k_PaddingDigits_Key] = data.paddingDigits;
  json[ValueType::k_Ordering_Key] = to_underlying(data.ordering);
  json[ValueType::k_DatasetPath_Key] = data.datasetPath;
  return json;
}

// -----------------------------------------------------------------------------
Result<std::any> ReadHDF5FileListParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'ReadHDF5FileListParameter' JSON Error: ";

  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(-791, fmt::format("{}JSON value for key '{}' is not an object", prefix, name()));
  }

  std::vector<const char*> stringKeys = {ValueType::k_InputPath_Key.c_str(), ValueType::k_FilePrefix_Key.c_str(), ValueType::k_FileSuffix_Key.c_str(), ValueType::k_FileExtension_Key.c_str(),
                                         ValueType::k_DatasetPath_Key.c_str()};
  for(const char* key : stringKeys)
  {
    if(!json.contains(key))
    {
      return MakeErrorResult<std::any>(-792, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), key));
    }
    if(!json[key].is_string())
    {
      return MakeErrorResult<std::any>(-793, fmt::format("{}JSON value for key '{}' is not a string", prefix, key));
    }
  }

  std::vector<const char*> integerKeys = {ValueType::k_StartIndex_Key.c_str(), ValueType::k_EndIndex_Key.c_str(), ValueType::k_IncrementIndex_Key.c_str()};
  for(const char* key : integerKeys)
  {
    if(!json.contains(key))
    {
      return MakeErrorResult<std::any>(-794, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), key));
    }
    if(!json[key].is_number_integer())
    {
      return MakeErrorResult<std::any>(-795, fmt::format("{}JSON value for key '{}' is not an integer", prefix, key));
    }
  }

  if(!json.contains(ValueType::k_PaddingDigits_Key.c_str()))
  {
    return MakeErrorResult<std::any>(-796, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), ValueType::k_PaddingDigits_Key.view()));
  }
  if(!json[ValueType::k_PaddingDigits_Key.c_str()].is_number_unsigned())
  {
    return MakeErrorResult<std::any>(-797, fmt::format("{}JSON value for key '{}' is not an unsigned integer", prefix, ValueType::k_PaddingDigits_Key.view()));
  }

  if(!json.contains(ValueType::k_Ordering_Key.c_str()))
  {
    return MakeErrorResult<std::any>(-798, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), ValueType::k_Ordering_Key.view()));
  }
  auto orderingCheck = json[ValueType::k_Ordering_Key.c_str()].get<OrderingUnderlyingT>();
  if(orderingCheck != to_underlying(Ordering::LowToHigh) && orderingCheck != to_underlying(Ordering::HighToLow))
  {
    return MakeErrorResult<std::any>(-799, fmt::format("{}JSON value for key '{}' was not a valid ordering value. [{}|{}] allowed.", prefix, ValueType::k_Ordering_Key.view(),
                                                        to_underlying(Ordering::LowToHigh), to_underlying(Ordering::HighToLow)));
  }

  ValueType value;
  value.inputPath = json[ValueType::k_InputPath_Key.c_str()].get<std::string>();
  value.filePrefix = json[ValueType::k_FilePrefix_Key.c_str()].get<std::string>();
  value.fileSuffix = json[ValueType::k_FileSuffix_Key.c_str()].get<std::string>();
  value.fileExtension = json[ValueType::k_FileExtension_Key.c_str()].get<std::string>();
  value.datasetPath = json[ValueType::k_DatasetPath_Key.c_str()].get<std::string>();
  value.startIndex = json[ValueType::k_StartIndex_Key.c_str()].get<int32>();
  value.endIndex = json[ValueType::k_EndIndex_Key.c_str()].get<int32>();
  value.incrementIndex = json[ValueType::k_IncrementIndex_Key.c_str()].get<int32>();
  value.paddingDigits = json[ValueType::k_PaddingDigits_Key.c_str()].get<uint32>();
  value.ordering = static_cast<Ordering>(orderingCheck);

  return {{std::move(value)}};
}

// -----------------------------------------------------------------------------
IParameter::UniquePointer ReadHDF5FileListParameter::clone() const
{
  return std::make_unique<ReadHDF5FileListParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

// -----------------------------------------------------------------------------
std::any ReadHDF5FileListParameter::defaultValue() const
{
  return m_DefaultValue;
}

// -----------------------------------------------------------------------------
Result<> ReadHDF5FileListParameter::validate(const std::any& valueRef) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());
  const auto& value = GetAnyRef<ValueType>(valueRef);

  if(value.inputPath.empty())
  {
    return MakeErrorResult(-4101, fmt::format("{}Input Path cannot be empty.", prefix));
  }

  if(value.datasetPath.empty())
  {
    return MakeErrorResult(-4102, fmt::format("{}Dataset Path cannot be empty.", prefix));
  }

  if(value.startIndex > value.endIndex)
  {
    return MakeErrorResult(-4103, fmt::format("{}Start Index ({}) must be less than or equal to End Index ({}).", prefix, value.startIndex, value.endIndex));
  }

  try
  {
    std::vector<std::string> fileList = value.generate();
    if(fileList.empty())
    {
      return MakeErrorResult(-4104, fmt::format("{}No files were generated from the given start/end index range.", prefix));
    }

    std::vector<Error> errors;
    for(const auto& filePath : fileList)
    {
      if(!fs::exists(filePath))
      {
        errors.push_back({-4105, fmt::format("{}FILE DOES NOT EXIST: '{}'", prefix, filePath)});
        continue;
      }

      HDF5::FileIO h5FileReader = HDF5::FileIO::ReadFile(filePath);
      if(!h5FileReader.isValid())
      {
        errors.push_back({-4106, fmt::format("{}'{}' could not be opened as an HDF5 file.", prefix, filePath)});
        continue;
      }

      if(!h5FileReader.exists(value.datasetPath))
      {
        errors.push_back({-4107, fmt::format("{}The dataset '{}' does not exist in HDF5 file '{}'.", prefix, value.datasetPath, filePath)});
      }
    }

    if(!errors.empty())
    {
      return {nonstd::make_unexpected(std::move(errors))};
    }
  } catch(const fs::filesystem_error& exception)
  {
    return MakeErrorResult(-4108, fmt::format("{}Filesystem exception: {}", prefix, exception.what()));
  }

  return {};
}
} // namespace nx::core
