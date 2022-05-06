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
static const std::string k_DataHeadersKey = "Data Headers";
static const std::string k_DataTypesKey = "Data Types";
static const std::string k_DelimitersKey = "Delimiters";
static const std::string k_TupleDimensionsKey = "Tuple Dimensions";
static const std::string k_InputFilePathKey = "Input File Path";
static const std::string k_BeginIndexKey = "Begin Index";
static const std::string k_NumberOfLinesKey = "Number of Lines";
static const std::string k_HeaderLineKey = "Header Line";
static const std::string k_HeaderModeKey = "Header Mode";
static const std::string k_TabAsDelimiterKey = "Tab As Delimiter";
static const std::string k_SemicolonAsDelimiterKey = "Semicolon As Delimiter";
static const std::string k_CommaAsDelimiterKey = "Comma As Delimiter";
static const std::string k_SpaceAsDelimiterKey = "Space As Delimiter";
static const std::string k_ConsecutiveDelimitersKey = "Consecutive Delimiters";
} // namespace

// -----------------------------------------------------------------------------
CSVWizardData::CSVWizardData()
{
  initialize();
}

// -----------------------------------------------------------------------------
void CSVWizardData::initialize()
{
  m_InputFilePath = "";
  m_DataHeaders.clear();
  m_DataTypes.clear();
  m_BeginIndex = 1;
  m_NumberOfLines = -1;
  m_Delimiters.clear();
  m_HeaderLine = 1;
  m_HeaderMode = HeaderMode::LINE;
  m_TabAsDelimiter = false;
  m_SemicolonAsDelimiter = false;
  m_CommaAsDelimiter = false;
  m_SpaceAsDelimiter = false;
  m_ConsecutiveDelimiters = false;
}

// -----------------------------------------------------------------------------
void CSVWizardData::reset()
{
  initialize();
}

// -----------------------------------------------------------------------------
void CSVWizardData::writeJson(nlohmann::json& json) const
{
  nlohmann::json dHeaders;
  for(const auto& header : m_DataHeaders)
  {
    dHeaders.push_back(header);
  }
  json[k_DataHeadersKey] = dHeaders;

  nlohmann::json dTypes;
  for(const auto& dType : m_DataTypes)
  {
    if(!dType.has_value())
    {
      dTypes.push_back(m_SkipDataTypeString);
    }
    else
    {
      dTypes.push_back(dType.value());
    }
  }
  json[k_DataTypesKey] = dTypes;

  nlohmann::json delimiters;
  for(const auto& delimiter : m_Delimiters)
  {
    delimiters.push_back(delimiter);
  }
  json[k_DelimitersKey] = delimiters;

  json[k_InputFilePathKey] = m_InputFilePath;
  json[k_BeginIndexKey] = m_BeginIndex;
  json[k_NumberOfLinesKey] = m_NumberOfLines;
  json[k_HeaderLineKey] = m_HeaderLine;
  json[k_HeaderModeKey] = m_HeaderMode;
  json[k_TabAsDelimiterKey] = m_TabAsDelimiter;
  json[k_SemicolonAsDelimiterKey] = m_SemicolonAsDelimiter;
  json[k_SpaceAsDelimiterKey] = m_SpaceAsDelimiter;
  json[k_CommaAsDelimiterKey] = m_CommaAsDelimiter;
  json[k_ConsecutiveDelimitersKey] = m_ConsecutiveDelimiters;
}

// -----------------------------------------------------------------------------
bool CSVWizardData::readJson(const nlohmann::json& json)
{
  nlohmann::json dHeaders = json[k_DataHeadersKey];
  for(const auto& header : dHeaders)
  {
    if(header.is_string())
    {
      m_DataHeaders.push_back(header);
    }
  }

  nlohmann::json dTypes = json[k_DataTypesKey];
  for(const auto& dType : dTypes)
  {
    if(dType.is_string() && dType == m_SkipDataTypeString)
    {
      m_DataTypes.push_back({});
    }
    else
    {
      m_DataTypes.push_back(dType);
    }
  }

  nlohmann::json delimiters = json[k_DelimitersKey];
  for(const auto& delimiterObj : delimiters)
  {
    if(delimiterObj.is_number_integer())
    {
      m_Delimiters.push_back(delimiterObj.get<char>());
    }
  }

  m_InputFilePath = json[k_InputFilePathKey];
  m_BeginIndex = json[k_BeginIndexKey];
  m_NumberOfLines = json[k_NumberOfLinesKey];
  m_HeaderLine = json[k_HeaderLineKey];
  m_HeaderMode = json[k_HeaderModeKey];
  m_TabAsDelimiter = json[k_TabAsDelimiterKey];
  m_SemicolonAsDelimiter = json[k_SemicolonAsDelimiterKey];
  m_SpaceAsDelimiter = json[k_SpaceAsDelimiterKey];
  m_CommaAsDelimiter = json[k_CommaAsDelimiterKey];
  m_ConsecutiveDelimiters = json[k_ConsecutiveDelimitersKey];

  return true;
}

