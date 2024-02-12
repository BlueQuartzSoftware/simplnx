#include "FileUtilities.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace nx::core::FileUtilities
{

int64 LinesInFile(const std::string& filepath)
{
  const usize BUFFER_SIZE = 16384;
  usize lines = 0;
  usize bytes = 0;

  FILE* fd = fopen(filepath.c_str(), "rb");
  if(nullptr == fd)
  {
    return -1;
  }

  // Check if the very last character is NOT a newline character
  fseek(fd, -1, SEEK_END);
  char last[1];
  fread(last, 1, 1, fd);
  if(last[0] != '\n')
  {
    lines++;
  }
  rewind(fd);

  // Read through the rest of the file
  char buf[BUFFER_SIZE + 1];
  while(true)
  {
    memset(buf, 0, BUFFER_SIZE + 1);
    size_t bytes_read = fread(buf, 1, BUFFER_SIZE, fd);
    if(bytes_read == 0)
    {
      break;
    }

    bytes += bytes_read;
    char* end = buf + bytes_read;
    usize buflines = 0;

    for(char* p = buf; p < end; p++)
    {
      buflines += *p == '\n';
    }
    lines += buflines;
  }
  fclose(fd);
  return lines;
}

Result<> ValidateCSVFile(const std::string& filePath)
{
  constexpr int64_t bufferSize = 2048;

  auto absPath = fs::absolute(filePath);

  if(!fs::exists({absPath}))
  {
    return MakeErrorResult(-300, fmt::format("File does not exist: {}", absPath.string()));
  }

  // Obtain the file size
  const size_t fileSize = fs::file_size(absPath);

  // Open the file
  std::ifstream in(absPath.c_str(), std::ios_base::binary);
  if(!in.is_open())
  {
    return MakeErrorResult(-301, fmt::format("Could not open file for reading: {}", absPath.string()));
  }

  size_t actualSize = bufferSize;
  if(fileSize <= bufferSize)
  {
    actualSize = fileSize;
  }

  // Allocate the buffer
  std::vector<char> buffer(actualSize, 0);

  // Copy the file contents into the buffer
  try
  {
    in.read(buffer.data(), actualSize);
  } catch(const std::exception& e)
  {
    return MakeErrorResult(-302, fmt::format("There was an error reading the data from file: {}.  Exception: {}", absPath.string(), e.what()));
  }

  // Check the buffer for invalid characters, tab characters, new-line characters, and carriage return characters
  bool hasNewLines = false;
  bool hasCarriageReturns = false;
  bool hasTabs = false;
  // If the first line of the file is > 2048 then this will fail! (MJ)
  for(size_t i = 0; i < actualSize; i++)
  {
    const char currentChar = buffer[i];

    if(currentChar < 32 && currentChar != 9 && currentChar != 10 && currentChar != 13)
    {
      // This is an unprintable character
      return MakeErrorResult(-303, fmt::format("Unprintable characters have been detected in file: {}.  Please import a different file.", absPath.string()));
    }
    if(currentChar == 9)
    {
      hasTabs = true;
    }
    else if(currentChar == 10)
    {
      hasNewLines = true;
    }
    else if(currentChar == 13)
    {
      hasCarriageReturns = true;
    }
  }

  if(!hasNewLines && !hasCarriageReturns && !hasTabs)
  {
    // This might be a binary file
    return MakeErrorResult(-304, fmt::format("The file \"{}\" might be a binary file, because line-feed, tab, or carriage return characters have not been detected. Using this file may crash the "
                                             "program or cause unexpected results.  Please import a different file.",
                                             absPath.string()));
  }

  return {};
}

} // namespace nx::core::FileUtilities
