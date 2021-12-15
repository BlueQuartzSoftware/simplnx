#pragma once

#include "complex/Utilities/StringUtilities.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;



namespace complex
{
namespace CsvParser
{

constexpr int32_t k_RBR_NO_ERROR = 0;
constexpr int32_t k_RBR_FILE_NOT_OPEN = -1000;
constexpr int32_t k_RBR_FILE_TOO_SMALL = -1010;
constexpr int32_t k_RBR_FILE_TOO_BIG = -1020;
constexpr int32_t k_RBR_READ_EOF = -1030;
constexpr int32_t k_RBR_READ_ERROR = 1040;
constexpr int32_t k_RBR_FILE_NOT_EXIST = 1050;

constexpr size_t k_BufferSize = 1024;

// -----------------------------------------------------------------------------
int32_t readLine(std::istream& in, char* result, size_t length)
{
  in.getline(result, length);
  if(in.fail())
  {
    if(in.eof())
    {
      return 0;
    }
    if(in.gcount() == static_cast<std::streamsize>(length))
    {
      // Read kBufferSize chars; ignoring the rest of the line.
      in.clear();
      in.ignore(std::numeric_limits<int>::max(), '\n');
    }
  }
  return 1;
}

/**
 * @brief Returns the number of lines in a text file.
 * @param inputPath The file to check
 * @return
 */
uint64 LineCount(const fs::path& inputPath)
{
  uint64 lineCount = 0;
  {
    std::string buf;

    std::ifstream in(inputPath, std::ios_base::binary);

    while(!in.eof())
    {
      std::getline(in, buf);
      lineCount++;
    }
    // Put the input stream back to the start
    in.clear();                 // clear fail and eof bits
    in.seekg(0, std::ios::beg); // back to the start!
  }
  return lineCount - 1;
}

class DelimiterType : public std::ctype<char>
{
  mask my_table[table_size];

public:
  DelimiterType(char delimiter, usize refs = 0)
  : std::ctype<char>(&my_table[0], false, refs)
  {
    std::copy_n(classic_table(), table_size, my_table);
    my_table[static_cast<mask>(delimiter)] = (mask)space;
  }
};

// -----------------------------------------------------------------------------
int check_error_bits(std::ifstream* f)
{
  int stop = k_RBR_NO_ERROR;
  if(f->eof())
  {
    // std::perror("stream eofbit. error state");
    stop = k_RBR_READ_EOF;
  }
  else if(f->fail())
  {
    // std::perror("stream failbit (or badbit). error state");
    stop = k_RBR_READ_ERROR;
  }
  else if(f->bad())
  {
    // std::perror("stream badbit. error state");
    stop = k_RBR_READ_ERROR;
  }
  return stop;
}

/**
 *
 * @tparam T
 * @tparam K
 * @param filename
 * @param data
 * @param skipHeaderLines
 * @param delimiter
 * @param inputIsBool
 * @return
 */
template <typename T, typename K>
Result<> ReadFile(const fs::path& filename, DataArray<T>& data, uint64 skipHeaderLines, char delimiter, bool inputIsBool = false)
{

  int32_t err = 0;
  fs::exists(filename);
  if(err < 0)
  {
    return MakeErrorResult(k_RBR_FILE_NOT_EXIST, fmt::format("Input file does not exist: {}", filename.string()));
  }

  std::ifstream in(filename.c_str(), std::ios_base::in | std::ios_base::binary);
  if(!in.is_open())
  {
    return MakeErrorResult(k_RBR_FILE_NOT_OPEN, fmt::format("Could not open file for reading: {}", filename.string()));
  }

  in.imbue(std::locale(std::locale(), new complex::CsvParser::DelimiterType(delimiter)));

  std::array<char, k_BufferSize> buf;
  char* buffer = buf.data();

  // Skip some header bytes by just reading those bytes into the pointer knowing that the next
  // thing we are going to do it over write those bytes with the real data that we are after.
  for(int i = 0; i < skipHeaderLines; i++)
  {
    buf.fill(0x00);                                               // Splat Null Chars across the line
    err = complex::CsvParser::readLine(in, buffer, k_BufferSize); // Read Line 1 - VTK Version Info
    if(err < 0)
    {
      return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Could not read data from file while skipping header lines: {}", filename.string()));
    }
  }

  size_t numTuples = data.getNumberOfTuples();
  int scalarNumComp = data.getNumberOfComponents();

  size_t totalSize = numTuples * static_cast<size_t>(scalarNumComp);
  err = k_RBR_NO_ERROR;
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
      err = check_error_bits(&in);
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
      err = check_error_bits(&in);
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

template <class T>
DataArray<T>* ArrayFromPath(DataStructure& data, const DataPath& path)
{
  using DataArrayType = DataArray<T>;
  DataObject* object = data.getData(path);
  DataArrayType* dataArray = dynamic_cast<DataArrayType*>(object);
  if(dataArray == nullptr)
  {
    throw std::runtime_error("Can't obtain DataArray");
  }
  return dataArray;
}

char IndexToDelimiter(uint64 index)
{
  switch(index)
  {
  case 0:
    return ',';
  case 1:
    return ';';
  case 2:
    return ' ';
  case 3:
    return ':';
  case 4:
    return '\t';
  default:
    throw std::runtime_error("Invalid index");
  }
}

std::vector<float> ParseVertices(const std::string& inputFile, const std::string& delimiter, bool headerLine)
{
  std::fstream in(inputFile, std::ios_base::in);
  if(!in.is_open())
  {
    std::cout << "Could not open input file: " << inputFile << std::endl;
    return {};
  }

  std::vector<float> data;
  char delim = delimiter.at(0);
  std::string buf;
  // Scan the file to figure out about how many values will be in the file
  size_t lineCount = 1;
  if(headerLine)
  {
    std::getline(in, buf);
  }
  while(!in.eof())
  {
    std::getline(in, buf);
    lineCount++;
  }
  // Put the input stream back to the start
  in.clear();                 // clear fail and eof bits
  in.seekg(0, std::ios::beg); // back to the start!
  if(headerLine)
  {
    std::getline(in, buf);
  }
  data.reserve(lineCount * 3); // Just reserve the worst case possible.
  while(!in.eof())
  {
    std::getline(in, buf);
    if(buf.empty())
    {
      continue;
    }
    std::vector<std::string> tokens = complex::StringUtilities::split(buf, delim);
    float value = std::atof(tokens[0].c_str());
    data.push_back(value);
    value = std::atof(tokens[1].c_str());
    data.push_back(value);
    value = std::atof(tokens[2].c_str());
    data.push_back(value);
  }
  in.close();

  return data;
}
} // namespace CsvParser
} // namespace complex
