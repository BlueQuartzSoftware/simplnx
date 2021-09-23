
#include "complex/Core/Parameters/utils/FilePathGenerator.hpp"
#include "complex/Core/Parameters/GeneratedFileListParameter.hpp"

#include "complex/unit_test/complex_test_dirs.h"

#include <catch2/catch.hpp>

#include <fmt/core.h>

TEST_CASE("FilePathGenerator")
{

  complex::GeneratedFileListParameter::ValueType value;
  value.StartIndex = 1;
  value.EndIndex = 2;
  value.IncrementIndex = 1;
  value.PaddingDigits = 1;
  value.FilePrefix = "TestFilter";
  value.FileSuffix = "";
  value.FileExtension = ".cpp";
  value.InputPath = fmt::format("{}/src/complex/Core/Filters/", complex::unit_test::k_ComplexSourceDir);
  value.Ordering = complex::GeneratedFileListParameter::Ordering::LowToHigh;

  bool missingFiles = false;
  // Generate the file list but do *NOT* validate the paths. this is a test after all
  std::vector<std::string> fileList =
      complex::FilePathGenerator::GenerateFileList(value.StartIndex, value.EndIndex, value.IncrementIndex, missingFiles, (value.Ordering == complex::GeneratedFileListParameter::Ordering::LowToHigh),
                                                   value.InputPath, value.FilePrefix, value.FileSuffix, value.FileExtension, value.PaddingDigits);
  REQUIRE(fileList.size() == 2);
}
