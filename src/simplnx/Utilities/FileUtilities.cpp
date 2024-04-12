#include "FileUtilities.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#define FSPP_ACCESS_FUNC_NAME _access
#else
#include <unistd.h>
#define FSPP_ACCESS_FUNC_NAME access
#endif

namespace fs = std::filesystem;

namespace
{
#ifdef _WIN32
constexpr int k_CheckWritable = 2;
#else
constexpr int k_CheckWritable = W_OK;
#endif

constexpr int k_HasAccess = 0;
}; // namespace

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
  auto bytsRead = fread(last, 1, 1, fd);
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
    return MakeErrorResult(-300, fmt::format("CSV file does not exist: {}", absPath.string()));
  }
  if(fs::is_directory({absPath}))
  {
    return MakeErrorResult(-301, fmt::format("CSV input file is a directory: {}", absPath.string()));
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

//-----------------------------------------------------------------------------
bool HasWriteAccess(const std::string& path)
{
  return FSPP_ACCESS_FUNC_NAME(path.c_str(), k_CheckWritable) == k_HasAccess;
}

//-----------------------------------------------------------------------------
Result<> ValidateDirectoryWritePermission(const fs::path& path, bool isFile)
{
  if(path.empty())
  {
    return MakeErrorResult(-16, "ValidateDirectoryWritePermission() Error: Input path empty.");
  }

  auto checkedPath = path;
  if(isFile)
  {
    checkedPath = checkedPath.parent_path();
  }
  // We now have the parent directory. Let us see if *any* part of the path exists

  // If the path is relative, then make it absolute
  if(!checkedPath.is_absolute())
  {
    try
    {
      checkedPath = fs::absolute(checkedPath);
    } catch(const std::filesystem::filesystem_error& error)
    {
      return MakeErrorResult(-15, fmt::format("ValidateDirectoryWritePermission() Error: Input Path '{}' was relative and trying to create an absolute path threw an exception with message '{}'. "
                                              "Further error code and message from the file system was: Code={} Message={}",
                                              path.string(), error.what(), error.code().value(), error.code().message()));
    }
  }

  auto rootPath = checkedPath.root_path();

  // The idea here is to start walking up from the deepest directory and hopefully
  // find an existing directory. If we get to the top if the path and we are still
  // empty then:
  //  On unix based systems not sure if it would happen. Even if the user set a path
  // to another drive that didn't exist, at some point you hit the '/' and then you
  // can try to create the directories.
  //  On Windows the user put in a bogus drive letter which is just a hard failure
  // because we can't make up a new drive letter.
  while(!fs::exists(checkedPath) && checkedPath != rootPath)
  {
    checkedPath = checkedPath.parent_path();
  }

  if(checkedPath.empty())
  {
    return MakeErrorResult(-19, fmt::format("ValidateDirectoryWritePermission() Error: Input path '{}' resolved to an empty path", path.string()));
  }

  if(!fs::exists(checkedPath))
  {
    return MakeErrorResult(-11,
                           fmt::format("ValidateDirectoryWritePermission() Error: Input Path '{}' resolved to '{}'. The drive does not exist on this system.", path.string(), checkedPath.string()));
  }

  // We should be at the top of the tree with an existing directory.
  if(HasWriteAccess(checkedPath.string()))
  {
    return {};
  }
  return MakeErrorResult(-8, fmt::format("ValidateDirectoryWritePermission() Error: User does not have write permissions to path '{}'", path.string()));
}
} // namespace nx::core::FileUtilities
