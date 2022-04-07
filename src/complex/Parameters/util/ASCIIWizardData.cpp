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

// -----------------------------------------------------------------------------
ASCIIWizardData::ASCIIWizardData() = default;

// -----------------------------------------------------------------------------
bool ASCIIWizardData::isEmpty()
{
  return (m_InputFilePath.empty() && m_DataHeaders.empty() && m_DataTypes.empty() && m_TupleDims.empty() && m_BeginIndex < 0 && m_NumberOfLines < 0 && m_SelectedPath.empty());
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::writeJson(nlohmann::json& json) const
{
  nlohmann::json dHeaders;
  for(const auto& header : m_DataHeaders)
  {
    dHeaders.push_back(header);
  }
  json["Data Headers"] = dHeaders;

  nlohmann::json dTypes;
  for(const auto& dType : m_DataTypes)
  {
    dTypes.push_back(dType);
  }
  json["Data Types"] = dTypes;

  nlohmann::json delimiters;
  for(const auto& delimiter : m_Delimiters)
  {
    delimiters.push_back(delimiter);
  }
  json["Delimiters"] = delimiters;

  nlohmann::json tDims;
  for(const auto& tDim : m_TupleDims)
  {
    tDims.push_back(tDim);
  }
  json["Tuple Dimensions"] = tDims;

  json["Input File Path"] = m_InputFilePath;
  json["Begin Index"] = m_BeginIndex;
  json["Number of Lines"] = m_NumberOfLines;
  json["Data Path"] = m_SelectedPath.toString();
  json["Header Line"] = m_HeaderLine;
  json["Header Mode"] = m_HeaderMode;
  json["Consecutive Delimiters"] = m_ConsecutiveDelimiters;
  json["Automatic Group"] = m_AutomaticGroup;
}

// -----------------------------------------------------------------------------
bool ASCIIWizardData::readJson(const nlohmann::json& json)
{
  nlohmann::json dHeaders = json["Data Headers"];
  for(const auto& header : dHeaders)
  {
    if(header.is_string())
    {
      m_DataHeaders.push_back(header);
    }
  }

  nlohmann::json dTypes = json["Data Types"];
  for(const auto& dType : dTypes)
  {
    if(dType.is_string())
    {
      m_DataTypes.push_back(dType);
    }
  }

  nlohmann::json delimiters = json["Delimiters"];
  for(const auto& delimiter : delimiters)
  {
    if(delimiter.is_number_integer())
    {
      m_Delimiters.push_back(delimiter);
    }
  }

  nlohmann::json tDims = json["Tuple Dimensions"];
  for(const auto& tDim : tDims)
  {
    if(tDim.is_number_integer())
    {
      m_TupleDims.push_back(tDim);
    }
  }

  m_InputFilePath = json["Input File Path"];
  m_BeginIndex = json["Begin Index"];
  m_NumberOfLines = json["Number of Lines"];
  m_HeaderLine = json["Header Line"];
  m_HeaderMode = json["Header Mode"];
  m_ConsecutiveDelimiters = json["Consecutive Delimiters"];
  m_AutomaticGroup = json["Automatic Group"];

  std::optional<DataPath> dPath = DataPath::FromString(json["Data Path"].dump());
  if(dPath.has_value())
  {
    m_SelectedPath = dPath.value();
  }

  return true;
}

// -----------------------------------------------------------------------------
const std::string& ASCIIWizardData::getInputFilePath() const
{
  return m_InputFilePath;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setInputFilePath(const std::string& newInputFilePath)
{
  m_InputFilePath = newInputFilePath;
}

// -----------------------------------------------------------------------------
const std::list<std::string>& ASCIIWizardData::getDataHeaders() const
{
  return m_DataHeaders;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setDataHeaders(const std::list<std::string>& newDataHeaders)
{
  m_DataHeaders = newDataHeaders;
}

// -----------------------------------------------------------------------------
usize ASCIIWizardData::getBeginIndex() const
{
  return m_BeginIndex;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setBeginIndex(usize newBeginIndex)
{
  m_BeginIndex = newBeginIndex;
}

// -----------------------------------------------------------------------------
usize ASCIIWizardData::getNumberOfLines() const
{
  return m_NumberOfLines;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setNumberOfLines(usize newNumberOfLines)
{
  m_NumberOfLines = newNumberOfLines;
}

// -----------------------------------------------------------------------------
const std::list<std::string>& ASCIIWizardData::getDataTypes() const
{
  return m_DataTypes;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setDataTypes(const std::list<std::string>& newDataTypes)
{
  m_DataTypes = newDataTypes;
}

// -----------------------------------------------------------------------------
const std::list<uint8>& ASCIIWizardData::getDelimiters() const
{
  return m_Delimiters;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setDelimiters(const std::list<uint8>& newDelimiters)
{
  m_Delimiters = newDelimiters;
}

// -----------------------------------------------------------------------------
const std::vector<usize>& ASCIIWizardData::getTupleDims() const
{
  return m_TupleDims;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setTupleDims(const std::vector<usize>& newTupleDims)
{
  m_TupleDims = newTupleDims;
}

// -----------------------------------------------------------------------------
const DataPath& ASCIIWizardData::getSelectedPath() const
{
  return m_SelectedPath;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setSelectedPath(const DataPath& newSelectedPath)
{
  m_SelectedPath = newSelectedPath;
}

// -----------------------------------------------------------------------------
usize ASCIIWizardData::getHeaderLine() const
{
  return m_HeaderLine;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setHeaderLine(usize newHeaderLine)
{
  m_HeaderLine = newHeaderLine;
}

// -----------------------------------------------------------------------------
bool ASCIIWizardData::getConsecutiveDelimiters() const
{
  return m_ConsecutiveDelimiters;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setConsecutiveDelimiters(bool newConsecutiveDelimiters)
{
  m_ConsecutiveDelimiters = newConsecutiveDelimiters;
}

// -----------------------------------------------------------------------------
bool ASCIIWizardData::getAutomaticGroup() const
{
  return m_AutomaticGroup;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setAutomaticGroup(bool newAutomaticGroup)
{
  m_AutomaticGroup = newAutomaticGroup;
}

// -----------------------------------------------------------------------------
ASCIIWizardData::HeaderMode ASCIIWizardData::getHeaderMode() const
{
  return m_HeaderMode;
}

// -----------------------------------------------------------------------------
void ASCIIWizardData::setHeaderMode(ASCIIWizardData::HeaderMode newHeaderMode)
{
  m_HeaderMode = newHeaderMode;
}
