#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <filesystem>
#include <map>
#include <sstream>
#include <string>

using namespace nx::core;

TEST_CASE("nx::core::Test SIMPL Json Conversion", "[simplnx][Filter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "simpl_json_exemplars.tar.gz", "simpl_json_exemplars");
  // Read Exemplar DREAM3D File Filter
  auto exemplarDirPath = fs::path(fmt::format("{}/simpl_json_exemplars/uuid", unit_test::k_TestFilesDir));

  auto appPtr = Application::GetOrCreateInstance();
  REQUIRE(appPtr != nullptr);

  appPtr->loadPlugins(unit_test::k_BuildDir.view());
  FilterList* filterList = appPtr->getFilterList();

  // Loop on each Plugin
  for(const auto* plugin : appPtr->getPluginList())
  {
    for(const auto& [simplID, conversionData] : plugin->getSimplToSimplnxMap())
    {
      IFilter::UniquePointer filter = filterList->createFilter(conversionData.simplnxUuid);
      REQUIRE(filter != nullptr);
      fs::path simplJsonFilePath = exemplarDirPath / fmt::format("{}.json", simplID.str());
      if(std::filesystem::exists(simplJsonFilePath))
      {
        Result<Pipeline> result = Pipeline::FromSIMPLFile(simplJsonFilePath, filterList);
        SIMPLNX_RESULT_REQUIRE_VALID(result)
      }
      else
      {
        fmt::print("{}::{}    SIMPL UUID FILE DOES NOT EXIST: {}\n", plugin->getName(), filter->className(), simplJsonFilePath.string());
      }
    }
  }
}
