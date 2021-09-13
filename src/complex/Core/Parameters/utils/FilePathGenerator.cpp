

#include "FilePathGenerator.hpp"

#include <fmt/core.h>

#include <filesystem>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace complex
{

namespace FilePathGenerator
{

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
std::vector<std::string> FilePathGenerator::GenerateFileList(int32_t start, int32_t end, int32_t increment, bool& hasMissingFiles, bool stackLowToHigh, const std::string& inputPath,
                                                             const std::string& filePrefix, const std::string& fileSuffix, const std::string& fileExtension, int32_t paddingDigits)
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

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
std::vector<std::string> FilePathGenerator::GenerateVectorFileList(int32_t start, int32_t end, int32_t compStart, int32_t compEnd, bool& hasMissingFiles, bool stackLowToHigh,
                                                                   const std::string& inputPath, const std::string& filePrefix, const std::string& separator, const std::string& fileSuffix,
                                                                   const std::string& fileExtension, int32_t paddingDigits)
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

#if 0
// -----------------------------------------------------------------------------
FilePathGenerator::TileRCIndexLayout2D FilePathGenerator::GenerateRCIndexMontageFileList(int32_t rowStart, int32_t rowEnd, int32_t colStart, int32_t colEnd, bool& hasMissingFiles, bool rcOrdering,
                                                                                         const std::string& inputPath, const std::string& filePrefix, const std::string& fileSuffix, const std::string& fileExtension,
                                                                                         int32_t paddingDigits)
{
  TileRCIndexLayout2D tileLayout2D;

  QDir dir(inputPath);
  if(!dir.exists())
  {
    return tileLayout2D;
  }

  bool missingFiles = false;

  for(int32_t r = rowStart; r < rowEnd; r++)
  {
    TileRCIndexRow2D tileRow2D;
    for(int32_t c = colStart; c < colEnd; c++)
    {
      TileRCIndex2D tile2D;

      std::string filePath;
      QTextStream fn(&filePath);
      fn << inputPath;
      if(!inputPath.endsWith("/"))
      {
        fn << "/";
      }
      fn << filePrefix;

      if(rcOrdering)
      {
        fn.setFieldWidth(0);
        fn << "r";

        fn.setFieldWidth(paddingDigits);
        fn.setFieldAlignment(QTextStream::AlignRight);
        fn.setPadChar('0');
        fn << r;

        fn.setFieldWidth(0);
        fn << "c";

        fn.setFieldWidth(paddingDigits);
        fn.setFieldAlignment(QTextStream::AlignRight);
        fn.setPadChar('0');
        fn << c;
      }
      else
      {
        fn.setFieldWidth(0);
        fn << "c";

        fn.setFieldWidth(paddingDigits);
        fn.setFieldAlignment(QTextStream::AlignRight);
        fn.setPadChar('0');
        fn << c;

        fn.setFieldWidth(0);
        fn << "r";

        fn.setFieldWidth(paddingDigits);
        fn.setFieldAlignment(QTextStream::AlignRight);
        fn.setPadChar('0');
        fn << r;
      };

      fn.setFieldWidth(0);
      fn << fileSuffix << "." << fileExtension;

      filePath = QDir::toNativeSeparators(filePath);

      tile2D.FileName = filePath;
      tile2D.data = {{r, c}};

      QFileInfo fi(filePath);
      if(!fi.exists())
      {
        missingFiles = true;
      }

      tileRow2D.emplace_back(tile2D);
    }

    // Push the row back on the TileLayout2D
    tileLayout2D.emplace_back(tileRow2D);
  }
  hasMissingFiles = missingFiles;

  return tileLayout2D;
}
#endif

} // namespace FilePathGenerator
