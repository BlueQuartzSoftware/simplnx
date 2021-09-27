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
std::vector<std::string> GenerateFileList(int32_t start, int32_t end, int32_t increment, bool& hasMissingFiles, bool stackLowToHigh, const std::string& inputPath, const std::string& filePrefix,
                                          const std::string& fileSuffix, const std::string& fileExtension, int32_t paddingDigits)
{
  std::vector<std::string> fileList;

  if(!fs::exists(inputPath))
  {
    return fileList;
  }
  int32_t index = 0;

  std::string format_string = fmt::format("{{}}/{{}}{{:0{}d}}{{}}{{}}", paddingDigits);

  bool missingFiles = false;
  for(int32_t i = 0; i < (end - start) + 1; i += increment)
  {
    if(stackLowToHigh)
    {
      index = start + i;
    }
    else
    {
      index = end - i;
    }

    std::string filePath = fmt::format(format_string, inputPath, filePrefix, index, fileSuffix, fileExtension);

    if(!fs::exists(filePath))
    {
      missingFiles = true;
    }

    fileList.push_back(filePath);
  }

  hasMissingFiles = missingFiles;

  return fileList;
}
} // namespace FilePathGenerator
} // namespace complex
