#pragma once

#include "complex/Core/Parameters/utils/StackFileListInfo.hpp"
#include "complex/complex_export.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/core.h>

namespace fs = std::filesystem;

namespace complex
{

namespace FilePathGenerator
{

std::vector<std::string> GenerateFileList2(const StackFileListInfo& info, bool validatePaths)
{
  std::vector<std::string> fileList;
  // make sure the input path exists. If it does *not* then just bail out now.
  if(validatePaths && !fs::exists(info.getInputPath()))
  {
    return fileList;
  }
  int32_t index = 0;
  // Create the actual format string
  std::stringstream fmt_string_stream;
  fmt_string_stream << "{}/{}{:0" << info.getPaddingDigits() << "d}{}{}";

  bool missingFiles = false;
  for(int32_t i = 0; i < (info.getEndIndex() - info.getStartIndex()) + 1; i = i + info.getIncrementIndex())
  {
    if(info.getOrdering() == 0)
    {
      index = info.getStartIndex() + i;
    }
    else
    {
      index = info.getEndIndex() - i;
    }

    std::string filePath = fmt::format(fmt_string_stream.str(), info.getInputPath(), info.getFilePrefix(), index, info.getFileSuffix(), info.getFileExtension());

    if(validatePaths && !fs::exists(filePath))
    {
      missingFiles = true;
    }

    fileList.push_back(filePath);
  }

  if(missingFiles)
  {
    return {};
  }
  return fileList;
}

/**
 * @brief generateFileList This method will generate a file list in the correct order of the files that should
 * be imported int32_to an h5ebsd file
 * @param start Z Slice Start
 * @param end S Slice End
 * @param increment How much to increment each item
 * @param hasMissingFiles Are any files missing
 * @param stackLowToHigh Is the Slice order low to high
 * @param filename Example File Name
 * @return
 */
std::vector<std::string> GenerateFileList(int32_t start, int32_t end, int32_t increment, bool& hasMissingFiles, bool stackLowToHigh, const std::string& inputPath, const std::string& filePrefix,
                                          const std::string& fileSuffix, const std::string& fileExtension, int32_t paddingDigits)
{
  std::vector<std::string> fileList;

  if(!fs::exists(inputPath))
  {
    return fileList;
  }
  int32_t index = 0;

  std::stringstream fmt_string_stream;

  fmt_string_stream << "{}/{}{:0" << paddingDigits << "d}{}{}";

  bool missingFiles = false;
  for(int32_t i = 0; i < (end - start) + 1; i = i + increment)
  {
    if(stackLowToHigh)
    {
      index = start + i;
    }
    else
    {
      index = end - i;
    }

    std::string filePath = fmt::format(fmt_string_stream.str(), inputPath, filePrefix, index, fileSuffix, fileExtension);

    if(!fs::exists(filePath))
    {
      missingFiles = true;
    }

    fileList.push_back(filePath);
  }

  hasMissingFiles = missingFiles;

  return fileList;
}

/**
 * @brief generateFileList This method will generate a file list in the correct order of the files that should
 * be imported int32_to an h5ebsd file
 * @param start Z Slice Start
 * @param end S Slice End
 * @param hasMissingFiles Are any files missing
 * @param stackLowToHigh Is the Slice order low to high
 * @param filename Example File Name
 * @return
 */
std::vector<std::string> GenerateVectorFileList(int32_t start, int32_t end, int32_t compStart, int32_t compEnd, bool& hasMissingFiles, bool stackLowToHigh, const std::string& inputPath,
                                                const std::string& filePrefix, const std::string& separator, const std::string& fileSuffix, const std::string& fileExtension, int32_t paddingDigits)
{
  std::vector<std::string> fileList;
  if(!fs::exists(inputPath))
  {
    return fileList;
  }
  //  int32_t index = 0;
  //  int32_t index2 = 0;
  //
  //  std::string filename;
  //  for(int32_t i = 0; i < (end - start) + 1; ++i)
  //  {
  //    for(int32_t j = 0; j < (compEnd - compStart) + 1; j++)
  //    {
  //      if(stackLowToHigh)
  //      {
  //        index = start + i;
  //      }
  //      else
  //      {
  //        index = end - i;
  //      }
  //
  //      index2 = compStart + j;
  //
  //      filename =
  //          std::string("%1%2%3%4%5.%6").arg(filePrefix).arg(std::string::number(index), paddingDigits, '0').arg(separator).arg(std::string::number(index2), paddingDigits,
  //          '0').arg(fileSuffix).arg(fileExtension);
  //      std::string filePath = inputPath + QDir::separator() + filename;
  //      filePath = QDir::toNativeSeparators(filePath);
  //      fileList.push_back(filePath);
  //    }
  //  }
  return fileList;
}
/**
 * @brief This method will generate a list of file paths based on the inputs provided. This method is primarily aimed
 * at generating file paths based on a Row & Column naming convention. The inputs allow for the generation of zero
 * based or offset based file names.
 * @param rowStart The starting row
 * @param rowEnd The Ending row
 * @param colStart The starting column
 * @param colEnd The Ending column
 * @param hasMissingFiles Are there missing files
 * @param rcOrdering Is the Filename ordering Row-Column or Column-Row
 * @param inputPath The Path to the files
 * @param filePrefix The file prefix
 * @param fileSuffix The file suffix
 * @param fileExtension The file extension to use
 * @param paddingDigits The number of padding digits (ZERO character) to use for each of the indices.
 * @return
 */
//    static TileRCIndexLayout2D GenerateRCIndexMontageFileList(int32_t rowStart, int32_t rowEnd, int32_t colStart, int32_t colEnd, bool& hasMissingFiles, bool rcOrdering, const std::string&
//    inputPath,
//                                                              const std::string& filePrefix, const std::string& fileSuffix, const std::string& fileExtension, int32_t paddingDigits);

} // namespace FilePathGenerator

} // namespace complex
