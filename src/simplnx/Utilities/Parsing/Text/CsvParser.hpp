#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace nx::core
{
namespace CsvParser
{

constexpr int32_t k_RBR_NO_ERROR = 0;
constexpr int32_t k_RBR_FILE_NOT_OPEN = -1000;
constexpr int32_t k_RBR_FILE_TOO_SMALL = -1010;
constexpr int32_t k_RBR_FILE_TOO_BIG = -1020;
constexpr int32_t k_RBR_READ_EOF = -1030;
constexpr int32_t k_RBR_READ_FAIL = -1035;
constexpr int32_t k_RBR_READ_ERROR = 1040;
constexpr int32_t k_RBR_READ_BAD = 1045;
constexpr int32_t k_RBR_FILE_NOT_EXIST = 1050;

constexpr size_t k_BufferSize = 1024;

class DelimiterType : public std::ctype<char>
{
  std::ctype<char>::mask my_table[std::ctype<char>::table_size];

public:
  DelimiterType(char delimiter, size_t refs = 0)
  : std::ctype<char>(&my_table[0], false, refs)
  {
    std::copy_n(std::ctype<char>::classic_table(), table_size, my_table);
    my_table[static_cast<std::ctype<char>::mask>(delimiter)] = (std::ctype<char>::mask)space;
  }
};

/**
 * @brief Converts an index that could come from a GUI to the associated delimiter
 * @param index
 * @return The delimiter associated with that delimiter.
 */
SIMPLNX_EXPORT char IndexToDelimiter(uint64_t index);

/**
 * @brief Checks the error bits of the input stream
 * @param fileStream file stream
 * @return Integer error code
 */
SIMPLNX_EXPORT int CheckErrorBits(std::ifstream* fileStream);

/**
 * @brief Returns the number of lines in a text file.
 * @param inputPath The file to check
 * @return
 */
SIMPLNX_EXPORT uint64_t LineCount(const fs::path& inputPath);

/**
 * @brief Reads a line from the input stream and returns the result of that operations, Data is read into the buffer
 * @param in The input file stream
 * @param buffer The buffer to store the bytes into
 * @param length Max number of bytes to read from the file
 * @return
 */
SIMPLNX_EXPORT int32_t ReadLine(std::istream& in, char* buffer, size_t length);

/**
 * @brief Reads a Text file that contains numeric values into a single DataArray<T>.
 * @tparam T Final Target type of the value being read
 * @tparam K Intermediate type to be used to initially read the value from the file
 * @param filename The input path to the text file
 * @param data The Target DataArray<T>
 * @param skipHeaderLines Number of "header lines" that should be skipped before parsing begins
 * @param delimiter The delimiter to use: Comma, Space, Tab
 * @param inputIsBool Are the values being read Booleans
 * @return Result<> with any errors or warnings that were encountered.
 */
template <typename T, typename K>
Result<> ReadFile(const fs::path& filename, DataArray<T>& data, uint64_t skipHeaderLines, char delimiter, bool inputIsBool = false)
{
  int32_t err = k_RBR_NO_ERROR;
  if(!fs::exists(filename))
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

  // Skip some header bytes by just reading those bytes into the pointer knowing that the next
  // thing we are going to do it over write those bytes with the real data that we are after.
  for(int i = 0; i < skipHeaderLines; i++)
  {
    buf.fill(0x00);                                                // Splat Null Chars across the line
    err = nx::core::CsvParser::ReadLine(in, buffer, k_BufferSize); // Read Line 1 - VTK Version Info
    if(err < 0)
    {
      return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Could not read data from file while skipping header lines: {}", filename.string()));
    }
  }

  size_t numTuples = data.getNumberOfTuples();
  int scalarNumComp = data.getNumberOfComponents();

  size_t totalSize = numTuples * static_cast<size_t>(scalarNumComp);

  if(inputIsBool)
  {
    double value = 0.0;
    int64_t* si64Ptr = reinterpret_cast<int64_t*>(&value);
    for(size_t i = 0; i < totalSize; ++i)
    {
      in >> value;
      if(*si64Ptr == 0)
      {
        data[i] = false;
      }
      else
      {
        data[i] = true;
      }
      err = CheckErrorBits(&in);
      if(err == k_RBR_READ_EOF && i < totalSize - 1)
      {
        return MakeErrorResult(k_RBR_READ_EOF, fmt::format("Read past End Of File (EOF) while parsing file: {}", filename.string()));
      }
      if(err == k_RBR_READ_ERROR)
      {
        return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Read error while parsing file: {}", filename.string()));
      }
    }
  }
  else
  {
    K value = static_cast<T>(0.0);
    for(size_t i = 0; i < totalSize; ++i)
    {
      in >> value;
      data[i] = static_cast<T>(value);
      err = CheckErrorBits(&in);
      if(err == k_RBR_READ_EOF && i < totalSize - 1)
      {
        return MakeErrorResult(k_RBR_READ_EOF, fmt::format("Read past End Of File (EOF) while parsing file: {}", filename.string()));
      }
      if(err == k_RBR_READ_ERROR)
      {
        return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Read error while parsing file: {}", filename.string()));
      }
    }
  }

  return {};
}

/**
 * @brief Reads a Text file that contains numeric values into a single DataArray<T> using an intermediate string token and checks for valid conversion to the templated type T.
 * @tparam T Final Target type of the value being read
 * @param filename The input path to the text file
 * @param data The Target DataArray<T>
 * @param skipHeaderLines Number of "header lines" that should be skipped before parsing begins
 * @param delimiter The delimiter to use: Comma, Space, Tab
 * @return Result<> with any errors or warnings that were encountered.
 */
template <typename T>
Result<> ReadFile(const fs::path& filename, DataArray<T>& data, uint64_t skipHeaderLines, char delimiter)
{
  int32_t err = k_RBR_NO_ERROR;
  if(!fs::exists(filename))
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

  // Skip some header bytes by just reading those bytes into the pointer knowing that the next
  // thing we are going to do it over write those bytes with the real data that we are after.
  for(int i = 0; i < skipHeaderLines; i++)
  {
    buf.fill(0x00);                                                // Splat Null Chars across the line
    err = nx::core::CsvParser::ReadLine(in, buffer, k_BufferSize); // Read Line 1 - VTK Version Info
    if(err < 0)
    {
      return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Could not read data from file while skipping header lines: {}", filename.string()));
    }
  }

  size_t numTuples = data.getNumberOfTuples();
  int scalarNumComp = data.getNumberOfComponents();

  size_t totalSize = numTuples * static_cast<size_t>(scalarNumComp);

  std::string value;
  for(size_t i = 0; i < totalSize; ++i)
  {
    in >> value;
    Result<T> parseResult = ConvertTo<T>::convert(value);
    if(parseResult.invalid())
    {
      return ConvertResult(std::move(parseResult));
    }
    data[i] = parseResult.value();
    err = CheckErrorBits(&in);
    if(err == k_RBR_READ_EOF && i < totalSize - 1)
    {
      return MakeErrorResult(k_RBR_READ_EOF, fmt::format("Read past End Of File (EOF) while parsing file: {}", filename.string()));
    }
    if(err == k_RBR_READ_ERROR)
    {
      return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Read error while parsing file: {}", filename.string()));
    }
  }

  return {};
}

SIMPLNX_EXPORT std::vector<float> ParseVertices(const std::string& inputFile, const std::string& delimiter, bool headerLine);

} // namespace CsvParser
} // namespace nx::core
