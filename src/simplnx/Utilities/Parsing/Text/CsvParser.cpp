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
} // namespace CsvParser
} // namespace nx::core
