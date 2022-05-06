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

#include "ASCIIWizardData.hpp"

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
ASCIIWizardData::ASCIIWizardData()
{
  initialize();
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::initialize()
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
void ASCIIWizardData::reset()
{
  initialize();
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::writeJson(nlohmann::json& json) const
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
bool ASCIIWizardData::readJson(const nlohmann::json& json)
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
const std::string& ASCIIWizardData::inputFilePath() const
{
  return m_InputFilePath;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setInputFilePath(const std::string& newInputFilePath)
{
  m_InputFilePath = newInputFilePath;
}

// -----------------------------------------------------------------------------
const std::vector<std::string>& ASCIIWizardData::dataHeaders() const
{
  return m_DataHeaders;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setDataHeaders(const std::vector<std::string>& newDataHeaders)
{
  m_DataHeaders = newDataHeaders;
}

// -----------------------------------------------------------------------------
usize ASCIIWizardData::beginIndex() const
{
  return m_BeginIndex;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setBeginIndex(usize newBeginIndex)
{
  m_BeginIndex = newBeginIndex;
}

// -----------------------------------------------------------------------------
int64 ASCIIWizardData::numberOfLines() const
{
  return m_NumberOfLines;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setNumberOfLines(int64 newNumberOfLines)
{
  m_NumberOfLines = newNumberOfLines;
}

// -----------------------------------------------------------------------------
const std::vector<std::optional<DataType>>& ASCIIWizardData::dataTypes() const
{
  return m_DataTypes;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setDataTypes(const std::vector<std::optional<DataType>>& newDataTypes)
{
  m_DataTypes = newDataTypes;
}

// -----------------------------------------------------------------------------
const std::vector<char>& ASCIIWizardData::delimiters() const
{
  return m_Delimiters;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setDelimiters(const std::vector<char>& newDelimiters)
{
  m_Delimiters = newDelimiters;
}

// -----------------------------------------------------------------------------
usize ASCIIWizardData::headerLine() const
{
  return m_HeaderLine;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setHeaderLine(usize newHeaderLine)
{
  m_HeaderLine = newHeaderLine;
}

// -----------------------------------------------------------------------------
bool ASCIIWizardData::consecutiveDelimiters() const
{
  return m_ConsecutiveDelimiters;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setConsecutiveDelimiters(bool newConsecutiveDelimiters)
{
  m_ConsecutiveDelimiters = newConsecutiveDelimiters;
}

// -----------------------------------------------------------------------------
ASCIIWizardData::HeaderMode ASCIIWizardData::headerMode() const
{
  return m_HeaderMode;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setHeaderMode(ASCIIWizardData::HeaderMode newHeaderMode)
{
  m_HeaderMode = newHeaderMode;
}

// -----------------------------------------------------------------------------
bool ASCIIWizardData::tabAsDelimiter() const
{
  return m_TabAsDelimiter;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setTabAsDelimiter(bool newTabAsDelimiter)
{
  m_TabAsDelimiter = newTabAsDelimiter;
}

// -----------------------------------------------------------------------------
bool ASCIIWizardData::semicolonAsDelimiter() const
{
  return m_SemicolonAsDelimiter;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setSemicolonAsDelimiter(bool newSemicolonAsDelimiter)
{
  m_SemicolonAsDelimiter = newSemicolonAsDelimiter;
}

// -----------------------------------------------------------------------------
bool ASCIIWizardData::commaAsDelimiter() const
{
  return m_CommaAsDelimiter;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setCommaAsDelimiter(bool newCommaAsDelimiter)
{
  m_CommaAsDelimiter = newCommaAsDelimiter;
}

// -----------------------------------------------------------------------------
bool ASCIIWizardData::spaceAsDelimiter() const
{
  return m_SpaceAsDelimiter;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setSpaceAsDelimiter(bool newSpaceAsDelimiter)
{
  m_SpaceAsDelimiter = newSpaceAsDelimiter;
}

// -----------------------------------------------------------------------------
usize ASCIIWizardData::totalPreviewLines() const
{
  return m_TotalPreviewLines;
}

// -----------------------------------------------------------------------------
std::string ASCIIWizardData::skipDataTypeString() const
{
  return m_SkipDataTypeString;
}