// -----------------------------------------------------------------------------
const std::string& CSVWizardData::inputFilePath() const
{
  return m_InputFilePath;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setInputFilePath(const std::string& newInputFilePath)
{
  m_InputFilePath = newInputFilePath;
}

// -----------------------------------------------------------------------------
const std::vector<std::string>& CSVWizardData::dataHeaders() const
{
  return m_DataHeaders;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setDataHeaders(const std::vector<std::string>& newDataHeaders)
{
  m_DataHeaders = newDataHeaders;
}

// -----------------------------------------------------------------------------
usize CSVWizardData::beginIndex() const
{
  return m_BeginIndex;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setBeginIndex(usize newBeginIndex)
{
  m_BeginIndex = newBeginIndex;
}

// -----------------------------------------------------------------------------
int64 CSVWizardData::numberOfLines() const
{
  return m_NumberOfLines;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setNumberOfLines(int64 newNumberOfLines)
{
  m_NumberOfLines = newNumberOfLines;
}

// -----------------------------------------------------------------------------
const std::vector<std::optional<DataType>>& CSVWizardData::dataTypes() const
{
  return m_DataTypes;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setDataTypes(const std::vector<std::optional<DataType>>& newDataTypes)
{
  m_DataTypes = newDataTypes;
}

// -----------------------------------------------------------------------------
const std::vector<char>& CSVWizardData::delimiters() const
{
  return m_Delimiters;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setDelimiters(const std::vector<char>& newDelimiters)
{
  m_Delimiters = newDelimiters;
}

// -----------------------------------------------------------------------------
usize CSVWizardData::headerLine() const
{
  return m_HeaderLine;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setHeaderLine(usize newHeaderLine)
{
  m_HeaderLine = newHeaderLine;
}

// -----------------------------------------------------------------------------
bool CSVWizardData::consecutiveDelimiters() const
{
  return m_ConsecutiveDelimiters;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setConsecutiveDelimiters(bool newConsecutiveDelimiters)
{
  m_ConsecutiveDelimiters = newConsecutiveDelimiters;
}

// -----------------------------------------------------------------------------
CSVWizardData::HeaderMode CSVWizardData::headerMode() const
{
  return m_HeaderMode;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setHeaderMode(CSVWizardData::HeaderMode newHeaderMode)
{
  m_HeaderMode = newHeaderMode;
}

// -----------------------------------------------------------------------------
bool CSVWizardData::tabAsDelimiter() const
{
  return m_TabAsDelimiter;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setTabAsDelimiter(bool newTabAsDelimiter)
{
  m_TabAsDelimiter = newTabAsDelimiter;
}

// -----------------------------------------------------------------------------
bool CSVWizardData::semicolonAsDelimiter() const
{
  return m_SemicolonAsDelimiter;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setSemicolonAsDelimiter(bool newSemicolonAsDelimiter)
{
  m_SemicolonAsDelimiter = newSemicolonAsDelimiter;
}

// -----------------------------------------------------------------------------
bool CSVWizardData::commaAsDelimiter() const
{
  return m_CommaAsDelimiter;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setCommaAsDelimiter(bool newCommaAsDelimiter)
{
  m_CommaAsDelimiter = newCommaAsDelimiter;
}

// -----------------------------------------------------------------------------
bool CSVWizardData::spaceAsDelimiter() const
{
  return m_SpaceAsDelimiter;
}

// -----------------------------------------------------------------------------
void CSVWizardData::setSpaceAsDelimiter(bool newSpaceAsDelimiter)
{
  m_SpaceAsDelimiter = newSpaceAsDelimiter;
}

// -----------------------------------------------------------------------------
usize CSVWizardData::totalPreviewLines() const
{
  return m_TotalPreviewLines;
}

// -----------------------------------------------------------------------------
std::string CSVWizardData::skipDataTypeString() const
{
  return m_SkipDataTypeString;
}
