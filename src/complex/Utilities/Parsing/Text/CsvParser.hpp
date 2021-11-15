

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

template <class T>
Result<> ReadFile(const fs::path& inputPath, DataArray<T>& data, uint64 skipLines, char delimiter)
{

  std::ifstream inputFile(inputPath, std::ios_base::binary);

  for(uint64 i = 0; i < skipLines; i++)
  {
    inputFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  inputFile.imbue(std::locale(std::locale(), new DelimiterType(delimiter)));

  usize numTuples = data.getNumberOfTuples();
  usize scalarNumComp = data.getNumberOfComponents();

  usize totalSize = numTuples * scalarNumComp;
  T value = {};
  for(usize i = 0; i < totalSize; i++)
  {
    inputFile >> value;
    if(inputFile.fail())
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-1, "Unable to convert value"}})};
    }
    data[i] = value;
  }

  Result<> result;
  result.warnings().push_back(Warning{-1, "Test Warning"});

  return {};
}

template <class T>
DataArray<T>& ArrayFromPath(DataStructure& data, const DataPath& path)
{
  DataObject* object = data.getData(path);
  DataArray<T>* dataArray = dynamic_cast<DataArray<T>*>(object);
  if(dataArray == nullptr)
  {
    throw std::runtime_error("Can't obtain DataArray");
  }
  return *dataArray;
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
