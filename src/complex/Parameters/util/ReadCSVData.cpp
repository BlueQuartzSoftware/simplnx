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

#include "ReadCSVData.hpp"

#include <fmt/format.h>

using namespace complex;

namespace
{
const std::string k_CustomHeadersKey = "Custom Headers";
const std::string k_DataTypesKey = "Data Types";
const std::string k_SkippedArrayMaskKey = "Skipped Array Mask";
const std::string k_TupleDimensionsKey = "Tuple Dimensions";
const std::string k_InputFilePathKey = "Input File Path";
const std::string k_StartImportRowKey = "Start Import Row";
const std::string k_HeaderLineKey = "Header Line";
const std::string k_HeaderModeKey = "Header Mode";
const std::string k_Delimiters = "Delimiters";
const std::string k_ConsecutiveDelimitersKey = "Consecutive Delimiters";
} // namespace

// -----------------------------------------------------------------------------
nlohmann::json ReadCSVData::writeJson() const
{
  nlohmann::json json;

  nlohmann::json dHeaders;
  for(const auto& header : customHeaders)
  {
    dHeaders.push_back(header);
  }
  json[k_CustomHeadersKey] = dHeaders;

  nlohmann::json dTypes;
  for(const auto& dType : dataTypes)
  {
    dTypes.push_back(dType);
  }
  json[k_DataTypesKey] = dTypes;

  nlohmann::json tDims;
  for(const auto& tDim : tupleDims)
  {
    tDims.push_back(tDim);
  }
  json[k_TupleDimensionsKey] = tDims;

  nlohmann::json dSkippedArrays;
  for(const auto& skippedArrayVal : skippedArrayMask)
  {
    dSkippedArrays.push_back(skippedArrayVal);
  }
  json[k_SkippedArrayMaskKey] = dSkippedArrays;

  json[k_InputFilePathKey] = inputFilePath;
  json[k_StartImportRowKey] = startImportRow;
  json[k_HeaderLineKey] = headersLine;
  json[k_HeaderModeKey] = headerMode;
  json[k_Delimiters] = delimiters;
  json[k_ConsecutiveDelimitersKey] = consecutiveDelimiters;

  return json;
}

