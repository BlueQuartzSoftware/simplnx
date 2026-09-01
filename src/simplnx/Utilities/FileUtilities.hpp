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
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/util/ReadCSVData.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

#include <filesystem>
#include <memory>
#include <regex>
#include <string>
#include <type_traits>
#include <vector>

namespace nx::core::FileUtilities
{

/**
 * @brief Counts text lines in a file.
 * @param filepath Identifies the file to read.
 * @return Line count, or -1 if the file cannot be opened or read.
 *
 * A final line without a newline still counts as one line. An empty file returns -1.
 */
SIMPLNX_EXPORT int64 LinesInFile(const std::string& filepath);

/**
 * @brief Performs a bounded text-file check for CSV input.
 * @param filePath Identifies the candidate CSV file.
 * @return Valid result when the initial segment contains supported text and line controls.
 *
 * The check reads at most 2,048 bytes after an optional UTF-8 byte-order mark.
 * A first line longer than this limit can be rejected as possible binary data.
 */
SIMPLNX_EXPORT Result<> ValidateCSVFile(const std::string& filePath);

/**
 * @brief Tests operating-system write access for an existing path.
 * @param path Identifies the path to test.
 * @return True when the platform access check reports write permission.
 */
SIMPLNX_EXPORT bool HasWriteAccess(const std::string& path);

/**
 * @brief Checks write permission on the nearest existing directory ancestor.
 * @param path Identifies a target file or directory.
 * @param isFile True when path includes a file name that must be removed before the check.
 * @return Valid result when an existing ancestor is writable.
 *
 * The function does not create directories. Relative paths are made absolute.
 * A nonexistent Windows drive produces an error.
 */
SIMPLNX_EXPORT Result<> ValidateDirectoryWritePermission(const std::filesystem::path& path, bool isFile);

/**
 * @brief Tests whether a file starts with the UTF-8 byte-order mark.
 * @param filePath Identifies the file to inspect.
 * @return Pair of byte-order-mark state and status code. Status -1 means read failure or a file shorter than three bytes.
 *
 * This function does not validate UTF-8 content after the first three bytes.
 */
SIMPLNX_EXPORT std::pair<bool, int32> IsUtf8(const std::filesystem::path& filePath);

namespace CSV
{
/**
 * @class AbstractDataParser
 * @brief Parses one CSV column into its destination array and exposes flush
 * operations so buffered numeric data can be committed in bounded bulk writes.
 *
 * A parser is mutable and not thread-safe. Its destructor does not flush pending
 * values. Call FlushParsers after the final input line and at each requested flush.
 */
class SIMPLNX_EXPORT AbstractDataParser
{
public:
  virtual ~AbstractDataParser() = default;

  AbstractDataParser(const AbstractDataParser&) = delete;
  AbstractDataParser(AbstractDataParser&&) = delete;
  AbstractDataParser& operator=(const AbstractDataParser&) = delete;
  AbstractDataParser& operator=(AbstractDataParser&&) = delete;

  [[nodiscard]] std::string columnName() const;

  [[nodiscard]] usize columnIndex() const;

  [[nodiscard]] const IArray& array() const;

  /**
   * @brief Parses one token for a destination value index.
   * @param token Supplies the text value.
   * @param index Identifies the flat destination value.
   * @param flushRequired Receives true when a bounded numeric buffer becomes full.
   * @return Parse or destination-write result.
   * @pre Buffered calls use consecutive indices. The caller flushes before the next
   * parse when flushRequired becomes true.
   */
  virtual Result<> parse(const std::string& token, usize index, bool& flushRequired) = 0;

