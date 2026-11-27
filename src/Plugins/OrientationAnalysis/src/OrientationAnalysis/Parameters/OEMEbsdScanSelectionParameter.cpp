#include "OEMEbsdScanSelectionParameter.h"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <nlohmann/json.hpp>
#include <utility>

namespace nx::core
{
namespace
{
constexpr StringLiteralType k_InputFilePath = "input_file_path";
constexpr StringLiteralType k_StackingOrder = "stacking_order";
constexpr StringLiteralType k_ScanNames = "scan_names";
} // namespace

//-----------------------------------------------------------------------------
OEMEbsdScanSelectionParameter::OEMEbsdScanSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue,
                                                             const EbsdReaderType& readerType, ExtensionsType extensionsType)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(std::move(defaultValue))
, m_AvailableExtensions(std::move(extensionsType))
, m_ReaderType(readerType)
{
}

//-----------------------------------------------------------------------------
Uuid OEMEbsdScanSelectionParameter::uuid() const
{
  return ParameterTraits<OEMEbsdScanSelectionParameter>::uuid;
}

//-----------------------------------------------------------------------------
IParameter::AcceptedTypesType OEMEbsdScanSelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType OEMEbsdScanSelectionParameter::getVersion() const
{
  return 1;
}

//-----------------------------------------------------------------------------
std::any OEMEbsdScanSelectionParameter::construct(const Arguments& args, const ExecutionContext& executionContext) const
{
  auto value = args.value<ValueType>(name());
  std::filesystem::path absolutePath = executionContext.getAbsolutePath(value.input_file_path);
  value.input_file_path = absolutePath.string();
  return value;
}

//-----------------------------------------------------------------------------
nlohmann::json OEMEbsdScanSelectionParameter::toJsonImpl(const std::any& value) const
{
  const auto& data = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[k_InputFilePath] = data.input_file_path;
  json[k_StackingOrder] = data.stacking_order;

  // scan names
  if(!data.scan_names.empty())
  {
    nlohmann::json scanNamesJson = nlohmann::json::array();
    for(const auto& name : data.scan_names)
    {
      scanNamesJson.push_back(name);
    }
    json[k_ScanNames] = std::move(scanNamesJson);
  }
  else
  {
    json[k_ScanNames] = nullptr;
  }

  return json;
}

//-----------------------------------------------------------------------------
Result<std::any> OEMEbsdScanSelectionParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'OEMEbsdScanSelectionParameter' Error: ";

  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotObject, fmt::format("{}The JSON data entry for key '{}' is not an object.", k_Prefix.view(), name()));
  }
  // Validate numeric json entries
  std::vector<const char*> keys = {k_StackingOrder.c_str()};
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

  const auto orderCheck = json[k_StackingOrder].get<int32>();
  if(orderCheck != ref_frame_z_dir::k_LowtoHigh && orderCheck != ref_frame_z_dir::k_HightoLow)
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotEnumeration, fmt::format("{}JSON value for key '{}' was not a valid ordering Value. [{}|{}] allowed.", k_Prefix.view(),
                                                                                                         k_StackingOrder.view(), ref_frame_z_dir::k_LowtoHigh, ref_frame_z_dir::k_HightoLow));
  }

  ValueType value;

  value.input_file_path = json[k_InputFilePath].get<std::string>();
  value.stacking_order = json[k_StackingOrder].get<int32>();

  const auto& jsonScanNames = json[k_ScanNames];
  if(jsonScanNames.is_null())
  {
    value.scan_names = {};
  }
  else
  {
    if(!jsonScanNames.is_array())
    {
      return MakeErrorResult<std::any>(-6054, fmt::format("{}JSON value for key '{} / {}' is not an array", k_Prefix, name(), k_ScanNames));
    }
    const auto scanNameStrings = jsonScanNames.get<std::vector<std::string>>();
    std::list<std::string> scanNames;
    std::vector<Error> errors;
    for(const auto& name : scanNameStrings)
    {
      if(name.empty())
      {
        errors.push_back(Error{.code = filter_parameter::constants::k_JsonValueNotValueType, .message = fmt::format("{}JSON value for key '{}' is not a string", k_Prefix.view(), name)});
        continue;
      }
      scanNames.push_back(name);
    }

    if(!errors.empty())
    {
      return {{nonstd::make_unexpected(std::move(errors))}};
    }

    value.scan_names = std::move(scanNames);
  }

  return {{std::move(value)}};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointerType OEMEbsdScanSelectionParameter::clone() const
{
  return std::make_unique<OEMEbsdScanSelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_ReaderType, m_AvailableExtensions);
}

//-----------------------------------------------------------------------------
std::any OEMEbsdScanSelectionParameter::defaultValue() const
{
  return m_DefaultValue;
}

//-----------------------------------------------------------------------------
Result<> OEMEbsdScanSelectionParameter::validate(const std::any& valueRef) const
{
  std::vector<Error> errors;

  const auto& value = GetAnyRef<ValueType>(valueRef);
  if(value.input_file_path.empty())
  {
    errors.push_back({-20030, fmt::format("The input file path for the H5 file was empty.")});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  if(!fs::exists(value.input_file_path))
  {
    errors.push_back({-20031, fmt::format("Input file does not exist: '{}'", value.input_file_path.string())});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  const std::string lowerExtension = nx::core::string_utilities::ToLower(value.input_file_path.extension().string());
  if(value.input_file_path.has_extension() && !m_AvailableExtensions.empty() && m_AvailableExtensions.find(lowerExtension) == m_AvailableExtensions.end())
  {
    return MakeErrorResult(-20032, fmt::format("File extension '{}' is not a valid file extension.", value.input_file_path.extension().string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
OEMEbsdScanSelectionParameter::ExtensionsType OEMEbsdScanSelectionParameter::getAvailableExtensions() const
{
  return m_AvailableExtensions;
}

//-----------------------------------------------------------------------------
OEMEbsdScanSelectionParameter::EbsdReaderType OEMEbsdScanSelectionParameter::getReaderType() const
{
  return m_ReaderType;
}

} // namespace nx::core
