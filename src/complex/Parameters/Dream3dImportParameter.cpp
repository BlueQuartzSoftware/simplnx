#include "Dream3dImportParameter.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

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
  const auto& importData = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  // File path
  json[k_FilePathKey] = importData.FilePath.string();

  // DataPaths
  if(importData.DataPaths.has_value())
  {
    nlohmann::json dataPathsJson = nlohmann::json::array();
    for(const auto& dataPath : importData.DataPaths.value())
    {
      dataPathsJson.push_back(dataPath.toString());
    }
    json[k_DataPathsKey] = std::move(dataPathsJson);
  }
  else
  {
    json[k_DataPathsKey] = nullptr;
  }

  return json;
}

//-----------------------------------------------------------------------------
Result<std::any> Dream3dImportParameter::fromJson(const nlohmann::json& json) const
{
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("JSON value for key '{}' is not an object", name()));
  }

  if(!json.contains(k_FilePathKey.view()))
  {
    return MakeErrorResult<std::any>(-3, fmt::format("JSON does not contain key \"{} / {}\"", name(), k_FilePathKey.view()));
  }

  if(!json.contains(k_DataPathsKey.view()))
  {
    return MakeErrorResult<std::any>(-4, fmt::format("JSON does not contain key \"{} / {}\"", name(), k_DataPathsKey.view()));
  }

  ImportData importData;
  const auto& jsonFilePath = json[k_FilePathKey];
  if(!jsonFilePath.is_string())
  {
    return MakeErrorResult<std::any>(-5, fmt::format("JSON value for key \"{} / {}\" is not an string", name(), k_FilePathKey));
  }
  importData.FilePath = jsonFilePath.get<std::string>();

  const auto& jsonDataPaths = json[k_DataPathsKey];
  if(jsonDataPaths.is_null())
  {
    importData.DataPaths = std::nullopt;
  }
  else
  {
    if(!jsonDataPaths.is_array())
    {
      return MakeErrorResult<std::any>(-6, fmt::format("JSON value for key \"{} / {}\" is not an array", name()));
    }
    auto dataPathStrings = jsonDataPaths.get<std::vector<std::string>>();
    std::vector<DataPath> dataPaths;
    std::vector<Error> errors;
    for(const auto& dataPathString : dataPathStrings)
    {
      std::optional<DataPath> dataPath = DataPath::FromString(dataPathString);
      if(!dataPath.has_value())
      {
        errors.push_back(Error{-7, fmt::format("Failed to parse '{}' as DataPath", dataPathString)});
        continue;
      }
      dataPaths.push_back(std::move(*dataPath));
    }

    if(!errors.empty())
    {
      return {{nonstd::make_unexpected(std::move(errors))}};
    }

    importData.DataPaths = std::move(dataPaths);
  }

  return {{std::move(importData)}};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointer Dream3dImportParameter::clone() const
{
  return std::make_unique<Dream3dImportParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

//-----------------------------------------------------------------------------
std::any Dream3dImportParameter::defaultValue() const
{
  return m_DefaultValue;
}

//-----------------------------------------------------------------------------
Result<> Dream3dImportParameter::validate(const std::any& value) const
{
  const auto& importData = GetAnyRef<ValueType>(value);
  return validatePath(importData);
}

//-----------------------------------------------------------------------------
Result<> Dream3dImportParameter::validatePath(const ValueType& importData) const
{
  std::filesystem::path path = importData.FilePath;
  if(!fs::exists(path))
  {
    return MakeErrorResult(-2, fmt::format("Path '{}' does not exist", path.string()));
  }

  if(!fs::is_regular_file(path))
  {
    return MakeErrorResult(-3, fmt::format("Path '{}' is not a file", path.string()));
  }

  H5::FileReader fileReader(path);
  if(!fileReader.isValid())
  {
    return MakeErrorResult(-4, fmt::format("HDF5 file at path '{}' could not be read", path.string()));
  }
  return {};
}
} // namespace complex
