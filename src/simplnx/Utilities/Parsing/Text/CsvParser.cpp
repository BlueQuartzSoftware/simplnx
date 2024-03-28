#include "CsvParser.hpp"

namespace nx::core
{
namespace CsvParser
{

int32_t ReadLine(std::istream& in, char* result, size_t length)
{
  in.getline(result, static_cast<std::streamsize>(length));

  if(in.fail())
  {
    if(in.eof())
    {
      return -1;
    }
    if(in.gcount() == static_cast<std::streamsize>(length))
    {
      // Read kBufferSize chars; ignoring the rest of the line.
      in.clear();
      in.ignore(std::numeric_limits<int>::max(), '\n');
    }
  }
  return static_cast<int32>(in.gcount());
}

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

int CheckErrorBits(std::ifstream* f)
{
  int stop = k_RBR_NO_ERROR;
  if(f->eof())
  {
    stop = k_RBR_READ_EOF;
  }
  else if(f->fail())
  {
    stop = k_RBR_READ_FAIL;
  }
  else if(f->bad())
  {
    stop = k_RBR_READ_BAD;
  }
  return stop;
}

char IndexToDelimiter(uint64_t index)
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
    std::vector<std::string> tokens = nx::core::StringUtilities::split(buf, delim);
    float value = static_cast<float>(std::atof(tokens[0].c_str()));
    data.push_back(value);
    value = static_cast<float>(std::atof(tokens[1].c_str()));
    data.push_back(value);
    value = static_cast<float>(std::atof(tokens[2].c_str()));
    data.push_back(value);
  }
  in.close();

  return data;
}

} // namespace CsvParser
} // namespace nx::core
