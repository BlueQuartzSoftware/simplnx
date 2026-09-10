/**
 * @file PrebuiltPipelineTest.cpp
 * @brief Verifies that every prebuilt example pipeline uses current filter parameter keys.
 *
 * This test guards BlueQuartzSoftware/simplnx#1376 by treating IFilter::fromJson warnings -5432, -5433, and -5434 as test failures. A failure means that a shipped pipeline's
 * argument keys are out of date with the filter's parameters() and that the pipeline file must be updated.
 */

#include <catch2/catch.hpp>

#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("Prebuilt Pipelines: Parameter Keys Are Current", "[Pipeline][PrebuiltPipelines]")
{
  UnitTest::LoadPlugins();

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
  for(const auto& pipelinePath : pipelinePaths)
  {
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

  // Prevent this test from passing when no shipped pipelines were discovered.
  const auto pipelineCount = pipelinePaths.size();
  REQUIRE(pipelineCount > 0);
}
