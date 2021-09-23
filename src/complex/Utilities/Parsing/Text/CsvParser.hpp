
#pragma once

#include "complex/Utilities/StringUtilities.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <fstream>


namespace complex
{
namespace CsvParser
{

std::vector<float> ParseVertices(const std::string& inputFile, const std::string& delimiter, bool headerLine)
{
  ;
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
}
}
