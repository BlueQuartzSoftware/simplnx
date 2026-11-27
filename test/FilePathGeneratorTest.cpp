#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <fmt/core.h>

using namespace nx::core;

TEST_CASE("FilePathGenerator")
{
  GeneratedFileListParameter::ValueType value;
  value.start_index = 1;
  value.end_index = 2;
  value.increment_index = 1;
  value.padding_digits = 1;
  value.file_prefix = "TestFilter";
  value.file_suffix = "";
  value.file_extension = ".cpp";
  value.input_path = fmt::format("{}/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/", unit_test::k_SourceDir.view());
  value.ordering = GeneratedFileListParameter::OrderingType::LowToHigh;

  // Generate the file list but do *NOT* validate the paths. this is a test after all

  auto fileList = value.generate();
  REQUIRE(fileList.size() == 2);
}
