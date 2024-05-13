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
  auto exemplarFilePath = fs::path(fmt::format("{}/simpl_json_exemplars/uuid", unit_test::k_TestFilesDir));

  auto appPtr = Application::GetOrCreateInstance();
  appPtr->loadPlugins(unit_test::k_BuildDir.view());
  REQUIRE(appPtr != nullptr);

  appPtr->loadPlugins(unit_test::k_BuildDir.view());
  auto* filterListPtr = Application::Instance()->getFilterList();
  const auto pluginListPtr = Application::Instance()->getPluginList();

  std::stringstream output;

  // Loop on each Plugin
  for(const auto& plugin : pluginListPtr)
  {
    const std::string plugName = plugin->getName();
    auto simplFilterMap = plugin->getSimplToSimplnxMap();

    for(const auto& simplMapIter : simplFilterMap)
    {
      IFilter::UniquePointer filter = filterListPtr->createFilter(simplMapIter.second.simplnxUuid);
      std::string simplJsonFilePath = fmt::format("{}/{}.json", exemplarFilePath.string(), simplMapIter.first.str());
      if(std::filesystem::exists({simplJsonFilePath}))
      {
        Result<Pipeline> result = Pipeline::FromSIMPLFile(simplJsonFilePath, filterListPtr);
        SIMPLNX_RESULT_REQUIRE_VALID(result)
      }
      else
      {
        std::cout << plugName << "::" << filter->className() << "    SIMPL UUID FILE DOES NOT EXIST: " << simplJsonFilePath << std::endl;
      }
    }
  }
}
