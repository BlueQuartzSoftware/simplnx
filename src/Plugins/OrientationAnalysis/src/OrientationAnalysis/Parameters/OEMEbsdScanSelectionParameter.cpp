#include "OEMEbsdScanSelectionParameter.h"

#include "complex/Common/Any.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <nlohmann/json.hpp>

namespace complex
{
namespace
{
constexpr StringLiteral k_InputFilePath = "input_file_path";
constexpr StringLiteral k_StackingOrder = "stacking_order";
constexpr StringLiteral k_ScanNames = "scan_names";
} // namespace

//-----------------------------------------------------------------------------
OEMEbsdScanSelectionParameter::OEMEbsdScanSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                             const EbsdReaderType& readerType, const ExtensionsType& extensionsType)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_AvailableExtensions(extensionsType)
, m_ReaderType(readerType)
{
}

//-----------------------------------------------------------------------------
Uuid OEMEbsdScanSelectionParameter::uuid() const
{
  return ParameterTraits<OEMEbsdScanSelectionParameter>::uuid;
}

//-----------------------------------------------------------------------------
IParameter::AcceptedTypes OEMEbsdScanSelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//-----------------------------------------------------------------------------
nlohmann::json OEMEbsdScanSelectionParameter::toJson(const std::any& value) const
{
  const auto& data = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[k_InputFilePath] = data.inputFilePath;
  json[k_StackingOrder] = data.stackingOrder;

  // scan names
  if(!data.scanNames.empty())
  {
    nlohmann::json scanNamesJson = nlohmann::json::array();
    for(const auto& name : data.scanNames)
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
Result<std::any> OEMEbsdScanSelectionParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'OEMEbsdScanSelectionParameter' Error: ";

  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Object, fmt::format("{}The JSON data entry for key '{}' is not an object.", prefix.view(), name()));
  }
  // Validate numeric json entries
  std::vector<const char*> keys = {k_StackingOrder.c_str()};
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

  constexpr int32 k_LowtoHigh = 0;
  constexpr int32 k_HightoLow = 1;
  const auto orderCheck = json[k_StackingOrder].get<int32>();
  if(orderCheck != k_LowtoHigh && orderCheck != k_HightoLow)
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Enumeration,
                                     fmt::format("{}JSON value for key '{}' was not a valid ordering Value. [{}|{}] allowed.", prefix.view(), k_StackingOrder.view(), k_LowtoHigh, k_HightoLow));
  }

  ValueType value;

  value.inputFilePath = json[k_InputFilePath].get<std::string>();
  value.stackingOrder = json[k_StackingOrder].get<int32>();

  const auto& jsonScanNames = json[k_ScanNames];
  if(jsonScanNames.is_null())
  {
    value.scanNames = {};
  }
  else
  {
    if(!jsonScanNames.is_array())
    {
      return MakeErrorResult<std::any>(-6054, fmt::format("{}JSON value for key '{} / {}' is not an array", prefix, name()));
    }
    const auto scanNameStrings = jsonScanNames.get<std::vector<std::string>>();
    std::list<std::string> scanNames;
    std::vector<Error> errors;
    for(const auto& name : scanNameStrings)
    {
      if(name.empty())
      {
        errors.push_back(Error{FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}JSON value for key '{}' is not a string", prefix.view(), name)});
        continue;
      }
      scanNames.push_back(name);
    }

    if(!errors.empty())
    {
      return {{nonstd::make_unexpected(std::move(errors))}};
    }

    value.scanNames = std::move(scanNames);
  }

  return {{std::move(value)}};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointer OEMEbsdScanSelectionParameter::clone() const
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
  if(value.inputFilePath.empty())
  {
    errors.push_back({-20030, fmt::format("The input file path for the H5 file was empty.")});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  if(!fs::exists(value.inputFilePath))
  {
    errors.push_back({-20031, fmt::format("Input file does not exist: '{}'", value.inputFilePath.string())});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  const std::string lowerExtension = complex::StringUtilities::toLower(value.inputFilePath.extension().string());
  if(value.inputFilePath.has_extension() && !m_AvailableExtensions.empty() && m_AvailableExtensions.find(lowerExtension) == m_AvailableExtensions.end())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-20032, fmt::format("File extension '{}' is not a valid file extension.", value.inputFilePath.extension().string())}})};
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

} // namespace complex
