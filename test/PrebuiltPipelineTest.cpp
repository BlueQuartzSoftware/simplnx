/**
 * @file PrebuiltPipelineTest.cpp
 * @brief Verifies that every prebuilt example pipeline and workflow loads without errors or warnings.
 *
 * This test guards BlueQuartzSoftware/simplnx#1376. Any Pipeline::FromFile error or warning fails the test, including the
 * IFilter::fromJson stale-key warnings -5432, -5433, and -5434. A failure means that the listed file must be updated to the
 * current filter parameters. Pipelines that use filters from plugins disabled in this build are skipped.
 */

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
bool IsFilterAvailable(const Uuid& uuid, const FilterList& filterList)
{
  if(filterList.createFilter(uuid) != nullptr)
  {
    return true;
  }

  // FindFilterReplacement is private to PipelineFilter.cpp. Match its first-replacement lookup.
  for(const auto* plugin : filterList.getLoadedPlugins())
  {
    const auto replacements = plugin->getFilterReplacementMap();
    const auto iter = replacements.find(uuid);
    if(iter != replacements.end())
    {
      return filterList.createFilter(iter->second) != nullptr;
    }
  }
  return false;
}

std::set<std::string> FindMissingFilterUuids(const fs::path& path, const FilterList& filterList)
{
  std::ifstream stream(path);
  if(!stream.is_open())
  {
    return {};
  }

  // Both pipeline and workflow files store their filters in the top-level pipeline array.
  const auto json = nlohmann::json::parse(stream, nullptr, false);
  if(json.is_discarded() || !json.contains("pipeline") || !json["pipeline"].is_array())
  {
    return {};
  }

  std::set<std::string> missingUuids;
  for(const auto& entry : json["pipeline"])
  {
    // Let the normal loader report malformed JSON/UUIDs rather than treating them as unavailable plugins.
    if(!entry.contains("filter") || !entry["filter"].contains("uuid") || !entry["filter"]["uuid"].is_string())
    {
      return {};
    }
    const auto uuidString = entry["filter"]["uuid"].get<std::string>();
    const auto uuid = Uuid::FromString(uuidString);
    if(!uuid.has_value())
    {
      return {};
    }
    if(!IsFilterAvailable(*uuid, filterList))
    {
      missingUuids.insert(uuidString);
    }
  }
  return missingUuids;
}
} // namespace

TEST_CASE("Prebuilt Pipelines: Parameter Keys Are Current", "[Pipeline][PrebuiltPipelines]")
{
  UnitTest::LoadPlugins();
  const auto* filterList = Application::GetOrCreateInstance()->getFilterList();

  std::vector<fs::path> pipelinePaths;
  for(const auto& pluginDir : unit_test::k_PluginSourceDirs)
  {
    const fs::path pipelinesDir = pluginDir / "pipelines";
    if(!fs::exists(pipelinesDir))
    {
      continue;
    }

    for(const auto& entry : fs::recursive_directory_iterator(pipelinesDir))
    {
      if(!entry.is_regular_file())
      {
        continue;
      }

      const fs::path extension = entry.path().extension();
      if(extension == ".d3dpipeline" || extension == ".d3dworkflow")
      {
        pipelinePaths.push_back(entry.path());
      }
    }
  }

  std::sort(pipelinePaths.begin(), pipelinePaths.end());

  std::ostringstream failureReport;
  usize checkedCount = 0;
  for(const auto& pipelinePath : pipelinePaths)
  {
    const auto missingUuids = FindMissingFilterUuids(pipelinePath, *filterList);
    if(!SIMPLNX_TEST_ALL_OPTIONAL_PLUGINS_ENABLED && !missingUuids.empty())
    {
      std::ostringstream missingReport;
      for(const auto& uuid : missingUuids)
      {
        missingReport << "\n  " << uuid;
      }
      WARN("Skipping unavailable prebuilt pipeline " << pipelinePath.string() << "; missing filter UUIDs:" << missingReport.str());
      continue;
    }

    ++checkedCount;
    const Result<Pipeline> result = Pipeline::FromFile(pipelinePath, false);
    if(result.valid() && result.warnings().empty())
    {
      continue;
    }

    failureReport << "\n" << pipelinePath.string() << "\n";
    if(result.invalid())
    {
      for(const auto& error : result.errors())
      {
        failureReport << "  [" << error.code << "] " << error.message << "\n";
      }
    }
    for(const auto& warning : result.warnings())
    {
      failureReport << "  [" << warning.code << "] " << warning.message << "\n";
    }
  }

  const std::string report = failureReport.str();
  INFO("Prebuilt pipelines produced load errors or warnings:" << report);
  CHECK(report.empty());

  // Prevent this test from passing when no shipped pipelines were actually checked.
  REQUIRE(checkedCount > 0);
}
