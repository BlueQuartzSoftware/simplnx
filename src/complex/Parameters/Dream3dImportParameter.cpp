#include "Dream3dImportParameter.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

using namespace complex;

namespace
{
constexpr StringLiteral k_FilePathKey = "filepath";
constexpr StringLiteral k_DataPathsKey = "datapaths";
} // namespace

namespace complex
{
//-----------------------------------------------------------------------------
Dream3dImportParameter::Dream3dImportParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

//-----------------------------------------------------------------------------
Uuid Dream3dImportParameter::uuid() const
{
  return ParameterTraits<Dream3dImportParameter>::uuid;
}

//-----------------------------------------------------------------------------
IParameter::AcceptedTypes Dream3dImportParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//-----------------------------------------------------------------------------
nlohmann::json Dream3dImportParameter::toJson(const std::any& value) const
{
  ValueType importData = std::any_cast<ValueType>(value);
  nlohmann::json json;
  // File path
  json[k_FilePathKey.c_str()] = importData.FilePath.string();

  // DataPaths
  nlohmann::json dataPathsJson = nlohmann::json::array();
  for(const auto& dataPath : importData.DataPaths)
  {
    dataPathsJson.push_back(dataPath.toString());
  }
  json[k_DataPathsKey.c_str()] = dataPathsJson;

  return json;
}

//-----------------------------------------------------------------------------
Result<std::any> Dream3dImportParameter::fromJson(const nlohmann::json& json) const
{
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonObject = json.at(key);
  if(!jsonObject.is_object())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not an object", key)}})};
  }

  if(!jsonObject.contains(k_FilePathKey.str()))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{} \\ {}\"", key, k_FilePathKey)}})};
  }
  if(!jsonObject.contains(k_DataPathsKey.str()))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{} \\ {}\"", key, k_DataPathsKey)}})};
  }

  ImportData importData;
  auto jsonFilePath = jsonObject.at(k_FilePathKey.str());
  if(!jsonFilePath.is_string())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{} \\ {}\" is not an string", key, k_FilePathKey)}})};
  }
  importData.FilePath = jsonFilePath.get<std::string>();

  auto jsonDataPaths = jsonObject.at(k_DataPathsKey.c_str());
  if(!jsonDataPaths.is_array())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{} \\ {}\" is not an array", key)}})};
  }
  auto dataPathStrings = jsonDataPaths.get<std::vector<std::string>>();
  std::vector<DataPath> dataPaths;
  for(const auto& dataPathString : dataPathStrings)
  {
    auto dataPath = DataPath::FromString(dataPathString);
    if(dataPath.has_value())
    {
      dataPaths.push_back(dataPath.value());
    }
  }
  importData.DataPaths = dataPaths;
  return {importData};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointer Dream3dImportParameter::clone() const
{
  return std::make_unique<Dream3dImportParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

//-----------------------------------------------------------------------------
std::any Dream3dImportParameter::defaultValue() const
{
  return ImportData();
}

//-----------------------------------------------------------------------------
Result<> Dream3dImportParameter::validate(const std::any& value) const
{
  auto importData = std::any_cast<ValueType>(value);
  return validatePath(importData);
}

//-----------------------------------------------------------------------------
Result<> Dream3dImportParameter::validatePath(const ValueType& importData) const
{
  std::filesystem::path path = importData.FilePath;
  if(!fs::exists(path))
  {
    return MakeErrorResult(-2, fmt::format("Path \"{}\" does not exist", path.string()));
  }

  if(!fs::is_regular_file(path))
  {
    return MakeErrorResult(-3, fmt::format("Path \"{}\" is not a file", path.string()));
  }

  H5::FileReader fileReader(path);
  if(!fileReader.isValid())
  {
    return MakeErrorResult(-4, fmt::format("HDF5 file at path \"{}\" could not be read", path.string()));
  }
  return {};
}
} // namespace complex
