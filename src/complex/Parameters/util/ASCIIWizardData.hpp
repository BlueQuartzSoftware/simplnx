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
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
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

#pragma once

#include <list>

#include "nlohmann/json.hpp"

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT ASCIIWizardData
{
public:
  ASCIIWizardData();

  enum class HeaderMode
  {
    LINE,
    CUSTOM,
    DEFAULTS
  };

  bool isEmpty();

  // Json Reader and Writer
  void writeJson(nlohmann::json& json) const;
  bool readJson(const nlohmann::json& json);

  // Getters and Setters
  const std::string& inputFilePath() const;
  void setInputFilePath(const std::string& newInputFilePath);

  const std::list<std::string>& dataHeaders() const;
  void setDataHeaders(const std::list<std::string>& newDataHeaders);

  usize beginIndex() const;
  void setBeginIndex(usize newBeginIndex);

  int64 numberOfLines() const;
  void setNumberOfLines(int64 newNumberOfLines);

  const std::list<std::string>& dataTypes() const;
  void setDataTypes(const std::list<std::string>& newDataTypes);

  const std::list<std::string>& delimiters() const;
  void setDelimiters(const std::list<std::string>& newDelimiters);

  //  const std::vector<usize>& tupleDims() const;
  //  void setTupleDims(const std::vector<usize>& newTupleDims);

  //  const DataPath& selectedPath() const;
  //  void setSelectedPath(const DataPath& newSelectedPath);

  usize headerLine() const;
  void setHeaderLine(usize newHeaderLine);

  bool headerIsCustom() const;
  void setHeaderIsCustom(bool newHeaderIsCustom);

  bool consecutiveDelimiters() const;
  void setConsecutiveDelimiters(bool newConsecutiveDelimiters);

  bool automaticGroup() const;
  void setAutomaticGroup(bool newAutomaticGroup);

  HeaderMode headerMode() const;
  void setHeaderMode(HeaderMode newHeaderMode);

  bool tabAsDelimiter() const;
  void setTabAsDelimiter(bool newTabAsDelimiter);

  bool semicolonAsDelimiter() const;
  void setSemicolonAsDelimiter(bool newSemicolonAsDelimiter);

  bool commaAsDelimiter() const;
  void setCommaAsDelimiter(bool newCommaAsDelimiter);

  bool spaceAsDelimiter() const;
  void setSpaceAsDelimiter(bool newSpaceAsDelimiter);

  usize totalPreviewLines() const;

private:
  std::string m_InputFilePath;
  std::list<std::string> m_DataHeaders;
  usize m_BeginIndex = 1;
  int64 m_NumberOfLines = -1;
  std::list<std::string> m_DataTypes;
  std::list<std::string> m_Delimiters;
  //  std::vector<usize> m_TupleDims;
  //  DataPath m_SelectedPath;

  usize m_HeaderLine = 0; // -1 means this selection is not used
  HeaderMode m_HeaderMode = HeaderMode::LINE;
  bool m_TabAsDelimiter = false;
  bool m_SemicolonAsDelimiter = false;
  bool m_CommaAsDelimiter = false;
  bool m_SpaceAsDelimiter = false;
  bool m_ConsecutiveDelimiters = false;
  bool m_AutomaticGroup = false;

  usize m_TotalPreviewLines = 50;
};
} // namespace complex
