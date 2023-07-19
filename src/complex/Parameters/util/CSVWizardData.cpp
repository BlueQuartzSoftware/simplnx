/* ============================================================================
 * Copyright (c) 2022-2022 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THEs
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "CSVWizardData.hpp"

#include <fmt/format.h>

using namespace complex;

namespace
{
const std::string k_DataHeadersKey = "Data Headers";
const std::string k_DataTypesKey = "Data Types";
const std::string k_DelimitersKey = "Delimiters";
const std::string k_TupleDimensionsKey = "Tuple Dimensions";
const std::string k_InputFilePathKey = "Input File Path";
const std::string k_BeginIndexKey = "Begin Index";
const std::string k_NumberOfLinesKey = "Number of Lines";
const std::string k_HeaderLineKey = "Header Line";
const std::string k_HeaderModeKey = "Header Mode";
const std::string k_TabAsDelimiterKey = "Tab As Delimiter";
const std::string k_SemicolonAsDelimiterKey = "Semicolon As Delimiter";
const std::string k_CommaAsDelimiterKey = "Comma As Delimiter";
const std::string k_SpaceAsDelimiterKey = "Space As Delimiter";
const std::string k_ConsecutiveDelimitersKey = "Consecutive Delimiters";
} // namespace

// -----------------------------------------------------------------------------
nlohmann::json CSVWizardData::writeJson() const
{
  nlohmann::json json;

  nlohmann::json dHeaders;
  for(const auto& header : dataHeaders)
  {
    dHeaders.push_back(header);
  }
  json[k_DataHeadersKey] = dHeaders;

  nlohmann::json dTypes;
  for(const auto& dType : dataTypes)
  {
    if(!dType.has_value())
    {
      dTypes.push_back(k_SkipDataTypeString);
    }
    else
    {
      dTypes.push_back(dType.value());
    }
  }
  json[k_DataTypesKey] = dTypes;

  nlohmann::json delimitersObj;
  for(const auto& delimiter : delimiters)
  {
    delimitersObj.push_back(delimiter);
  }
  json[k_DelimitersKey] = delimitersObj;

  json[k_InputFilePathKey] = inputFilePath;
  json[k_BeginIndexKey] = beginIndex;
  json[k_NumberOfLinesKey] = numberOfLines;
  json[k_HeaderLineKey] = headerLine;
  json[k_HeaderModeKey] = headerMode;
  json[k_TabAsDelimiterKey] = tabAsDelimiter;
  json[k_SemicolonAsDelimiterKey] = semicolonAsDelimiter;
  json[k_SpaceAsDelimiterKey] = spaceAsDelimiter;
  json[k_CommaAsDelimiterKey] = commaAsDelimiter;
  json[k_ConsecutiveDelimitersKey] = consecutiveDelimiters;

  return json;
}

// -----------------------------------------------------------------------------
Result<CSVWizardData> CSVWizardData::ReadJson(const nlohmann::json& json)
{
  CSVWizardData data;

  if(!json.contains(k_DataHeadersKey))
  {
    return MakeErrorResult<CSVWizardData>(-100, fmt::format("CSVWizardData: Cannot find the Data Headers key \"{}\" in the CSVWizardData json object.", k_DataHeadersKey));
  }

  nlohmann::json dHeaders = json[k_DataHeadersKey];
  for(usize i = 0; i < dHeaders.size(); i++)
  {
    auto header = dHeaders[i];
    if(!header.is_string())
    {
      return MakeErrorResult<CSVWizardData>(-101, fmt::format("CSVWizardData: Data header at index {} is of type {} and is not a string.", std::to_string(i), header.type_name()));
    }

    data.dataHeaders.push_back(header);
  }

  if(!json.contains(k_DataTypesKey))
  {
    return MakeErrorResult<CSVWizardData>(-102, fmt::format("CSVWizardData: Cannot find the Data Types key \"{}\" in the CSVWizardData json object.", k_DataTypesKey));
  }

  nlohmann::json dTypes = json[k_DataTypesKey];
  for(usize i = 0; i < dTypes.size(); i++)
  {
    auto dType = dTypes[i];
    if(dType.is_string())
    {
      if(dType != k_SkipDataTypeString)
      {
        return MakeErrorResult<CSVWizardData>(-103, fmt::format("CSVWizardData: Data type at index {} is not a valid data type and is not marked as \"skipped\".", std::to_string(i)));
      }
    }
    else if(!dType.is_number_integer())
    {
      return MakeErrorResult<CSVWizardData>(-104, fmt::format("CSVWizardData: Data type at index {} is of type {} and is not an integer.", std::to_string(i), dType.type_name()));
    }

    if(dType.is_string())
    {
      data.dataTypes.push_back({});
    }
    else
    {
      data.dataTypes.push_back(dType);
    }
  }

  if(!json.contains(k_DelimitersKey))
  {
    return MakeErrorResult<CSVWizardData>(-105, fmt::format("CSVWizardData: Cannot find the Delimiters key \"{}\" in the CSVWizardData json object.", k_DelimitersKey));
  }

  nlohmann::json delimiters = json[k_DelimitersKey];
  for(usize i = 0; i < delimiters.size(); i++)
  {
    auto delimiterObj = delimiters[i];
    if(!delimiterObj.is_number_integer())
    {
      return MakeErrorResult<CSVWizardData>(-106, fmt::format("CSVWizardData: Delimiter at index {} is of type {} and is not an integer.", std::to_string(i), fmt::underlying(delimiterObj.type())));
    }

    data.delimiters.push_back(delimiterObj.get<char>());
  }

  if(!json.contains(k_InputFilePathKey))
  {
    return MakeErrorResult<CSVWizardData>(-107, fmt::format("CSVWizardData: Cannot find the 'Input File Path' key \"{}\" in the CSVWizardData json object.", k_InputFilePathKey));
  }
  else if(!json[k_InputFilePathKey].is_string())
  {
    return MakeErrorResult<CSVWizardData>(-108, fmt::format("CSVWizardData: 'Input File Path' value is of type {} and is not a string.", json[k_InputFilePathKey].type_name()));
  }
  data.inputFilePath = json[k_InputFilePathKey];

  if(!json.contains(k_BeginIndexKey))
  {
    return MakeErrorResult<CSVWizardData>(-109, fmt::format("CSVWizardData: Cannot find the 'Begin Index' key \"{}\" in the CSVWizardData json object.", k_BeginIndexKey));
  }
  else if(!json[k_BeginIndexKey].is_number_integer())
  {
    return MakeErrorResult<CSVWizardData>(-110, fmt::format("CSVWizardData: 'Begin Index' value is of type {} and is not an integer.", json[k_BeginIndexKey].type_name()));
  }
  data.beginIndex = json[k_BeginIndexKey];

  if(!json.contains(k_NumberOfLinesKey))
  {
    return MakeErrorResult<CSVWizardData>(-111, fmt::format("CSVWizardData: Cannot find the 'Number of Lines' key \"{}\" in the CSVWizardData json object.", k_NumberOfLinesKey));
  }
  else if(!json[k_NumberOfLinesKey].is_number_integer())
  {
    return MakeErrorResult<CSVWizardData>(-112, fmt::format("CSVWizardData: 'Number of Lines' value is of type {} and is not an integer.", json[k_NumberOfLinesKey].type_name()));
  }
  data.numberOfLines = json[k_NumberOfLinesKey];

  if(!json.contains(k_HeaderLineKey))
  {
    return MakeErrorResult<CSVWizardData>(-113, fmt::format("CSVWizardData: Cannot find the 'Header Line' key \"{}\" in the CSVWizardData json object.", k_HeaderLineKey));
  }
  else if(!json[k_HeaderLineKey].is_number_integer())
  {
    return MakeErrorResult<CSVWizardData>(-114, fmt::format("CSVWizardData: 'Header Line' value is of type {} and is not an integer.", json[k_HeaderLineKey].type_name()));
  }
  data.headerLine = json[k_HeaderLineKey];

  if(!json.contains(k_HeaderModeKey))
  {
    return MakeErrorResult<CSVWizardData>(-115, fmt::format("CSVWizardData: Cannot find the 'Header Mode' key \"{}\" in the CSVWizardData json object.", k_HeaderModeKey));
  }
  else if(!json[k_HeaderModeKey].is_number_integer())
  {
    return MakeErrorResult<CSVWizardData>(-116, fmt::format("CSVWizardData: 'Header Mode' value is of type {} and is not an integer.", json[k_HeaderModeKey].type_name()));
  }
  data.headerMode = json[k_HeaderModeKey];

  if(!json.contains(k_TabAsDelimiterKey))
  {
    return MakeErrorResult<CSVWizardData>(-117, fmt::format("CSVWizardData: Cannot find the 'Tab As Delimiter' key \"{}\" in the CSVWizardData json object.", k_TabAsDelimiterKey));
  }
  else if(!json[k_TabAsDelimiterKey].is_boolean())
  {
    return MakeErrorResult<CSVWizardData>(-118, fmt::format("CSVWizardData: 'Tab As Delimiter' value is of type {} and is not a boolean.", json[k_TabAsDelimiterKey].type_name()));
  }
  data.tabAsDelimiter = json[k_TabAsDelimiterKey];

  if(!json.contains(k_SemicolonAsDelimiterKey))
  {
    return MakeErrorResult<CSVWizardData>(-119, fmt::format("CSVWizardData: Cannot find the 'Semicolon As Delimiter' key \"{}\" in the CSVWizardData json object.", k_SemicolonAsDelimiterKey));
  }
  else if(!json[k_SemicolonAsDelimiterKey].is_boolean())
  {
    return MakeErrorResult<CSVWizardData>(-120, fmt::format("CSVWizardData: 'Semicolon As Delimiter' value is of type {} and is not a boolean.", json[k_SemicolonAsDelimiterKey].type_name()));
  }
  data.semicolonAsDelimiter = json[k_SemicolonAsDelimiterKey];

  if(!json.contains(k_SpaceAsDelimiterKey))
  {
    return MakeErrorResult<CSVWizardData>(-121, fmt::format("CSVWizardData: Cannot find the 'Space As Delimiter' key \"{}\" in the CSVWizardData json object.", k_SpaceAsDelimiterKey));
  }
  else if(!json[k_SpaceAsDelimiterKey].is_boolean())
  {
    return MakeErrorResult<CSVWizardData>(-122, fmt::format("CSVWizardData: 'Space As Delimiter' value is of type {} and is not a boolean.", json[k_SpaceAsDelimiterKey].type_name()));
  }
  data.spaceAsDelimiter = json[k_SpaceAsDelimiterKey];

  if(!json.contains(k_CommaAsDelimiterKey))
  {
    return MakeErrorResult<CSVWizardData>(-123, fmt::format("CSVWizardData: Cannot find the 'Comma As Delimiter' key \"{}\" in the CSVWizardData json object.", k_CommaAsDelimiterKey));
  }
  else if(!json[k_CommaAsDelimiterKey].is_boolean())
  {
    return MakeErrorResult<CSVWizardData>(-124, fmt::format("CSVWizardData: 'Comma As Delimiter' value is of type {} and is not a boolean.", json[k_CommaAsDelimiterKey].type_name()));
  }
  data.commaAsDelimiter = json[k_CommaAsDelimiterKey];

  if(!json.contains(k_ConsecutiveDelimitersKey))
  {
    return MakeErrorResult<CSVWizardData>(-125, fmt::format("CSVWizardData: Cannot find the 'Consecutive Delimiters' key \"{}\" in the CSVWizardData json object.", k_ConsecutiveDelimitersKey));
  }
  else if(!json[k_ConsecutiveDelimitersKey].is_boolean())
  {
    return MakeErrorResult<CSVWizardData>(-126, fmt::format("CSVWizardData: 'Consecutive Delimiters' value is of type {} and is not a boolean.", json[k_ConsecutiveDelimitersKey].type_name()));
  }
  data.consecutiveDelimiters = json[k_ConsecutiveDelimitersKey];

  return {data};
}
