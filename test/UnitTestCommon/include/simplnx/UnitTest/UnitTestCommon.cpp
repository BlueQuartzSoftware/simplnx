#include "UnitTestCommon.hpp"

#include <reproc++/reproc.hpp>
#include <reproc++/run.hpp>

using namespace nx::core::UnitTest;

TestFileSentinel::TestFileSentinel(std::string cmakeExecutable, std::string testFilesDir, std::string inputArchiveName, std::string expectedTopLevelOutput, bool decompressFiles, bool removeTemp)
: m_CMakeExecutable(std::move(cmakeExecutable))
, m_TestFilesDir(std::move(testFilesDir))
, m_InputArchiveName(std::move(inputArchiveName))
, m_ExpectedTopLevelOutput(std::move(expectedTopLevelOutput))
, m_Decompress(decompressFiles)
, m_RemoveTemp(removeTemp)
{
  if(m_Decompress)
  {
    const auto errorCode = decompress();
    if(errorCode)
    {
      std::cout << "std::error_code.value(): " << errorCode.value() << std::endl;
      std::cout << "std::error_code.message(): " << errorCode.message() << std::endl;
      //        REQUIRE(errorCode.value() == 0);
    }
  }
}

TestFileSentinel::~TestFileSentinel()
{
  if(m_RemoveTemp)
  {
    std::error_code errorCode;
    std::filesystem::remove_all(fmt::format("{}/{}", m_TestFilesDir, m_ExpectedTopLevelOutput), errorCode);
    if(errorCode)
    {
      std::cout << "Removing decompressed data failed: " << errorCode.message() << std::endl;
    }
  }
}

std::error_code TestFileSentinel::decompress()
{
  reproc::options options;
  options.redirect.parent = true;
  options.deadline = reproc::milliseconds(600000);
  options.working_directory = m_TestFilesDir.c_str();
  options.nonblocking = false;

  std::vector<std::string> args = {m_CMakeExecutable, "-E", "tar", "xvzf", fmt::format("{}/{}", m_TestFilesDir, m_InputArchiveName)};

  auto resultPair = reproc::run(args, options);
  return resultPair.second;
}
