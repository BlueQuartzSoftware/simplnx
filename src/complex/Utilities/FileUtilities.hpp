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
#pragma once

#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <string>

#include "complex/Common/Result.hpp"

namespace fs = std::filesystem;

namespace complex
{
namespace FileUtilities
{
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
} // namespace FileUtilities
} // namespace complex
