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

#include <optional>

#include "nlohmann/json.hpp"

#include "complex/Common/Result.hpp"
#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
struct COMPLEX_EXPORT CSVWizardData
{
public:
  enum class HeaderMode
  {
    LINE,
    CUSTOM,
    DEFAULTS
  };

  void reset();

  // Json Reader and Writer
  void writeJson(nlohmann::json& json) const;
  static Result<CSVWizardData> ReadJson(const nlohmann::json& json);

  std::string inputFilePath;
  std::vector<std::string> dataHeaders;
  usize beginIndex = 1;
  int64 numberOfLines = -1;
  std::vector<std::optional<DataType>> dataTypes;
  std::vector<char> delimiters;
  usize headerLine = 1;
  HeaderMode headerMode = HeaderMode::LINE;
  bool tabAsDelimiter = false;
  bool semicolonAsDelimiter = false;
  bool commaAsDelimiter = false;
  bool spaceAsDelimiter = false;
  bool consecutiveDelimiters = false;

  static const usize k_TotalPreviewLines = 50;
  static const std::string k_SkipDataTypeString;
};
} // namespace complex
