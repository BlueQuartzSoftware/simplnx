#include "OEMEbsdScanSelectionParameter.h"

#include "complex/Common/Any.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

#include "EbsdLib/IO/BrukerNano/EspritConstants.h"
#include "EbsdLib/IO/BrukerNano/H5EspritReader.h"
#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/TSL/H5OIMReader.h"

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
                                                             const AllowedManufacturers& allowedManufacturers, const EbsdReaderType& readerType, const ExtensionsType& extensionsType)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_AllowedManufacturers(allowedManufacturers)
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

  const auto orderCheck = json[k_StackingOrder].get<int32>();
  if(orderCheck != EbsdLib::RefFrameZDir::LowtoHigh && orderCheck != EbsdLib::RefFrameZDir::HightoLow)
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Enumeration, fmt::format("{}JSON value for key '{}' was not a valid ordering Value. [{}|{}] allowed.", prefix.view(),
                                                                                                           k_StackingOrder.view(), EbsdLib::RefFrameZDir::LowtoHigh, EbsdLib::RefFrameZDir::HightoLow));
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
  return std::make_unique<OEMEbsdScanSelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowedManufacturers, m_ReaderType, m_AvailableExtensions);
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

  if(value.stackingOrder != EbsdLib::RefFrameZDir::LowtoHigh && value.stackingOrder != EbsdLib::RefFrameZDir::HightoLow)
  {
    errors.push_back({-20033, fmt::format("{} is not a valid value for the stacking order. Please choose either {} or {}", value.stackingOrder, EbsdLib::RefFrameZDir::LowtoHigh,
                                          EbsdLib::RefFrameZDir::HightoLow)});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  std::list<std::string> scanNames;
  int32 err = 0;
  if(m_ReaderType == EbsdReaderType::Oim)
  {
    const H5OIMReader::Pointer reader = H5OIMReader::New();
    reader->setFileName(value.inputFilePath.string());
    err = reader->readScanNames(scanNames);
  }
  else
  {
    const H5EspritReader::Pointer reader = H5EspritReader::New();
    reader->setFileName(value.inputFilePath.string());
    err = reader->readScanNames(scanNames);
  }

  if(err < 0)
  {
    errors.push_back({-20034, fmt::format("H5 file '{}' could not be opened. Reported error code from the H5OIMReader class is '{}'", value.inputFilePath.string(), err)});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  const ManufacturerType manufacturer = ReadManufacturer(value.inputFilePath.string());
  if(m_AllowedManufacturers.find(manufacturer) == m_AllowedManufacturers.end())
  {
    errors.push_back({-20035, fmt::format("Original data source type {} is not a valid manufacturer", fmt::underlying(manufacturer))});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  if(value.scanNames.empty())
  {
    errors.push_back({-20036, fmt::format("At least one scan must be chosen.  Please select a scan from the list.")});
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {};
}

//-----------------------------------------------------------------------------
OEMEbsdScanSelectionParameter::ManufacturerType OEMEbsdScanSelectionParameter::ReadManufacturer(const std::string& inputFile)
{
  EbsdLib::OEM manuf = EbsdLib::OEM::Unknown;

  const hid_t fid = H5Utilities::openFile(inputFile, true);
  if(fid < 0)
  {
    return manuf;
  }
  H5ScopedFileSentinel sentinel(fid, false);
  std::string dsetName;
  std::list<std::string> names;
  herr_t err = H5Utilities::getGroupObjects(fid, H5Utilities::CustomHDFDataTypes::Any, names);
  auto findIter = std::find(names.begin(), names.end(), EbsdLib::H5OIM::Manufacturer);
  if(findIter != names.end())
  {
    dsetName = EbsdLib::H5OIM::Manufacturer;
  }

  findIter = std::find(names.begin(), names.end(), EbsdLib::H5Esprit::Manufacturer);
  if(findIter != names.end())
  {
    dsetName = EbsdLib::H5Esprit::Manufacturer;
  }

  std::string manufacturer("Unknown");
  err = H5Lite::readStringDataset(fid, dsetName, manufacturer);
  if(err < 0)
  {
    return manuf;
  }
  if(manufacturer == EbsdLib::H5OIM::EDAX)
  {
    manuf = EbsdLib::OEM::EDAX;
  }
  if(manufacturer == EbsdLib::H5Esprit::BrukerNano)
  {
    manuf = EbsdLib::OEM::Bruker;
  }
  if(manufacturer == "DREAM.3D")
  {
    manuf = EbsdLib::OEM::DREAM3D;
  }
  return manuf;
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
