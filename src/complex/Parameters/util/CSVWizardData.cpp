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

const std::string CSVWizardData::k_SkipDataTypeString = "Skip";

// -----------------------------------------------------------------------------
void CSVWizardData::writeJson(nlohmann::json& json) const
{
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
}

// -----------------------------------------------------------------------------
Result<CSVWizardData> CSVWizardData::ReadJson(const nlohmann::json& json)
{
  CSVWizardData data;

  nlohmann::json dHeaders = json[k_DataHeadersKey];
  for(const auto& header : dHeaders)
  {
    if(header.is_string())
    {
      data.dataHeaders.push_back(header);
    }
  }

  nlohmann::json dTypes = json[k_DataTypesKey];
  for(const auto& dType : dTypes)
  {
    if(dType.is_string() && dType == k_SkipDataTypeString)
    {
      data.dataTypes.push_back({});
    }
    else
    {
      data.dataTypes.push_back(dType);
    }
  }

  nlohmann::json delimiters = json[k_DelimitersKey];
  for(const auto& delimiterObj : delimiters)
  {
    if(delimiterObj.is_number_integer())
    {
      data.delimiters.push_back(delimiterObj.get<char>());
    }
  }

  data.inputFilePath = json[k_InputFilePathKey];
  data.beginIndex = json[k_BeginIndexKey];
  data.numberOfLines = json[k_NumberOfLinesKey];
  data.headerLine = json[k_HeaderLineKey];
  data.headerMode = json[k_HeaderModeKey];
  data.tabAsDelimiter = json[k_TabAsDelimiterKey];
  data.semicolonAsDelimiter = json[k_SemicolonAsDelimiterKey];
  data.spaceAsDelimiter = json[k_SpaceAsDelimiterKey];
  data.commaAsDelimiter = json[k_CommaAsDelimiterKey];
  data.consecutiveDelimiters = json[k_ConsecutiveDelimitersKey];

  return {data};
}
