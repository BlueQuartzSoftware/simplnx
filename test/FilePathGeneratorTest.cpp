#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <fmt/core.h>

using namespace nx::core;

TEST_CASE("FilePathGenerator")
{
  GeneratedFileListParameter::ValueType value;
  value.startIndex = 1;
  value.endIndex = 2;
  value.incrementIndex = 1;
  value.paddingDigits = 1;
  value.filePrefix = "TestFilter";
  value.fileSuffix = "";
  value.fileExtension = ".cpp";
  value.inputPath = fmt::format("{}/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/", unit_test::k_SourceDir.view());
  value.ordering = GeneratedFileListParameter::Ordering::LowToHigh;

  // Generate the file list but do *NOT* validate the paths. this is a test after all

  auto fileList = value.generate();
  REQUIRE(fileList.size() == 2);
}
