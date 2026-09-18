#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nx::core::CsvParser
{

constexpr int32_t k_RBR_NO_ERROR = 0;
constexpr int32_t k_RBR_FILE_NOT_OPEN = -1000;
constexpr int32_t k_RBR_READ_EOF = -1030;
constexpr int32_t k_RBR_READ_FAIL = -1035;
constexpr int32_t k_RBR_READ_ERROR = 1040;
constexpr int32_t k_RBR_READ_BAD = 1045;
constexpr int32_t k_RBR_FILE_NOT_EXIST = 1050;

constexpr size_t k_BufferSize = 1024;

/**
 * @class DelimiterType
 * @brief Treats one selected character as whitespace for formatted extraction.
 *
 * A std::locale owns this facet when refs is zero. The facet is not modified
 * after construction.
 */
class DelimiterType : public std::ctype<char>
{
  std::ctype<char>::mask my_table[std::ctype<char>::table_size] = {};

public:
  /**
   * @brief Creates a character-classification table for one delimiter.
   * @param delimiter Specifies an ASCII delimiter.
   * @param refs Specifies the standard facet reference count.
   * @pre delimiter has a nonnegative unsigned-character index.
   */
  explicit DelimiterType(char delimiter, size_t refs = 0)
  : std::ctype<char>(&my_table[0], false, refs)
  {
    std::copy_n(std::ctype<char>::classic_table(), table_size, my_table);
    my_table[static_cast<std::ctype<char>::mask>(delimiter)] = (std::ctype<char>::mask)space;
  }
};

/**
 * @brief Converts a GUI delimiter index to one character.
 * @param index Specifies a value from zero through five.
 * @return Comma, semicolon, space, colon, tab, or newline.
 * @throws std::runtime_error If index is greater than five.
 */
SIMPLNX_EXPORT char IndexToDelimiter(uint64_t index);

/**
 * @brief Converts input-stream state flags to a parser code.
 * @param fileStream Supplies a non-null input stream.
 * @return EOF, fail, bad, or no-error code. EOF has precedence over fail.
 */
SIMPLNX_EXPORT int CheckErrorBits(std::ifstream* fileStream);

/**
 * @brief Returns the number of lines in a text file.
 * @param inputPath Identifies the readable text file.
 * @return Number of lines.
 * @pre The file opens successfully.
 */
SIMPLNX_EXPORT uint64_t LineCount(const std::filesystem::path& inputPath);

/**
 * @brief Reads one bounded line and discards characters beyond the buffer.
 * @param in Supplies the input stream.
 * @param buffer Receives a null-terminated line prefix.
 * @param length Specifies buffer capacity.
 * @return Character count, or -1 when end of file prevents a read.
 * @pre buffer contains length writable characters. length fits std::streamsize.
 */
SIMPLNX_EXPORT int32_t ReadLine(std::istream& in, char* buffer, size_t length);

/**
 * @brief Reads formatted numeric values into one data store.
 * @tparam T Specifies the destination value type.
 * @tparam K Specifies the formatted-extraction value type.
 * @param filename Identifies the text file.
 * @param data Receives flat tuple-component values.
 * @param skipHeaderLines Specifies the number of leading lines to discard.
 * @param delimiter Specifies a character that formatted extraction treats as whitespace.
 * @param inputIsBool True to convert extracted numeric zero to false and other values to true.
 * @return Early-EOF or destination-store errors.
 * @pre skipHeaderLines fits int. Destination size arithmetic fits size_t.
 * @pre K values can convert to T under the caller's numeric conversion contract.
 *
 * A one-MiB target bounds the staging page. Extra input values are ignored.
 * Formatted-extraction fail states other than early EOF are not currently returned.
 */
template <typename T, typename K>
Result<> ReadFile(const std::filesystem::path& filename, AbstractDataStore<T>& data, uint64_t skipHeaderLines, char delimiter, bool inputIsBool = false)
{
  int32 err;
  if(!std::filesystem::exists(filename))
  {
    return MakeErrorResult(k_RBR_FILE_NOT_EXIST, fmt::format("Input file does not exist: {}", filename.string()));
  }

  std::ifstream in(filename.c_str(), std::ios_base::in | std::ios_base::binary);
  if(!in.is_open())
  {
    return MakeErrorResult(k_RBR_FILE_NOT_OPEN, fmt::format("Could not open file for reading: {}", filename.string()));
  }

  in.imbue(std::locale(std::locale(), new nx::core::CsvParser::DelimiterType(delimiter)));

  std::array<char, k_BufferSize> buf = {};
  char* buffer = buf.data();

  // Discard each bounded header line. ReadLine discards any suffix beyond its buffer.
  for(int i = 0; i < skipHeaderLines; i++)
  {
    buf.fill(0x00);
    err = nx::core::CsvParser::ReadLine(in, buffer, k_BufferSize);
    if(err < 0)
    {
      return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Could not read data from file while skipping header lines: {}", filename.string()));
    }
  }

  size_t numTuples = data.getNumberOfTuples();
  int scalarNumComp = data.getNumberOfComponents();

  size_t totalSize = numTuples * static_cast<size_t>(scalarNumComp);
  constexpr size_t k_TargetBufferBytes = 1024 * 1024;
  const size_t bufferSize = std::max<size_t>(1, std::min(totalSize, k_TargetBufferBytes / sizeof(T)));
  auto valueBuffer = std::make_unique<T[]>(bufferSize);
  size_t bufferedValues = 0;
  size_t outputOffset = 0;

  const auto flushBuffer = [&]() -> Result<> {
    if(bufferedValues == 0)
    {
      return {};
    }
    Result<> result = data.copyFromBuffer(outputOffset, nonstd::span<const T>(valueBuffer.get(), bufferedValues));
    if(result.valid())
    {
      outputOffset += bufferedValues;
      bufferedValues = 0;
    }
    return result;
  };

  if(inputIsBool)
  {
    double value = 0.0;
    auto* si64Ptr = reinterpret_cast<int64_t*>(&value);
    for(size_t i = 0; i < totalSize; ++i)
    {
      in >> value;
      if(*si64Ptr == 0)
      {
        valueBuffer[bufferedValues] = false;
      }
      else
      {
        valueBuffer[bufferedValues] = true;
      }
      bufferedValues++;
      err = CheckErrorBits(&in);
      if(err == k_RBR_READ_EOF && i < totalSize - 1)
      {
        return MakeErrorResult(k_RBR_READ_EOF, fmt::format("Read past End Of File (EOF) while parsing file: {}", filename.string()));
      }
      if(err == k_RBR_READ_ERROR)
      {
        return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Read error while parsing file: {}", filename.string()));
      }
      if(bufferedValues == bufferSize)
      {
        Result<> result = flushBuffer();
        if(result.invalid())
        {
          return result;
        }
      }
    }
  }
  else
  {
    K value = static_cast<T>(0.0);
    for(size_t i = 0; i < totalSize; ++i)
    {
      in >> value;
      valueBuffer[bufferedValues++] = static_cast<T>(value);
      err = CheckErrorBits(&in);
      if(err == k_RBR_READ_EOF && i < totalSize - 1)
      {
        return MakeErrorResult(k_RBR_READ_EOF, fmt::format("Read past End Of File (EOF) while parsing file: {}", filename.string()));
      }
      if(err == k_RBR_READ_ERROR)
      {
        return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Read error while parsing file: {}", filename.string()));
      }
      if(bufferedValues == bufferSize)
      {
        Result<> result = flushBuffer();
        if(result.invalid())
        {
          return result;
        }
      }
    }
  }

  return flushBuffer();
}

/**
 * @brief Reads string tokens and converts them into one numeric data store.
 * @tparam T Specifies the destination value type.
 * @param filename Identifies the text file.
 * @param data Receives flat tuple-component values.
 * @param skipHeaderLines Specifies the number of leading lines to discard.
 * @param delimiter Specifies a character that formatted extraction treats as whitespace.
 * @return Conversion, early-EOF, or destination-store errors.
 * @pre skipHeaderLines fits int. Destination size arithmetic fits size_t.
 *
 * A one-MiB target bounds the staging page. Extra input values are ignored.
 * Formatted-extraction fail states other than early EOF are not currently returned.
 */
template <typename T>
Result<> ReadFile(const std::filesystem::path& filename, AbstractDataStore<T>& data, uint64_t skipHeaderLines, char delimiter)
{
  int32 err;
  if(!std::filesystem::exists(filename))
  {
    return MakeErrorResult(k_RBR_FILE_NOT_EXIST, fmt::format("Input file does not exist: {}", filename.string()));
  }

  std::ifstream in(filename.c_str(), std::ios_base::in | std::ios_base::binary);
  if(!in.is_open())
  {
    return MakeErrorResult(k_RBR_FILE_NOT_OPEN, fmt::format("Could not open file for reading: {}", filename.string()));
  }

  in.imbue(std::locale(std::locale(), new nx::core::CsvParser::DelimiterType(delimiter)));

  std::array<char, k_BufferSize> buf = {};
  char* buffer = buf.data();

  // Discard each bounded header line. ReadLine discards any suffix beyond its buffer.
  for(int i = 0; i < skipHeaderLines; i++)
  {
    buf.fill(0x00);
    err = nx::core::CsvParser::ReadLine(in, buffer, k_BufferSize);
    if(err < 0)
    {
      return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Could not read data from file while skipping header lines: {}", filename.string()));
    }
  }

  size_t numTuples = data.getNumberOfTuples();
  int scalarNumComp = data.getNumberOfComponents();

  size_t totalSize = numTuples * static_cast<size_t>(scalarNumComp);
  constexpr size_t k_TargetBufferBytes = 1024 * 1024;
  const size_t bufferSize = std::max<size_t>(1, std::min(totalSize, k_TargetBufferBytes / sizeof(T)));
  auto valueBuffer = std::make_unique<T[]>(bufferSize);
  size_t bufferedValues = 0;
  size_t outputOffset = 0;

  std::string value;
  for(size_t i = 0; i < totalSize; ++i)
  {
    in >> value;
    Result<T> parseResult = StringInterpretationUtilities::Convert<T>(value);
    if(parseResult.invalid())
    {
      return ConvertResult(std::move(parseResult));
    }
    valueBuffer[bufferedValues++] = parseResult.value();
    err = CheckErrorBits(&in);
    if(err == k_RBR_READ_EOF && i < totalSize - 1)
    {
      return MakeErrorResult(k_RBR_READ_EOF, fmt::format("Read past End Of File (EOF) while parsing file: {}", filename.string()));
    }
    if(err == k_RBR_READ_ERROR)
    {
      return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Read error while parsing file: {}", filename.string()));
    }
    if(bufferedValues == bufferSize)
    {
      Result<> result = data.copyFromBuffer(outputOffset, nonstd::span<const T>(valueBuffer.get(), bufferedValues));
      if(result.invalid())
      {
        return result;
      }
      outputOffset += bufferedValues;
      bufferedValues = 0;
    }
  }

  if(bufferedValues == 0)
  {
    return {};
  }
  return data.copyFromBuffer(outputOffset, nonstd::span<const T>(valueBuffer.get(), bufferedValues));
}
} // namespace nx::core::CsvParser
