/* ============================================================================
 * Copyright (c) 2020 BlueQuartz Software, LLC
 * All rights reserved.
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
 * Neither the names of any of the BlueQuartz Software contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
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
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace nx::core::FileUtilities
{

/**
 * @brief
 * @param filepath
 * @return
 */
SIMPLNX_EXPORT int64 LinesInFile(const std::string& filepath);

/**
 * @brief
 * @param filePath
 * @return
 */
SIMPLNX_EXPORT Result<> ValidateCSVFile(const std::string& filePath);

/**
 * @brief
 * @param filePath
 * @return
 */
SIMPLNX_EXPORT bool HasWriteAccess(const std::string& path);

/**
 * @brief
 * @param filePath
 * @return
 */
SIMPLNX_EXPORT Result<> ValidateDirectoryWritePermission(const fs::path& path, bool isFile);

/**
 * @brief
 * @param filePath
 * @return
 */
SIMPLNX_EXPORT std::pair<bool, int32> IsUtf8(const fs::path& filePath);

namespace CSV
{
class AbstractDataParser
{
public:
  virtual ~AbstractDataParser() = default;

  AbstractDataParser(const AbstractDataParser&) = delete;            // Copy Constructor Not Implemented
  AbstractDataParser(AbstractDataParser&&) = delete;                 // Move Constructor Not Implemented
  AbstractDataParser& operator=(const AbstractDataParser&) = delete; // Copy Assignment Not Implemented
  AbstractDataParser& operator=(AbstractDataParser&&) = delete;      // Move Assignment

  [[nodiscard]] std::string columnName() const;

  [[nodiscard]] usize columnIndex() const;

  [[nodiscard]] const IDataArray& dataArray() const;

  virtual Result<> parse(const std::string& token, size_t index) = 0;

protected:
  AbstractDataParser(IDataArray& array, const std::string& columnName, usize columnIndex);

private:
  IDataArray& m_DataArray;
  usize m_ColumnIndex = 0;
  std::string m_ColumnName;
};

template <typename ArrayType, typename T>
class CSVDataParser : public AbstractDataParser
{
public:
  CSVDataParser(ArrayType& array, const std::string& name, usize index)
  : AbstractDataParser(array, name, index)
  , m_Array(array)
  {
  }
  ~CSVDataParser() override = default;

  CSVDataParser(const CSVDataParser&) = delete;            // Copy Constructor Not Implemented
  CSVDataParser(CSVDataParser&&) = delete;                 // Move Constructor Not Implemented
  CSVDataParser& operator=(const CSVDataParser&) = delete; // Copy Assignment Not Implemented
  CSVDataParser& operator=(CSVDataParser&&) = delete;      // Move Assignment

  Result<> parse(const std::string& token, size_t index) override
  {
    Result<T> parseResult = ConvertTo<T>::convert(token);
    if(parseResult.valid())
    {
      m_Array[index] = parseResult.value();
    }

    return ConvertResult(std::move(parseResult));
  }

private:
  ArrayType& m_Array;
};

using Int8Parser = CSVDataParser<Int8Array, int8>;
using UInt8Parser = CSVDataParser<UInt8Array, uint8>;

using Int16Parser = CSVDataParser<Int16Array, int16>;
using UInt16Parser = CSVDataParser<UInt16Array, uint16>;

using Int32Parser = CSVDataParser<Int32Array, int32>;
using UInt32Parser = CSVDataParser<UInt32Array, uint32>;

using Int64Parser = CSVDataParser<Int64Array, int64>;
using UInt64Parser = CSVDataParser<UInt64Array, uint64>;

using Float32Parser = CSVDataParser<Float32Array, float32>;
using Float64Parser = CSVDataParser<Float64Array, float64>;

using BoolParser = CSVDataParser<BoolArray, bool>;

// using StringParser = Parser<StringArray, std::string>;

using ParsersVector = std::vector<std::unique_ptr<AbstractDataParser>>;

/**
 *
 * @param dataTypes
 * @param skippedArrays
 * @param parentPath
 * @param headers
 * @param dataStructure
 * @return
 */
SIMPLNX_EXPORT Result<ParsersVector> CreateParsers(const std::vector<DataType>& dataTypes, const std::vector<bool>& skippedArrays, const DataPath& parentPath, const std::vector<std::string>& headers,
                                                   DataStructure& dataStructure);

/**
 *
 * @param inStream
 * @param dataParsers
 * @param headers
 * @param delimiters
 * @param consecutiveDelimiters
 * @param lineNumber
 * @param beginIndex
 * @return
 */
SIMPLNX_EXPORT Result<> ParseLine(std::fstream& inStream, const ParsersVector& dataParsers, const std::vector<std::string>& headers, const std::vector<char>& delimiters, bool consecutiveDelimiters,
                                  usize lineNumber, usize beginIndex);

/**
 *
 * @param tupleDims
 * @return
 */
SIMPLNX_EXPORT std::string TupleDimsToString(const std::vector<usize>& tupleDims);

/**
 *
 * @param inStream
 * @param numberOfLines
 * @return
 */
SIMPLNX_EXPORT bool SkipNumberOfLines(std::fstream& inStream, usize numberOfLines);

/**
 *
 * @param inputFilePath
 * @param headersLineNum
 * @return
 */
SIMPLNX_EXPORT Result<std::string> ReadHeaders(const std::string& inputFilePath, usize headersLineNum);
} // namespace CSV
} // namespace nx::core::FileUtilities
