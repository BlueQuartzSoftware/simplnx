#include "ReadH5EbsdFileParameter.h"

#include "complex/Common/Any.hpp"
#include "complex/Common/StringLiteral.hpp"

#include "EbsdLib/IO/H5EbsdVolumeInfo.h"
#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/TSL/AngFields.h"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace complex
{
namespace
{
constexpr StringLiteral k_InputFilePath = "input_file_path";
constexpr StringLiteral k_StartSlice = "start_slice";
constexpr StringLiteral k_EndSlice = "end_slice";
constexpr StringLiteral k_UseRecommendedTransform = "use_recommended_transform";
constexpr StringLiteral k_EulerRepresentation = "euler_representation";
constexpr StringLiteral k_HDF5DataPaths = "hdf5_data_paths";

} // namespace

//-----------------------------------------------------------------------------
ReadH5EbsdFileParameter::ReadH5EbsdFileParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

//-----------------------------------------------------------------------------
Uuid ReadH5EbsdFileParameter::uuid() const
{
  return ParameterTraits<ReadH5EbsdFileParameter>::uuid;
}

//-----------------------------------------------------------------------------
IParameter::AcceptedTypes ReadH5EbsdFileParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//-----------------------------------------------------------------------------
nlohmann::json ReadH5EbsdFileParameter::toJson(const std::any& value) const
{
  const auto& data = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[k_InputFilePath] = data.inputFilePath;
  json[k_StartSlice] = data.startSlice;
  json[k_EndSlice] = data.endSlice;
  json[k_EulerRepresentation] = data.eulerRepresentation;
  json[k_UseRecommendedTransform] = data.useRecommendedTransform;

  // DataPaths
  if(!data.selectedArrayNames.empty())
  {
    nlohmann::json dataPathsJson = nlohmann::json::array();
    for(const auto& dataPath : data.selectedArrayNames)
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
Result<std::any> ReadH5EbsdFileParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'ReadH5EbsdFileParameter' Error: ";

  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Object, fmt::format("{}The JSON data entry for key '{}' is not an object.", prefix.view(), name()));
  }
  // Validate numeric json entries
  std::vector<const char*> keys = {k_StartSlice.c_str(), k_EndSlice.c_str(), k_EulerRepresentation.c_str()};
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

  keys = {k_InputFilePath.c_str()};
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

  keys = {k_UseRecommendedTransform.c_str()};
  for(const char* key : keys)
  {
    if(!json.contains(key))
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", prefix.view(), key));
    }
    if(!json[key].is_boolean())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not a string", prefix.view(), key));
    }
  }

  auto ordering_check = json[k_EulerRepresentation].get<int32>();
  if(ordering_check != EbsdLib::AngleRepresentation::Radians && ordering_check != EbsdLib::AngleRepresentation::Degrees)
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Enumeration,
                                     fmt::format("{}JSON value for key '{}' was not a valid ordering Value. [{}|{}] allowed.", prefix.view(), k_UseRecommendedTransform.view(),
                                                 EbsdLib::AngleRepresentation::Radians, EbsdLib::AngleRepresentation::Degrees));
  }

  ValueType value;

  value.inputFilePath = json[k_InputFilePath].get<std::string>();
  value.startSlice = json[k_StartSlice].get<int32>();
  value.endSlice = json[k_EndSlice].get<int32>();
  value.eulerRepresentation = json[k_EulerRepresentation].get<int32>();
  value.useRecommendedTransform = json[k_UseRecommendedTransform].get<bool>();

  const auto& jsonDataPaths = json[k_HDF5DataPaths];
  if(jsonDataPaths.is_null())
  {
    value.selectedArrayNames = {};
  }
  else
  {
    if(!jsonDataPaths.is_array())
    {
      return MakeErrorResult<std::any>(-6054, fmt::format("{}JSON value for key '{} / {}' is not an array", prefix, name()));
    }
    auto dataPathStrings = jsonDataPaths.get<std::vector<std::string>>();
    std::vector<std::string> dataPaths;
    std::vector<Error> errors;
    for(const auto& dataPath : dataPathStrings)
    {
      if(dataPath.empty())
      {
        errors.push_back(Error{FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as HDF5 DataPath", prefix.view(), dataPath)});
        continue;
      }
      dataPaths.push_back(dataPath);
    }

    if(!errors.empty())
    {
      return {{nonstd::make_unexpected(std::move(errors))}};
    }

    value.selectedArrayNames = std::move(dataPaths);
  }

  return {{std::move(value)}};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointer ReadH5EbsdFileParameter::clone() const
{
  return std::make_unique<ReadH5EbsdFileParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

//-----------------------------------------------------------------------------
std::any ReadH5EbsdFileParameter::defaultValue() const
{
  return m_DefaultValue;
}

//-----------------------------------------------------------------------------
Result<> ReadH5EbsdFileParameter::validate(const std::any& valueRef) const
{
  std::vector<Error> errors;

  const auto& value = GetAnyRef<ValueType>(valueRef);
  if(value.inputFilePath.empty())
  {
    errors.push_back({-2000, fmt::format("The input file path for the H5EbsdFile was empty.", value.startSlice, value.endSlice)});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  if(!fs::exists(value.inputFilePath))
  {
    errors.push_back({-2001, fmt::format("Input file does not exist: '{}'", value.inputFilePath)});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  if(value.startSlice > value.endSlice)
  {
    errors.push_back({-2002, fmt::format("StartSlice '{}' must be less than or equal EndSlice '{}'", value.startSlice, value.endSlice)});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  H5EbsdVolumeInfo::Pointer reader = H5EbsdVolumeInfo::New();
  reader->setFileName(value.inputFilePath);
  int32_t err = reader->readVolumeInfo();
  if(err < 0)
  {
    errors.push_back({-2003, fmt::format("H5Ebsd file '{}' could not be opened. Reported error code from the H5EbsdVolumeInfo class is '{}'", value.inputFilePath, err)});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  std::string manufacturerString = reader->getManufacturer();
  if(manufacturerString != EbsdLib::Ang::Manufacturer && manufacturerString != EbsdLib::Ctf::Manufacturer)
  {
    errors.push_back({-2004, fmt::format("Original Data source could not be determined. It should be TSL, HKL or HEDM")});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {};
}

} // namespace complex
