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
#include "FileUtilities.hpp"

#include <fmt/format.h>

#include <cctype>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <io.h>
#define FSPP_ACCESS_FUNC_NAME _access
#else
#include <unistd.h>
#define FSPP_ACCESS_FUNC_NAME access
#endif

using namespace nx::core;

#ifdef _WIN32
constexpr int k_CheckWritable = 2;
#else
constexpr int k_CheckWritable = W_OK;
#endif

constexpr int k_HasAccess = 0;

//-----------------------------------------------------------------------------
bool FileUtilities::HasWriteAccess(const std::string& path)
{
  return FSPP_ACCESS_FUNC_NAME(path.c_str(), k_CheckWritable) == k_HasAccess;
}

//-----------------------------------------------------------------------------
Result<> FileUtilities::ValidateInputFile(const fs::path& path)
{
  if(!fs::exists(path))
  {
    return MakeErrorResult(-2, fmt::format("File System Path '{}' does not exist", path.string()));
  }

  if(!fs::is_regular_file(path))
  {
    return MakeErrorResult(-3, fmt::format("File System Path '{}' is not a file", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> FileUtilities::ValidateInputDir(const fs::path& path)
{
  if(!fs::exists(path))
  {
    return MakeErrorResult(-4, fmt::format("File System Path '{}' does not exist", path.string()));
  }
  if(!fs::is_directory(path))
  {
    return MakeErrorResult(-5, fmt::format("File System Path '{}' is not a file", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> FileUtilities::ValidateDirectoryWritePermission(const fs::path& path, bool isFile)
{
  if(path.empty())
  {
    return MakeErrorResult(-16, "ValidateDirectoryWritePermission() error: given path was empty.");
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
      return MakeErrorResult(-15, fmt::format("ValidateDirectoryWritePermission() threw an error: '{}'", error.what()));
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
    return MakeErrorResult(-19, "ValidateDirectoryWritePermission() resolved path was empty");
  }

  if(!fs::exists(checkedPath))
  {
    return MakeErrorResult(-11, fmt::format("ValidateDirectoryWritePermission() error: The drive does not exist on this system: '{}'", checkedPath.string()));
  }

  // We should be at the top of the tree with an existing directory.
  if(HasWriteAccess(checkedPath.string()))
  {
    return {};
  }
  return MakeErrorResult(-8, fmt::format("User does not have write permissions to path '{}'", path.string()));
}

//-----------------------------------------------------------------------------
Result<> FileUtilities::ValidateOutputFile(const fs::path& path)
{
  auto result = ValidateDirectoryWritePermission(path, true);
  if(result.invalid())
  {
    return result;
  }
  if(!fs::exists(path))
  {
    return MakeWarningVoidResult(-6, fmt::format("File System Path '{}' does not exist. It will be created during execution.", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> FileUtilities::ValidateOutputDir(const fs::path& path)
{
  auto result = ValidateDirectoryWritePermission(path, false);
  if(result.invalid())
  {
    return result;
  }
  if(!fs::exists(path))
  {
    return MakeWarningVoidResult(-7, fmt::format("File System Path '{}' does not exist. It will be created during execution.", path.string()));
  }
  return {};
}

//-----------------------------------------------------------------------------
Result<> FileUtilities::ValidateCSVFile(const std::string& filePath)
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
