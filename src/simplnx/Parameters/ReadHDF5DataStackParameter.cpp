#include "ReadHDF5DataStackParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace nx::core
{
// -----------------------------------------------------------------------------
ReadHDF5DataStackParameter::ReadHDF5DataStackParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

// -----------------------------------------------------------------------------
Uuid ReadHDF5DataStackParameter::uuid() const
{
  return ParameterTraits<ReadHDF5DataStackParameter>::uuid;
}

// -----------------------------------------------------------------------------
IParameter::AcceptedTypes ReadHDF5DataStackParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType ReadHDF5DataStackParameter::getVersion() const
{
  return 1;
}

//-----------------------------------------------------------------------------
std::any ReadHDF5DataStackParameter::construct(const Arguments& args, const ExecutionContext& executionContext) const
{
  auto value = args.value<ValueType>(name());
  std::filesystem::path absolutePath = executionContext.getAbsolutePath(value.inputPath);
  value.inputPath = absolutePath.string();
  return value;
}

// -----------------------------------------------------------------------------
nlohmann::json ReadHDF5DataStackParameter::toJsonImpl(const std::any& value) const
{
  const auto& dataStackImportInfo = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[ValueType::k_FilePath_Key] = dataStackImportInfo.inputPath;
  json[ValueType::k_PathPrefix_Key] = dataStackImportInfo.pathPrefix;
  json[ValueType::k_PathSuffix_Key] = dataStackImportInfo.pathSuffix;
  json[ValueType::k_StartIndex_Key] = dataStackImportInfo.startIndex;
  json[ValueType::k_EndIndex_Key] = dataStackImportInfo.endIndex;
  json[ValueType::k_PaddingDigits_Key] = dataStackImportInfo.paddingDigits;
  return json;
}

// -----------------------------------------------------------------------------
Result<std::any> ReadHDF5DataStackParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'ReadHDF5DataStackParameter' JSON Error: ";
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(-780, fmt::format("{}JSON value for key '{}' is not an object", prefix, name()));
  }

  std::vector<const char*> stringKeys = {ValueType::k_FilePath_Key.c_str(), ValueType::k_PathPrefix_Key.c_str(), ValueType::k_PathSuffix_Key.c_str()};
  for(const char* key : stringKeys)
  {
    if(!json.contains(key))
    {
      return MakeErrorResult<std::any>(-781, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), key));
    }
    if(!json[key].is_string())
    {
      return MakeErrorResult<std::any>(-782, fmt::format("{}JSON value for key '{}' is not a string", prefix, key));
    }
  }

  std::vector<const char*> unsignedKeys = {ValueType::k_StartIndex_Key.c_str(), ValueType::k_EndIndex_Key.c_str(), ValueType::k_PaddingDigits_Key.c_str()};
  for(const char* key : unsignedKeys)
  {
    if(!json.contains(key))
    {
      return MakeErrorResult<std::any>(-783, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), key));
    }
    if(!json[key].is_number_unsigned())
    {
      return MakeErrorResult<std::any>(-784, fmt::format("{}JSON value for key '{}' is not an unsigned integer", prefix, key));
    }
  }

  ValueType importData;
  importData.inputPath = json[ValueType::k_FilePath_Key.c_str()].get<std::string>();
  importData.pathPrefix = json[ValueType::k_PathPrefix_Key.c_str()].get<std::string>();
  importData.pathSuffix = json[ValueType::k_PathSuffix_Key.c_str()].get<std::string>();
  importData.startIndex = json[ValueType::k_StartIndex_Key.c_str()].get<uint32>();
  importData.endIndex = json[ValueType::k_EndIndex_Key.c_str()].get<uint32>();
  importData.paddingDigits = json[ValueType::k_PaddingDigits_Key.c_str()].get<uint8>();
  return {{std::move(importData)}};
}

// -----------------------------------------------------------------------------
IParameter::UniquePointer ReadHDF5DataStackParameter::clone() const
{
  return std::make_unique<ReadHDF5DataStackParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

// -----------------------------------------------------------------------------
std::any ReadHDF5DataStackParameter::defaultValue() const
{
  return m_DefaultValue;
}

// -----------------------------------------------------------------------------
Result<> ReadHDF5DataStackParameter::validate(const std::any& value) const
{
  const auto& data = GetAnyRef<ValueType>(value);

  if(data.inputPath.empty())
  {
    return MakeErrorResult(-786, "The HDF5 file path is empty. Please select an HDF5 file.");
  }

  fs::path inputPath(data.inputPath);
  if(!fs::exists(inputPath))
  {
    return MakeErrorResult(-787, fmt::format("The HDF5 file '{}' does not exist.", data.inputPath));
  }

  if(data.startIndex > data.endIndex)
  {
    return MakeErrorResult(-788, fmt::format("The start index ({}) must be less than or equal to the end index ({}).", data.startIndex, data.endIndex));
  }

  HDF5::FileIO h5FileReader = HDF5::FileIO::ReadFile(inputPath);
  if(!h5FileReader.isValid())
  {
    return MakeErrorResult(-789, fmt::format("The file '{}' could not be opened as an HDF5 file.", data.inputPath));
  }

  for(uint32 index = data.startIndex; index <= data.endIndex; ++index)
  {
    std::string datasetPath = fmt::format("{}{:0{}}{}", data.pathPrefix, index, data.paddingDigits, data.pathSuffix);
    if(!h5FileReader.exists(datasetPath))
    {
      return MakeErrorResult(-790, fmt::format("The dataset '{}' does not exist in the HDF5 file '{}'.", datasetPath, data.inputPath));
    }
  }

  return {};
}
} // namespace nx::core
