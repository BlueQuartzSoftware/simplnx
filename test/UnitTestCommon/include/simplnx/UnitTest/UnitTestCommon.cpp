#include "UnitTestCommon.hpp"

#include "simplnx/Parameters/Dream3dImportParameter.hpp"

#include <reproc++/reproc.hpp>
#include <reproc++/run.hpp>

namespace nx::core::UnitTest
{
DataStructure LoadDataStructure(const fs::path& filepath)
{
  // Ensure the plugins a loaded.
  LoadPlugins();

  INFO(fmt::format("Error loading file: '{}'  ", filepath.string()));
  REQUIRE(fs::exists(filepath));

  DataStructure dataStructure;

  // const Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
  auto* filterList = Application::Instance()->getFilterList();
  /*************************************************************************
   * ReadDREAM3DFilter
   ************************************************************************/
  constexpr Uuid k_ReadDREAM3DFilterId = *Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
  const FilterHandle k_ReadDREAM3DFilterHandle(k_ReadDREAM3DFilterId, k_SimplnxCorePluginId);

  auto filterPtr = filterList->createFilter(k_ReadDREAM3DFilterHandle);
  REQUIRE(nullptr != filterPtr);

  Arguments args;
  args.insertOrAssign("import_data_object", std::make_any<Dream3dImportParameter::ImportData>(Dream3dImportParameter::ImportData{filepath, Dream3dImportParameter::PathImportPolicy::All}));

  // Preflight the filter and check result
  auto preflightResult = filterPtr->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filterPtr->execute(dataStructure, args); //, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  return dataStructure;
}

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
} // namespace nx::core::UnitTest
