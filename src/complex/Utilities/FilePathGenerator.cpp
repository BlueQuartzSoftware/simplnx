#include "FilePathGenerator.hpp"

#include <filesystem>
#include <string>

#include <fmt/core.h>

namespace fs = std::filesystem;

namespace complex
{
namespace FilePathGenerator
{
// -----------------------------------------------------------------------------
std::pair<std::vector<std::string>, bool> GenerateAndValidateFileList(int32 start, int32 end, int32 increment, Ordering order, std::string_view inputPath, std::string_view filePrefix,
                                                                      std::string_view fileSuffix, std::string_view fileExtension, int32 paddingDigits, bool failFast)
{
  std::vector<std::string> fileList = GenerateFileList(start, end, increment, order, inputPath, filePrefix, fileSuffix, fileExtension, paddingDigits);
  bool missingFiles = false;
  for(const auto& file : fileList)
  {
    if(!std::filesystem::exists(file))
    {
      missingFiles = true;
      if(failFast)
      {
        break;
      }
    }
  }
  return std::make_pair(std::move(fileList), missingFiles);
}

// -----------------------------------------------------------------------------
std::vector<std::string> GenerateFileList(int32 start, int32 end, int32 increment, Ordering order, std::string_view inputPath, std::string_view filePrefix, std::string_view fileSuffix,
                                          std::string_view fileExtension, int32 paddingDigits)
{
  std::vector<std::string> fileList;
  if(!fs::exists(inputPath))
  {
    return fileList;
  }
  if(increment < 1)
  {
    return fileList;
  }
  int32 index = 0;
  std::string format_string = fmt::format("{{}}/{{}}{{:0{}d}}{{}}{{}}", paddingDigits);
  for(int32 i = 0; i < (end - start) + 1; i += increment)
  {
    if(order == Ordering::LowToHigh)
    {
      index = start + i;
    }
    else
    {
      index = end - i;
    }
    std::string filePath = fmt::format(format_string, inputPath, filePrefix, index, fileSuffix, fileExtension);
    fileList.push_back(filePath);
  }

  return fileList;
}
} // namespace FilePathGenerator
} // namespace complex
