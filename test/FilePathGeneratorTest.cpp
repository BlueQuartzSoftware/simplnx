
#include "complex/Core/Parameters/utils/FilePathGenerator.hpp"
#include "complex/Core/Parameters/GeneratedFileListParameter.hpp"

#include "complex/unit_test/complex_test_dirs.h"

#include <catch2/catch.hpp>

#include <fmt/core.h>

TEST_CASE("FilePathGenerator")
{

  complex::GeneratedFileListParameter::ValueType value;
  value.startIndex = 1;
  value.endIndex = 2;
  value.incrementIndex = 1;
  value.paddingDigits = 1;
  value.filePrefix = "TestFilter";
  value.fileSuffix = "";
  value.fileExtension = ".cpp";
  value.inputPath = fmt::format("{}/src/complex/Core/Filters/", complex::unit_test::k_ComplexSourceDir);
  value.ordering = complex::GeneratedFileListParameter::Ordering::LowToHigh;

  bool missingFiles = false;
  // Generate the file list but do *NOT* validate the paths. this is a test after all
  std::vector<std::string> fileList =
      complex::FilePathGenerator::GenerateFileList(value.startIndex, value.endIndex, value.incrementIndex, missingFiles, (value.ordering == complex::GeneratedFileListParameter::Ordering::LowToHigh),
                                                   value.inputPath, value.filePrefix, value.fileSuffix, value.fileExtension, value.paddingDigits);
  REQUIRE(fileList.size() == 2);
}
