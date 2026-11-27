#include "ReadH5EbsdFileParameter.h"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"

#include <EbsdLib/IO/H5EbsdVolumeInfo.h>
#include <EbsdLib/IO/HKL/CtfFields.h>
#include <EbsdLib/IO/TSL/AngFields.h>

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace nx::core
{
namespace
{
constexpr StringLiteralType k_InputFilePath = "input_file_path";
constexpr StringLiteralType k_StartSlice = "start_slice";
constexpr StringLiteralType k_EndSlice = "end_slice";
constexpr StringLiteralType k_UseRecommendedTransform = "use_recommended_transform";
constexpr StringLiteralType k_EulerRepresentation = "euler_representation";
constexpr StringLiteralType k_HDF5DataPaths = "hdf5_data_paths";

} // namespace

//-----------------------------------------------------------------------------
ReadH5EbsdFileParameter::ReadH5EbsdFileParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(std::move(defaultValue))
{
}

//-----------------------------------------------------------------------------
Uuid ReadH5EbsdFileParameter::uuid() const
{
  return ParameterTraits<ReadH5EbsdFileParameter>::uuid;
}

//-----------------------------------------------------------------------------
IParameter::AcceptedTypesType ReadH5EbsdFileParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType ReadH5EbsdFileParameter::getVersion() const
{
  return 1;
}

//-----------------------------------------------------------------------------
nlohmann::json ReadH5EbsdFileParameter::toJsonImpl(const std::any& value) const
{
  const auto& data = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[k_InputFilePath] = data.input_file_path;
  json[k_StartSlice] = data.start_slice;
  json[k_EndSlice] = data.end_slice;
  json[k_EulerRepresentation] = data.euler_representation;
  json[k_UseRecommendedTransform] = data.use_recommended_transform;

  // DataPaths
  if(!data.selected_array_names.empty())
  {
    nlohmann::json dataPathsJson = nlohmann::json::array();
    for(const auto& dataPath : data.selected_array_names)
    {
      dataPathsJson.push_back(dataPath);
    }
    json[k_HDF5DataPaths] = std::move(dataPathsJson);
  }
  else
  {
    json[k_HDF5DataPaths] = nullptr;
  }

  return json;
}

//-----------------------------------------------------------------------------
Result<std::any> ReadH5EbsdFileParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'ReadH5EbsdFileParameter' Error: ";

  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotObject, fmt::format("{}The JSON data entry for key '{}' is not an object.", k_Prefix.view(), name()));
  }
  // Validate numeric json entries
  std::vector<const char*> keys = {k_StartSlice.c_str(), k_EndSlice.c_str(), k_EulerRepresentation.c_str()};
  for(const char* keyPtr : keys)
  {
    if(!json.contains(keyPtr))
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonMissingEntry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", k_Prefix.view(), keyPtr));
    }
    if(!json[keyPtr].is_number_integer())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotInteger, fmt::format("{}JSON value for key '{}' is not an integer", k_Prefix.view(), keyPtr));
    }
  }

  keys = {k_InputFilePath.c_str()};
  for(const char* keyPtr : keys)
  {
    if(!json.contains(keyPtr))
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonMissingEntry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", k_Prefix.view(), keyPtr));
    }
    if(!json[keyPtr].is_string())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}JSON value for key '{}' is not a string", k_Prefix.view(), keyPtr));
    }
  }

  keys = {k_UseRecommendedTransform.c_str()};
  for(const char* keyPtr : keys)
  {
    if(!json.contains(keyPtr))
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonMissingEntry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", k_Prefix.view(), keyPtr));
    }
    if(!json[keyPtr].is_boolean())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}JSON value for key '{}' is not a boolean", k_Prefix.view(), keyPtr));
    }
  }

  auto orderingCheck = json[k_EulerRepresentation].get<int32>();
  if(orderingCheck != ebsdlib::AngleRepresentation::Radians && orderingCheck != ebsdlib::AngleRepresentation::Degrees)
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotEnumeration,
                                     fmt::format("{}JSON value for key '{}' was not a valid ordering Value. [{}|{}] allowed.", k_Prefix.view(), k_UseRecommendedTransform.view(),
                                                 ebsdlib::AngleRepresentation::Radians, ebsdlib::AngleRepresentation::Degrees));
  }

  ValueType value;

  value.input_file_path = json[k_InputFilePath].get<std::string>();
  value.start_slice = json[k_StartSlice].get<int32>();
  value.end_slice = json[k_EndSlice].get<int32>();
  value.euler_representation = json[k_EulerRepresentation].get<int32>();
  value.use_recommended_transform = json[k_UseRecommendedTransform].get<bool>();

  const auto& jsonDataPaths = json[k_HDF5DataPaths];
  if(jsonDataPaths.is_null())
  {
    value.selected_array_names = {};
  }
  else
  {
    if(!jsonDataPaths.is_array())
    {
      return MakeErrorResult<std::any>(-6054, fmt::format("{}JSON value for key '{} / {}' is not an array", k_Prefix, name(), k_HDF5DataPaths));
    }
    auto dataPathStrings = jsonDataPaths.get<std::vector<std::string>>();
    std::vector<std::string> dataPaths;
    std::vector<Error> errors;
    for(const auto& dataPath : dataPathStrings)
    {
      if(dataPath.empty())
      {
        errors.push_back(Error{.code = filter_parameter::constants::k_JsonValueNotValueType, .message = fmt::format("{}Failed to parse '{}' as HDF5 DataPath", k_Prefix.view(), dataPath)});
        continue;
      }
      dataPaths.push_back(dataPath);
    }

    if(!errors.empty())
    {
      return {{nonstd::make_unexpected(std::move(errors))}};
    }

    value.selected_array_names = std::move(dataPaths);
  }

  return {{std::move(value)}};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointerType ReadH5EbsdFileParameter::clone() const
{
  return std::make_unique<ReadH5EbsdFileParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

//-----------------------------------------------------------------------------
std::any ReadH5EbsdFileParameter::defaultValue() const
{
  return m_DefaultValue;
}

//-----------------------------------------------------------------------------
std::any ReadH5EbsdFileParameter::construct(const Arguments& args, const ExecutionContext& executionContext) const
{
  auto value = args.value<ValueType>(name());
  std::filesystem::path absolutePath = executionContext.getAbsolutePath(value.input_file_path);
  value.input_file_path = absolutePath.string();
  return value;
}

//-----------------------------------------------------------------------------
Result<> ReadH5EbsdFileParameter::validate(const std::any& valueRef) const
{
  std::vector<Error> errors;

  const auto& value = GetAnyRef<ValueType>(valueRef);
  if(value.input_file_path.empty())
  {
    errors.push_back({-2000, "The input file path for the H5EbsdFile was empty."});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  if(!fs::exists(value.input_file_path))
  {
    errors.push_back({-2001, fmt::format("Input file does not exist: '{}'", value.input_file_path)});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  if(value.start_slice > value.end_slice)
  {
    errors.push_back({-2002, fmt::format("StartSlice '{}' must be less than or equal EndSlice '{}'", value.start_slice, value.end_slice)});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  ebsdlib::H5EbsdVolumeInfo::Pointer reader = ebsdlib::H5EbsdVolumeInfo::New();
  reader->setFileName(value.input_file_path);
  int32_t err = reader->readVolumeInfo();
  if(err < 0)
  {
    errors.push_back({-2003, fmt::format("H5Ebsd file '{}' could not be opened. Reported error code from the H5EbsdVolumeInfo class is '{}'", value.input_file_path, err)});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  std::string manufacturerString = reader->getManufacturer();
  if(manufacturerString != ebsdlib::Ang::Manufacturer && manufacturerString != ebsdlib::Ctf::Manufacturer)
  {
    errors.push_back({-2004, fmt::format("Original Data source could not be determined. It should be TSL, HKL or HEDM")});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {};
}

} // namespace nx::core
