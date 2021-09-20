
#include "complex/Core/Parameters/utils/FilePathGenerator.hpp"
#include "complex/Core/Parameters/GeneratedFileListParameter.hpp"
#include <catch2/catch.hpp>

#include <fmt/core.h>

TEST_CASE("FilePathGenerator")
{

  complex::GeneratedFileListParameter::ValueType value;
  value.m_StartIndex = 9;
  value.m_EndIndex = 12;
  value.m_IncrementIndex = 1;
  value.m_PaddingDigits = 1;
  value.m_FilePrefix = "LEROY_0107_2_section_";
  value.m_FileSuffix = "_x10_U16";
  value.m_FileExtension = ".tif";
  value.m_InputPath = "/Volumes/970-2/LEROY_0107/Mosaics_x10_U16/";
  value.m_Ordering = 0;

  // Generate the file list but do *NOT* validate the paths. this is a test after all
  std::vector<std::string> fileList = complex::FilePathGenerator::GenerateFileList(value.m_StartIndex, value.m_EndIndex, value.m_IncrementIndex, true, (value.m_Ordering == 0), value.m_InputPath,
                                                                                   value.m_FilePrefix, value.m_FileSuffix, value.m_FileExtension, value.m_PaddingDigits);
  REQUIRE(fileList.size() == 4);

  //  for(const auto& file : fileList)
  //  {
  //    fmt::print("{}\n", file);
  //  }
}
