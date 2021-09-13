
#include "complex/Core/Parameters/utils/FilePathGenerator.hpp"
#include "complex/Core/Parameters/utils/StackFileListInfo.hpp"

#include <catch2/catch.hpp>

#include <fmt/core.h>

TEST_CASE("FilePathGenerator")
{

  complex::StackFileListInfo info;
  info.setStartIndex(9);
  info.setEndIndex(12);
  info.setIncrementIndex(1);
  info.setPaddingDigits(1);
  info.setFilePrefix("LEROY_0107_2_section_");
  info.setFileSuffix("_x10_U16");
  info.setFileExtension(".tif");
  info.setInputPath("/LEROY_0107/Mosaics_x10_U16");
  info.setOrdering(0);

  // Generate the file list but do *NOT* validate the paths.. this is a test after all
  std::vector<std::string> fileList = complex::FilePathGenerator::GenerateFileList2(info, false);
  REQUIRE(fileList.size() == 4);

  //  for(const auto& file : fileList)
  //  {
  //    fmt::print("{}\n", file);
  //  }
}
