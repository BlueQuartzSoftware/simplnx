
#include "complex/Core/Parameters/utils/FilePathGenerator.hpp"
#include "complex/Core/Parameters/GeneratedFileListParameter.hpp"

#include "complex_test_dirs.h"

#include <catch2/catch.hpp>

#include <fmt/core.h>

TEST_CASE("FilePathGenerator")
{

  complex::GeneratedFileListParameter::ValueType value;
  value.m_StartIndex = 1;
  value.m_EndIndex = 2;
  value.m_IncrementIndex = 1;
  value.m_PaddingDigits = 1;
  value.m_FilePrefix = "TestFilter";
  value.m_FileSuffix = "";
  value.m_FileExtension = ".cpp";
  value.m_InputPath = fmt::format("{}/src/complex/Core/Filters/", complex::unit_test::k_ComplexSourceDir);
  value.m_Ordering = complex::GeneratedFileListParameter::Ordering::LowToHigh;

  // Generate the file list but do *NOT* validate the paths. this is a test after all
  std::vector<std::string> fileList =
      complex::FilePathGenerator::GenerateFileList(value.m_StartIndex, value.m_EndIndex, value.m_IncrementIndex, true, (value.m_Ordering == complex::GeneratedFileListParameter::Ordering::LowToHigh),
                                                   value.m_InputPath, value.m_FilePrefix, value.m_FileSuffix, value.m_FileExtension, value.m_PaddingDigits);
  REQUIRE(fileList.size() == 2);
}