  /**
   * @brief Commits any parsed values that have not yet reached the destination array.
   * @return Bulk-write result. A successful flush clears the pending buffer.
   */
  virtual Result<> flush() = 0;

protected:
  /**
   * @brief Stores one non-owning destination and its CSV column identity.
   * @param array Supplies the destination array and must outlive this parser.
   * @param columnName Specifies the column name for diagnostics.
   * @param columnIndex Specifies the token position in each input row.
   */
  AbstractDataParser(IArray& array, const std::string& columnName, usize columnIndex);

private:
  IArray& m_Array;
  usize m_ColumnIndex = 0;
  std::string m_ColumnName;
};

/**
 * @class CSVDataParser
 * @brief Parses one CSV column while buffering primitive values for contiguous
 * bulk writes. String values retain direct assignment because StringArray has
 * required in-memory storage semantics.
 * @tparam ArrayType Specifies the concrete destination array type.
 * @tparam T Specifies the parsed value type.
 *
 * Exact DataStore destinations use a direct pointer unless the OOC path is forced.
 * Other numeric stores use a 65,536-value buffer, including OOC and nonstandard
 * in-memory stores. The parser is not thread-safe and does not flush during destruction.
 * @pre The destination array has one component per CSV row.
 */
template <typename ArrayType, typename T>
class CSVDataParser : public AbstractDataParser
{
public:
  /**
   * @brief Creates a parser for one existing destination array.
   * @param array Supplies the destination and must outlive this parser.
   * @param name Specifies the column name for diagnostics.
   * @param index Specifies the token position in each input row.
   */
  CSVDataParser(ArrayType& array, const std::string& name, usize index)
  : AbstractDataParser(array, name, index)
  , m_Array(array)
  {
    if constexpr(!std::is_same_v<T, std::string>)
    {
      auto& dataStore = m_Array.getDataStoreRef();
      bool useBufferedPath = !ForceInCoreAlgorithm() && (ForceOocAlgorithm() || dataStore.getStoreType() != IDataStore::StoreType::InMemory);
      if(!useBufferedPath)
      {
        auto* inCoreStore = dynamic_cast<DataStore<T>*>(&dataStore);
        if(inCoreStore != nullptr)
        {
          m_InCoreData = inCoreStore->data();
        }
        else
        {
          useBufferedPath = true;
        }
      }

      const bool usesOutOfCoreStore = dataStore.getStoreType() == IDataStore::StoreType::OutOfCore;
      RecordAlgorithmPathExecution(useBufferedPath ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
      if(useBufferedPath)
      {
        m_Buffer = std::make_unique<T[]>(k_BufferCapacity);
      }
    }
  }
  ~CSVDataParser() override = default;

  CSVDataParser(const CSVDataParser&) = delete;
  CSVDataParser(CSVDataParser&&) = delete;
  CSVDataParser& operator=(const CSVDataParser&) = delete;
  CSVDataParser& operator=(CSVDataParser&&) = delete;

  Result<> parse(const std::string& token, usize index, bool& flushRequired) override
  {
    if constexpr(std::is_same_v<T, std::string>)
    {
      // Remove leading and trailing single or double quote characters.
      const std::regex re(R"(^['"]+|['"]+$)");
      m_Array[index] = std::regex_replace(token, re, "");
    }
    else
    {
      Result<T> parseResult = StringInterpretationUtilities::Convert<T>(token);
      if(parseResult.invalid())
      {
        return ConvertResult(std::move(parseResult));
      }

      if(m_InCoreData != nullptr)
      {
        m_InCoreData[index] = parseResult.value();
        return {};
      }

      if(m_BufferSize == 0)
      {
        m_BufferStartIndex = index;
      }
      m_Buffer[m_BufferSize] = parseResult.value();
      m_BufferSize++;
      flushRequired |= m_BufferSize == k_BufferCapacity;
    }

    return {};
  }

  Result<> flush() override
  {
    if constexpr(std::is_same_v<T, std::string>)
    {
      return {};
    }
    else
    {
      if(m_BufferSize == 0)
      {
        return {};
      }

      Result<> result = m_Array.getDataStoreRef().copyFromBuffer(m_BufferStartIndex, nonstd::span<const T>(m_Buffer.get(), m_BufferSize));
      if(result.valid())
      {
        m_BufferSize = 0;
      }
      return result;
    }
  }

private:
  static constexpr usize k_BufferCapacity = 65'536;

  ArrayType& m_Array;
  T* m_InCoreData = nullptr;
  std::unique_ptr<T[]> m_Buffer;
  usize m_BufferStartIndex = 0;
  usize m_BufferSize = 0;
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

using StringParser = CSVDataParser<StringArray, std::string>;

using ParsersVector = std::vector<std::unique_ptr<AbstractDataParser>>;

/**
 * @brief Creates one typed parser for each non-skipped CSV column.
 * @param csvTypes Specifies the destination type for each column.
 * @param skippedArrays Selects columns that have no parser.
 * @param parentPath Identifies the group that owns destination arrays.
 * @param headers Supplies destination array names and diagnostic labels.
 * @param dataStructure Supplies the existing destination arrays.
 * @return Parser vector aligned with the CSV columns, or an invalid-type error.
 * @pre csvTypes, skippedArrays, and headers have equal sizes. Each non-skipped
 * destination exists and has the requested concrete array type.
 */
SIMPLNX_EXPORT Result<ParsersVector> CreateParsers(const std::vector<CSVType>& csvTypes, const std::vector<bool>& skippedArrays, const DataPath& parentPath, const std::vector<std::string>& headers,
                                                   DataStructure& dataStructure);

/**
 * @brief Flushes all non-skipped CSV parsers.
 * @param dataParsers Supplies parsers and null skipped-column entries.
 * @return Merged bulk-write results with column names added to errors.
 *
 * The function attempts every parser so one failure does not hide later failures.
 */
SIMPLNX_EXPORT Result<> FlushParsers(const ParsersVector& dataParsers);

/**
 * @brief Parses one CSV line into aligned destination parsers.
 * @param inStream Supplies the open input stream at the next data line.
 * @param dataParsers Supplies one parser or null entry for each column.
 * @param headers Supplies column names for diagnostics.
 * @param delimiters Specifies accepted delimiters.
 * @param consecutiveDelimiters Selects whether adjacent delimiters form empty tokens.
 * @param lineNumber Specifies the current one-based file line.
 * @param beginIndex Specifies the one-based line that maps to destination index zero.
 * @param flushRequired Receives true when one or more bounded parser buffers become full.
 * @return Parse result for the line.
 * @pre dataParsers and headers cover every token. lineNumber is not less than beginIndex.
 */
SIMPLNX_EXPORT Result<> ParseLine(std::fstream& inStream, const ParsersVector& dataParsers, const std::vector<std::string>& headers, const std::vector<char>& delimiters, bool consecutiveDelimiters,
                                  usize lineNumber, usize beginIndex, bool& flushRequired);

/**
 * @brief Formats tuple dimensions with an x separator.
 * @param tupleDims Supplies dimensions in stored order.
 * @return Dimension text such as 10x20x30. An empty shape returns an empty string.
 */
SIMPLNX_EXPORT std::string TupleDimsToString(const ShapeType& tupleDims);

/**
 * @brief Replaces path-reserved characters in CSV array names.
 * @param headers Supplies names to modify in place.
 * @return Copy of the modified names.
 *
 * Ampersand, colon, and path separators become underscores. Quote characters are removed.
 */
SIMPLNX_EXPORT std::vector<std::string> RemoveIllegalCharacters(std::vector<std::string>& headers);

/**
 * @brief Positions a stream at a selected one-based line.
 * @param inStream Supplies an open input stream at line one.
 * @param numberOfLines Specifies the one-based target line.
 * @return False if end of file occurs before the target line.
 * @pre numberOfLines is at least one.
 */
SIMPLNX_EXPORT bool SkipNumberOfLines(std::fstream& inStream, usize numberOfLines);

/**
 * @brief Reads and splits one selected CSV header line.
 * @param inputFilePath Identifies the CSV file.
 * @param headersLineNum Specifies the one-based header line.
 * @param delimiters Specifies accepted delimiters.
 * @param consecutiveDelimiters Selects whether adjacent delimiters form empty tokens.
 * @return Header tokens, or a file-open or line-position error.
 * @pre headersLineNum is at least one.
 */
SIMPLNX_EXPORT Result<std::vector<std::string>> ReadHeaders(const std::string& inputFilePath, usize headersLineNum, const std::vector<char>& delimiters, bool consecutiveDelimiters);
} // namespace CSV
} // namespace nx::core::FileUtilities