// -----------------------------------------------------------------------------
Result<ReadCSVData> ReadCSVData::ReadJson(const nlohmann::json& json)
{
  ReadCSVData data;

  if(!json.contains(k_CustomHeadersKey))
  {
    return MakeErrorResult<ReadCSVData>(-100, fmt::format("ReadCSVData: Cannot find the Data Headers key \"{}\" in the ReadCSVData json object.", k_CustomHeadersKey));
  }

  nlohmann::json dHeaders = json[k_CustomHeadersKey];
  for(usize i = 0; i < dHeaders.size(); i++)
  {
    auto header = dHeaders[i];
    if(!header.is_string())
    {
      return MakeErrorResult<ReadCSVData>(-101, fmt::format("ReadCSVData: Custom header at index {} is of type {} and is not a string.", std::to_string(i), header.type_name()));
    }

    data.customHeaders.push_back(header);
  }

  if(!json.contains(k_DataTypesKey))
  {
    return MakeErrorResult<ReadCSVData>(-102, fmt::format("ReadCSVData: Cannot find the Data Types key \"{}\" in the ReadCSVData json object.", k_DataTypesKey));
  }

  nlohmann::json dTypes = json[k_DataTypesKey];
  for(usize i = 0; i < dTypes.size(); i++)
  {
    auto dType = dTypes[i];
    if(!dType.is_number_integer())
    {
      return MakeErrorResult<ReadCSVData>(-103, fmt::format("ReadCSVData: Data type at index {} is of type {} and is not an integer.", std::to_string(i), dType.type_name()));
    }

    data.dataTypes.push_back(dType);
  }

  if(!json.contains(k_TupleDimensionsKey))
  {
    return MakeErrorResult<ReadCSVData>(-104, fmt::format("ReadCSVData: Cannot find the Tuple Dimensions key \"{}\" in the ReadCSVData json object.", k_TupleDimensionsKey));
  }

  nlohmann::json tDims = json[k_TupleDimensionsKey];
  data.tupleDims.clear();
  for(usize i = 0; i < tDims.size(); i++)
  {
    auto tDim = tDims[i];
    data.tupleDims.push_back(tDim);
  }

  if(!json.contains(k_SkippedArrayMaskKey))
  {
    return MakeErrorResult<ReadCSVData>(-105, fmt::format("ReadCSVData: Cannot find the Skipped Arrays key \"{}\" in the ReadCSVData json object.", k_DataTypesKey));
  }

  nlohmann::json dSkippedArrays = json[k_SkippedArrayMaskKey];
  for(usize i = 0; i < dSkippedArrays.size(); i++)
  {
    auto skippedArrayVal = dSkippedArrays[i];
    if(!skippedArrayVal.is_boolean())
    {
      return MakeErrorResult<ReadCSVData>(-106, fmt::format("ReadCSVData: Skipped array value at index {} is of type {} and is not a boolean.", std::to_string(i), skippedArrayVal.type_name()));
    }

    data.skippedArrayMask.push_back(skippedArrayVal);
  }

  if(!json.contains(k_InputFilePathKey))
  {
    return MakeErrorResult<ReadCSVData>(-107, fmt::format("ReadCSVData: Cannot find the 'Input File Path' key \"{}\" in the ReadCSVData json object.", k_InputFilePathKey));
  }
  else if(!json[k_InputFilePathKey].is_string())
  {
    return MakeErrorResult<ReadCSVData>(-108, fmt::format("ReadCSVData: 'Input File Path' value is of type {} and is not a string.", json[k_InputFilePathKey].type_name()));
  }
  data.inputFilePath = json[k_InputFilePathKey];

  if(!json.contains(k_StartImportRowKey))
  {
    return MakeErrorResult<ReadCSVData>(-109, fmt::format("ReadCSVData: Cannot find the 'Begin Index' key \"{}\" in the ReadCSVData json object.", k_StartImportRowKey));
  }
  else if(!json[k_StartImportRowKey].is_number_integer())
  {
    return MakeErrorResult<ReadCSVData>(-110, fmt::format("ReadCSVData: 'Begin Index' value is of type {} and is not an integer.", json[k_StartImportRowKey].type_name()));
  }
  data.startImportRow = json[k_StartImportRowKey];

  if(!json.contains(k_HeaderLineKey))
  {
    return MakeErrorResult<ReadCSVData>(-113, fmt::format("ReadCSVData: Cannot find the 'Header Line' key \"{}\" in the ReadCSVData json object.", k_HeaderLineKey));
  }
  else if(!json[k_HeaderLineKey].is_number_integer())
  {
    return MakeErrorResult<ReadCSVData>(-114, fmt::format("ReadCSVData: 'Header Line' value is of type {} and is not an integer.", json[k_HeaderLineKey].type_name()));
  }
  data.headersLine = json[k_HeaderLineKey];

  if(!json.contains(k_HeaderModeKey))
  {
    return MakeErrorResult<ReadCSVData>(-115, fmt::format("ReadCSVData: Cannot find the 'Header Mode' key \"{}\" in the ReadCSVData json object.", k_HeaderModeKey));
  }
  else if(!json[k_HeaderModeKey].is_number_integer())
  {
    return MakeErrorResult<ReadCSVData>(-116, fmt::format("ReadCSVData: 'Header Mode' value is of type {} and is not an integer.", json[k_HeaderModeKey].type_name()));
  }
  data.headerMode = json[k_HeaderModeKey];

  if(!json.contains(k_Delimiters))
  {
    return MakeErrorResult<ReadCSVData>(-117, fmt::format("ReadCSVData: Cannot find the 'Delimiters' key \"{}\" in the ReadCSVData json object.", k_Delimiters));
  }

  nlohmann::json dDelimiters = json[k_Delimiters];
  for(usize i = 0; i < dDelimiters.size(); i++)
  {
    auto delimiter = dDelimiters[i];
    if(!delimiter.is_string() || delimiter.get<std::string>().size() != 1)
    {
      return MakeErrorResult<ReadCSVData>(-118, fmt::format("ReadCSVData: Delimiter at index {} is of type {} and is not a boolean.", std::to_string(i), delimiter.type_name()));
    }

    data.delimiters.push_back(delimiter.get<std::string>()[0]);
  }

  if(!json.contains(k_ConsecutiveDelimitersKey))
  {
    return MakeErrorResult<ReadCSVData>(-125, fmt::format("ReadCSVData: Cannot find the 'Consecutive Delimiters' key \"{}\" in the ReadCSVData json object.", k_ConsecutiveDelimitersKey));
  }
  else if(!json[k_ConsecutiveDelimitersKey].is_boolean())
  {
    return MakeErrorResult<ReadCSVData>(-126, fmt::format("ReadCSVData: 'Consecutive Delimiters' value is of type {} and is not a boolean.", json[k_ConsecutiveDelimitersKey].type_name()));
  }
  data.consecutiveDelimiters = json[k_ConsecutiveDelimitersKey];

  return {data};
}
