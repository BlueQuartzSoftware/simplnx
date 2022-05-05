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

  void reset();

  // Json Reader and Writer
  void writeJson(nlohmann::json& json) const;
  bool readJson(const nlohmann::json& json);

  // Getters and Setters
  const std::string& inputFilePath() const;
  void setInputFilePath(const std::string& newInputFilePath);

  const std::vector<std::string>& dataHeaders() const;
  void setDataHeaders(const std::vector<std::string>& newDataHeaders);

  usize beginIndex() const;
  void setBeginIndex(usize newBeginIndex);

  int64 numberOfLines() const;
  void setNumberOfLines(int64 newNumberOfLines);

  const std::vector<std::optional<DataType>>& dataTypes() const;
  void setDataTypes(const std::vector<std::optional<DataType>>& newDataTypes);

  const std::vector<char>& delimiters() const;
  void setDelimiters(const std::vector<char>& newDelimiters);

  const DataPath& selectedGroupPath() const;
  void setSelectedGroupPath(const DataPath& selectedGroupPath);

  usize headerLine() const;
  void setHeaderLine(usize newHeaderLine);

  bool headerIsCustom() const;
  void setHeaderIsCustom(bool newHeaderIsCustom);

  bool consecutiveDelimiters() const;
  void setConsecutiveDelimiters(bool newConsecutiveDelimiters);

  bool useExistingGroup() const;
  void setUseExistingGroup(bool useExistingGroup);

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
  std::string skipDataTypeString() const;

protected:
  void initialize();

private:
  std::string m_InputFilePath;
  std::vector<std::string> m_DataHeaders;
  usize m_BeginIndex;
  int64 m_NumberOfLines;
  std::vector<std::optional<DataType>> m_DataTypes;
  std::vector<char> m_Delimiters;
  DataPath m_SelectedGroupPath;

  usize m_HeaderLine;
  HeaderMode m_HeaderMode;
  bool m_TabAsDelimiter;
  bool m_SemicolonAsDelimiter;
  bool m_CommaAsDelimiter;
  bool m_SpaceAsDelimiter;
  bool m_ConsecutiveDelimiters;
  bool m_UseExistingGroup;

  const usize m_TotalPreviewLines = 50;
  const std::string m_SkipDataTypeString = "Skip";
};
} // namespace complex
