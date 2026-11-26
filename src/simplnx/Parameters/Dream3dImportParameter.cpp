#include "Dream3dImportParameter.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/DataObject.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <unordered_set>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr StringLiteral k_FilePathKey = "file_path";
constexpr StringLiteral k_DataPathsKey = "data_paths";
constexpr StringLiteral k_PathImportPolicyKey = "path_import_policy";
constexpr StringLiteral k_LegacyExcludedContainersKey = "legacy_excluded_containers";

bool IsObjectName(const nlohmann::json& value)
{
  return value.is_string() && DataObject::IsValidName(value.get_ref<const std::string&>());
}

Result<> ValidateLegacySelection(const Dream3dImportParameter::ImportData& selection)
{
  std::unordered_set<std::string> selected;
  selected.reserve(selection.DataPaths.size());
  for(const auto& path : selection.DataPaths)
  {
    if(path.empty() || path.getLength() > 3)
    {
      return MakeErrorResult(-10, fmt::format("The legacy selection path '{}' must name a container, matrix or array.", path.toString()));
    }
    if(!selected.insert(path.toString()).second)
    {
      return MakeErrorResult(-11, fmt::format("The legacy selection repeats the path '{}'.", path.toString()));
    }
  }
  for(const auto& path : selection.DataPaths)
  {
    if(path.getLength() > 1 && !selected.contains(path.getParent().toString()))
    {
      return MakeErrorResult(-12, fmt::format("The legacy selection for '{}' must also select its parent '{}'.", path.toString(), path.getParent().toString()));
    }
  }
  std::unordered_set<std::string> excluded;
  for(const auto& name : selection.LegacyExcludedContainers)
  {
    if(!DataObject::IsValidName(name))
    {
      return MakeErrorResult(-13, fmt::format("The excluded legacy container name '{}' must be nonempty and contain no '/'.", name));
    }
    if(!excluded.insert(name).second)
    {
      return MakeErrorResult(-14, fmt::format("The legacy selection repeats the excluded container '{}'.", name));
    }
    if(selected.contains(name))
    {
      return MakeErrorResult(-15, fmt::format("The legacy selection both selects and excludes the container '{}'.", name));
    }
  }
  return {};
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
void Dream3dImportParameter::AddImageGeometryImportHint(std::vector<Error>& errors)
{
  for(auto& error : errors)
  {
    if(error.code == ImageGeom::k_InvalidOrigin || error.code == ImageGeom::k_InvalidSpacing)
    {
      error.message += fmt::format(" The file has invalid image geometry metadata. You can import it with the {} filter with '{}' enabled.", k_ReadFilterHumanName, k_UseDefaultGeometryValuesLabel);
    }
  }
}

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

//------------------------------------------------------------------------------
IParameter::VersionType Dream3dImportParameter::getVersion() const
{
  return 3;
}

//-----------------------------------------------------------------------------
std::any Dream3dImportParameter::construct(const Arguments& args, const ExecutionContext& executionContext) const
{
  auto value = args.value<ValueType>(name());
  std::filesystem::path absolutePath = executionContext.getAbsolutePath(value.FilePath);
  value.FilePath = absolutePath;
  return value;
}

//-----------------------------------------------------------------------------
nlohmann::json Dream3dImportParameter::toJsonImpl(const std::any& value) const
{
  const auto& importData = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  // File path
  json[k_FilePathKey] = importData.FilePath.string();

  // DataPaths
  nlohmann::json dataPathsJson = nlohmann::json::array();
  for(const auto& dataPath : importData.DataPaths)
  {
    dataPathsJson.push_back(dataPath.toString());
  }
  json[k_DataPathsKey] = std::move(dataPathsJson);

  json[k_PathImportPolicyKey] = importData.ImportPolicy;
  if(importData.ImportPolicy == PathImportPolicy::LegacySelection)
  {
    json[k_LegacyExcludedContainersKey] = importData.LegacyExcludedContainers;
  }

  return json;
}

//-----------------------------------------------------------------------------
Result<std::any> Dream3dImportParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'Dream3dImportParameter' JSON Error: ";
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not an object", prefix, name()));
  }

  if(!json.contains(k_FilePathKey.view()))
  {
    return MakeErrorResult<std::any>(-3, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), k_FilePathKey.view()));
  }

  if(!json.contains(k_DataPathsKey.view()))
  {
    return MakeErrorResult<std::any>(-4, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), k_DataPathsKey.view()));
  }

  const auto& jsonFilePath = json[k_FilePathKey];
  if(!jsonFilePath.is_string())
  {
    return MakeErrorResult<std::any>(-5, fmt::format("{}JSON value for key '{} / {}' is not an string", prefix, name(), k_FilePathKey));
  }
  auto filePath = fs::path(jsonFilePath.get<std::string>());

  const auto& jsonDataPaths = json[k_DataPathsKey];
  std::vector<DataPath> dataPaths;
  if(!jsonDataPaths.is_null())
  {
    if(!jsonDataPaths.is_array())
    {
      return MakeErrorResult<std::any>(-6, fmt::format("{}JSON value for key '{} / {}' is not an array", prefix, name(), k_DataPathsKey));
    }
    std::vector<Error> errors;
    for(const auto& entry : jsonDataPaths)
    {
      if(!entry.is_string())
      {
        return MakeErrorResult<std::any>(-7, fmt::format("Each entry in '{}' must be a path string. Found '{}'.", k_DataPathsKey, entry.dump()));
      }
      const auto& dataPathString = entry.get_ref<const std::string&>();
      std::optional<DataPath> dataPath = DataPath::FromString(dataPathString);
      if(!dataPath.has_value())
      {
        errors.push_back(Error{FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath", prefix.view(), dataPathString)});
        continue;
      }
      dataPaths.push_back(std::move(*dataPath));
    }

    if(!errors.empty())
    {
      return {{nonstd::make_unexpected(std::move(errors))}};
    }
  }

  PathImportPolicy pathImportPolicy = PathImportPolicy::All;
  if(version >= 2)
  {
    const auto policy = json.find(k_PathImportPolicyKey.view());
    if(policy == json.end() || !policy->is_number_integer())
    {
      return MakeErrorResult<std::any>(-8, fmt::format("Provide an integer import policy in '{}'.", k_PathImportPolicyKey));
    }
    const auto maxPolicy = version == 2 ? PathImportPolicy::ExcludeList : PathImportPolicy::LegacySelection;
    if(*policy < 0 || *policy > static_cast<uint8>(maxPolicy))
    {
      return MakeErrorResult<std::any>(-9, fmt::format("The import policy '{}' is unsupported for parameter version {}.", policy->dump(), version));
    }
    pathImportPolicy = static_cast<PathImportPolicy>(policy->get<uint8>());
  }
  else if(!dataPaths.empty())
  {
    pathImportPolicy = PathImportPolicy::IncludeList;
  }

  ImportData importData(std::move(filePath), pathImportPolicy, std::move(dataPaths));
  if(pathImportPolicy == PathImportPolicy::LegacySelection)
  {
    if(jsonDataPaths.is_null())
    {
      return MakeErrorResult<std::any>(-18, fmt::format("Provide an array of selected SIMPL paths in '{}'. Use an empty array for an empty selection.", k_DataPathsKey));
    }
    for(usize i = 0; i < importData.DataPaths.size(); i++)
    {
      if(importData.DataPaths[i].toString() != jsonDataPaths[i].get_ref<const std::string&>())
      {
        return MakeErrorResult<std::any>(-19, fmt::format("The selected SIMPL path '{}' contains an empty name between or outside the '/' separators.", jsonDataPaths[i].dump()));
      }
    }
    const auto excluded = json.find(k_LegacyExcludedContainersKey.view());
    if(excluded == json.end() || !excluded->is_array())
    {
      return MakeErrorResult<std::any>(-16, fmt::format("Provide an array of unchecked container names in '{}'.", k_LegacyExcludedContainersKey));
    }
    for(const auto& entry : *excluded)
    {
      if(!entry.is_string())
      {
        return MakeErrorResult<std::any>(-17, fmt::format("Each excluded container must be a name string. Found '{}'.", entry.dump()));
      }
      importData.LegacyExcludedContainers.push_back(entry.get<std::string>());
    }
    auto result = ValidateLegacySelection(importData);
    if(result.invalid())
    {
      return ConvertInvalidResult<std::any>(std::move(result));
    }
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
  if(importData.ImportPolicy == PathImportPolicy::LegacySelection)
  {
    auto result = ValidateLegacySelection(importData);
    if(result.invalid())
    {
      return result;
    }
  }
  return validatePath(importData);
}

//-----------------------------------------------------------------------------
Result<> Dream3dImportParameter::validatePath(const ValueType& importData) const
{
  try
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

    // Opening the file to confirm it is readable HDF5 is left to the consuming
    // filter's preflight (e.g. ReadDREAM3DFilter via Dream3dPreflightCache),
    // which already performs that open to read the file's metadata. Doing it
    // again here would be a second HDF5 open per parameter validation, and
    // validation runs on every pipeline edit, so on high-latency storage that
    // is a repeated, redundant round-trip for a check the preflight already
    // makes authoritatively.
  } catch(const fs::filesystem_error& exception)
  {
    return MakeErrorResult(-5, fmt::format("Filesystem exception: {}", exception.what()));
  }
  return {};
}

namespace SIMPLConversion
{
namespace
{
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_InputFileDataContainerArrayProxyKey = "InputFileDataContainerArrayProxy";
constexpr std::array<std::string_view, 3> k_ProxyChildrenKeys = {"Data Containers", "Attribute Matricies", "Data Arrays"};

Result<> ReadProxyChildren(const nlohmann::json& proxy, const DataPath& parent, bool parentSelected, Dream3dImportParameter::ImportData& selection)
{
  const auto key = k_ProxyChildrenKeys[parent.getLength()];
  const auto children = proxy.find(key);
  if(children == proxy.end() || !children->is_array())
  {
    return MakeErrorResult(-3, fmt::format("The SIMPL reader proxy at '{}' requires an array named '{}'.", parent.toString(), key));
  }
  std::unordered_set<std::string> names;
  for(const auto& child : *children)
  {
    const auto name = child.find("Name");
    if(name == child.end() || !IsObjectName(*name))
    {
      return MakeErrorResult(-4, fmt::format("Each entry in '{}' at '{}' needs a nonempty Name without '/'. Found '{}'.", key, parent.toString(), child.dump()));
    }
    const auto& objectName = name->get_ref<const std::string&>();
    const auto path = parent.createChildPath(objectName);
    if(!names.insert(objectName).second)
    {
      return MakeErrorResult(-5, fmt::format("The SIMPL reader proxy contains more than one entry for '{}'.", path.toString()));
    }

    // SIMPL wrote numeric uint8 flags. Earlier NX conversions also accepted name-only proxies as selected.
    bool selected = parentSelected;
    const auto flag = child.find("Flag");
    if(flag != child.end())
    {
      if(!flag->is_number())
      {
        return MakeErrorResult(-6, fmt::format("The SIMPL selection Flag for '{}' must be numeric. Found '{}'.", path.toString(), flag->dump()));
      }
      const auto value = flag->get<float64>();
      if(!(value >= 0 && value <= 255 && value == std::trunc(value)))
      {
        return MakeErrorResult(-7, fmt::format("The SIMPL selection Flag for '{}' must be an integer from 0 to 255. Found '{}'.", path.toString(), flag->dump()));
      }
      selected = parentSelected && value != 0;
    }
    if(selected)
    {
      selection.DataPaths.push_back(path);
    }
    else if(parent.empty())
    {
      selection.LegacyExcludedContainers.push_back(objectName);
    }
    if(path.getLength() < k_ProxyChildrenKeys.size())
    {
      auto result = ReadProxyChildren(child, path, selected, selection);
      if(result.invalid())
      {
        return result;
      }
    }
  }
  return {};
}
} // namespace

Result<DataContainerReaderFilterParameterConverter::ValueType> DataContainerReaderFilterParameterConverter::convert(const nlohmann::json& json)
{
  const auto input = json.find(k_InputFileKey.view());
  if(input == json.end() || !input->is_string())
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("Provide the SIMPL reader's file path as a string in '{}'.", k_InputFileKey));
  }
  const auto proxy = json.find(k_InputFileDataContainerArrayProxyKey.view());
  if(proxy == json.end() || !proxy->is_object())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("Provide the SIMPL reader's selection as an object in '{}'.", k_InputFileDataContainerArrayProxyKey));
  }

  ValueType selection(fs::path(input->get<std::string>()));
  auto result = ReadProxyChildren(*proxy, {}, true, selection);
  if(result.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(result));
  }
  // SIMPL rebuilt an empty proxy from the file, selecting all available containers.
  if(!selection.DataPaths.empty() || !selection.LegacyExcludedContainers.empty())
  {
    selection.ImportPolicy = ParameterType::PathImportPolicy::LegacySelection;
  }
  return {std::move(selection)};
}
} // namespace SIMPLConversion
} // namespace nx::core
