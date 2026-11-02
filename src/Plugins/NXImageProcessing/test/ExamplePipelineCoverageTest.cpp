#include "ImageProcessing/ImageProcessing_test_dirs.hpp"

#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
std::string Trim(std::string value)
{
  const auto isNotSpace = [](unsigned char character) { return std::isspace(character) == 0; };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
  value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
  return value;
}

std::string ReadTextFile(const fs::path& path)
{
  std::ifstream stream(path);
  REQUIRE(stream.is_open());
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

nlohmann::json ReadJsonFile(const fs::path& path)
{
  std::ifstream stream(path);
  REQUIRE(stream.is_open());
  return nlohmann::json::parse(stream);
}

std::set<std::string> ReadCMakeFilterList(const fs::path& cmakePath)
{
  const std::string cmakeText = ReadTextFile(cmakePath);
  constexpr std::string_view k_FilterListStart = "set(FilterList";

  const usize listStart = cmakeText.find(k_FilterListStart);
  REQUIRE(listStart != std::string::npos);
  const usize contentStart = listStart + k_FilterListStart.size();
  const usize listEnd = cmakeText.find("\n)", contentStart);
  REQUIRE(listEnd != std::string::npos);

  std::set<std::string> filterNames;
  std::istringstream lines(cmakeText.substr(contentStart, listEnd - contentStart));
  for(std::string line; std::getline(lines, line);)
  {
    line = Trim(std::move(line));
    if(!line.empty() && !line.starts_with('#'))
    {
      filterNames.insert(std::move(line));
    }
  }
  return filterNames;
}

void RequireNonEmptyArray(const nlohmann::json& object, const std::string& key)
{
  INFO("Required non-empty array: " << key);
  REQUIRE(object.contains(key));
  REQUIRE(object.at(key).is_array());
  REQUIRE_FALSE(object.at(key).empty());
}

void ValidatePythonGenerationSpec(const fs::path& exampleDir)
{
  const fs::path specPath = exampleDir / "PythonGeneration.yaml";
  CAPTURE(specPath);
  REQUIRE(fs::exists(specPath));

  const nlohmann::json specification = ReadJsonFile(specPath);
  REQUIRE(specification.at("schema_version").get<int32>() == 1);
  REQUIRE(specification.at("id").get<std::string>() == "imageprocessing-python-generation-v1");
  REQUIRE_FALSE(specification.at("converter_api").get<std::string>().empty());
  RequireNonEmptyArray(specification, "required_modules");
  REQUIRE_FALSE(specification.at("working_directory").get<std::string>().empty());
  REQUIRE_FALSE(specification.at("output_lifetime").get<std::string>().empty());
  REQUIRE_FALSE(specification.at("failure_behavior").get<std::string>().empty());
  REQUIRE_FALSE(specification.at("regeneration_rule").get<std::string>().empty());
  REQUIRE(specification.at("validation").is_object());
}

void ValidateCompanionFiles(const fs::path& exampleDir, const std::string& pipelineName)
{
  const fs::path pipelineFilename = fs::path(pipelineName).filename();
  const fs::path exampleFolder = exampleDir / pipelineFilename.stem();
  const fs::path pipelinePath = exampleFolder / pipelineFilename;
  fs::path basePath = pipelinePath;
  basePath.replace_extension();
  const fs::path pythonPath = basePath.string() + ".py";
  const fs::path yamlPath = basePath.string() + ".yaml";
  const fs::path markdownPath = basePath.string() + ".md";

  CAPTURE(pipelinePath, pythonPath, yamlPath, markdownPath);
  REQUIRE(exampleFolder.filename() == pipelineFilename.stem());
  REQUIRE(fs::exists(pipelinePath));
  REQUIRE(fs::exists(yamlPath));
  REQUIRE(fs::exists(markdownPath));
  REQUIRE_FALSE(fs::exists(pythonPath));

  const nlohmann::json pipelineJson = ReadJsonFile(pipelinePath);
  const nlohmann::json metadata = ReadJsonFile(yamlPath);
  const std::string markdownText = ReadTextFile(markdownPath);

  REQUIRE(metadata.at("schema_version").get<int32>() == 1);
  REQUIRE(metadata.at("pipeline_file").get<std::string>() == pipelinePath.filename().string());
  REQUIRE_FALSE(metadata.contains("python_file"));
  REQUIRE(metadata.at("markdown_file").get<std::string>() == markdownPath.filename().string());
  REQUIRE_FALSE(metadata.at("title").get<std::string>().empty());
  REQUIRE_FALSE(metadata.at("summary").get<std::string>().empty());
  RequireNonEmptyArray(metadata, "primary_industries");
  RequireNonEmptyArray(metadata, "use_cases");
  RequireNonEmptyArray(metadata, "keywords");
  RequireNonEmptyArray(metadata, "inputs");
  RequireNonEmptyArray(metadata, "outputs");
  RequireNonEmptyArray(metadata, "citations");

  const nlohmann::json& inputStrategy = metadata.at("input_data_strategy");
  const std::string strategyName = inputStrategy.at("strategy").get<std::string>();
  REQUIRE_FALSE(strategyName.empty());
  RequireNonEmptyArray(inputStrategy, "asset_keys");
  REQUIRE_FALSE(inputStrategy.at("reference_visual").get<std::string>().empty());
  RequireNonEmptyArray(inputStrategy, "target_traits");
  REQUIRE_FALSE(inputStrategy.at("known_differences").get<std::string>().empty());
  if(strategyName.find("synthetic") != std::string::npos)
  {
    const nlohmann::json& generator = inputStrategy.at("generator");
    REQUIRE(generator.at("path").get<std::string>() == "Generators/GenerateImageProcessingExampleData.py");
    REQUIRE(generator.at("seed").get<int64>() == 20260901);
    REQUIRE(generator.at("metrics_file").get<std::string>() == "PaperInputMetrics.json");
  }
  if(strategyName == "original-paper-linked")
  {
    REQUIRE_FALSE(inputStrategy.at("source_url").get<std::string>().empty());
    REQUIRE(inputStrategy.at("source_sha512").get<std::string>().size() == 128);
  }

  const nlohmann::json& pythonGeneration = metadata.at("python_generation");
  REQUIRE(pythonGeneration.at("specification_file").get<std::string>() == "PythonGeneration.yaml");
  REQUIRE(pythonGeneration.at("resolve_from").get<std::string>() == "pipeline_suite_root");
  REQUIRE(pythonGeneration.at("specification_id").get<std::string>() == "imageprocessing-python-generation-v1");

  const nlohmann::json& mcp = metadata.at("mcp");
  RequireNonEmptyArray(mcp, "user_intents");
  RequireNonEmptyArray(mcp, "questions_to_ask");
  RequireNonEmptyArray(mcp, "constraints");

  const nlohmann::json& pipelineSteps = pipelineJson.at("pipeline");
  const nlohmann::json& metadataSteps = metadata.at("steps");
  REQUIRE(metadataSteps.is_array());
  REQUIRE(metadataSteps.size() == pipelineSteps.size());

  for(usize stepIndex = 0; stepIndex < pipelineSteps.size(); stepIndex++)
  {
    const nlohmann::json& pipelineStep = pipelineSteps.at(stepIndex);
    const nlohmann::json& metadataStep = metadataSteps.at(stepIndex);
    const std::string filterName = pipelineStep.at("filter").at("name").get<std::string>();
    const std::string shortFilterName = filterName.substr(filterName.rfind(':') + 1);
    const std::string filterUuid = pipelineStep.at("filter").at("uuid").get<std::string>();

    CAPTURE(stepIndex, filterName, filterUuid);
    REQUIRE(metadataStep.at("index").get<usize>() == stepIndex);
    REQUIRE(metadataStep.at("filter").get<std::string>() == shortFilterName);
    REQUIRE(metadataStep.at("uuid").get<std::string>() == filterUuid);
    REQUIRE_FALSE(metadataStep.at("role").get<std::string>().empty());
    REQUIRE(metadataStep.contains("key_parameters"));
  }

  constexpr std::array<std::string_view, 10> k_RequiredHeadings = {
      "## Purpose and real-world setting",
      "## When to use this workflow",
      "## Input data and assumptions",
      "## Data flow",
      "## Step-by-step filter explanation and parameter rationale",
      "## Outputs and how to interpret them",
      "## How to adapt the pipeline",
      "## Failure modes and quality checks",
      "## Scientific basis and annotated references",
      "## Guidance for an LLM or MCP assistant",
  };
  const usize executablePosition = markdownText.find("Executable pipeline: `" + pipelinePath.filename().string() + "`");
  const usize warningPosition =
      markdownText.find("Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.");
  const usize purposePosition = markdownText.find("## Purpose and real-world setting");
  REQUIRE(executablePosition != std::string::npos);
  REQUIRE(warningPosition != std::string::npos);
  REQUIRE(purposePosition != std::string::npos);
  REQUIRE(executablePosition < warningPosition);
  REQUIRE(warningPosition < purposePosition);
  for(const std::string_view heading : k_RequiredHeadings)
  {
    INFO("Required Markdown heading: " << heading);
    REQUIRE(markdownText.find(heading) != std::string::npos);
  }

  const nlohmann::json& workflowCitation = metadata.at("citations").front();
  REQUIRE(workflowCitation.at("scope").get<std::string>() == "pipeline_workflow");
  const nlohmann::json& dataset = workflowCitation.at("dataset");
  REQUIRE_FALSE(dataset.at("availability").get<std::string>().empty());
  REQUIRE_FALSE(dataset.at("license").get<std::string>().empty());
  REQUIRE(dataset.at("included").is_boolean());
  const std::string reproductionStatus = dataset.at("reproduction_status").get<std::string>();
  REQUIRE((reproductionStatus == "exact" || reproductionStatus == "method_reproduction" || reproductionStatus == "illustrative"));
  REQUIRE_FALSE(dataset.at("reproduction_notes").get<std::string>().empty());

  for(const nlohmann::json& citation : metadata.at("citations"))
  {
    REQUIRE_FALSE(citation.at("title").get<std::string>().empty());
    REQUIRE_FALSE(citation.at("role").get<std::string>().empty());
    const std::string doi = citation.value("doi", "");
    const std::string url = citation.value("url", "");
    REQUIRE_FALSE((doi.empty() && url.empty()));
    if(!doi.empty())
    {
      INFO("DOI missing from Markdown: " << doi);
      REQUIRE(markdownText.find(doi) != std::string::npos);
    }
  }
}
} // namespace

TEST_CASE("ImageProcessing::Every registered filter has an enabled real-world example", "[ImageProcessing][ExamplePipelines][Coverage]")
{
  UnitTest::LoadPlugins();

  const fs::path pluginSourceDir(unit_test::k_SourceDir.str());
  const fs::path exampleDir = pluginSourceDir / "pipelines";
  const fs::path manifestPath = exampleDir / "FilterCoverage.json";
  const std::set<std::string> registeredFilters = ReadCMakeFilterList(pluginSourceDir / "CMakeLists.txt");

  REQUIRE(registeredFilters.size() == 92);
  INFO("Missing example coverage manifest: " << manifestPath.string());
  REQUIRE(fs::exists(manifestPath));
  ValidatePythonGenerationSpec(exampleDir);

  const nlohmann::json manifest = ReadJsonFile(manifestPath);
  REQUIRE(manifest.at("version").get<int32>() == 1);
  const nlohmann::json& assignments = manifest.at("filters");
  REQUIRE(assignments.is_array());
  REQUIRE(assignments.size() == registeredFilters.size());

  std::set<std::string> assignedFilters;
  std::set<std::string> assignedPipelines;
  for(const nlohmann::json& assignment : assignments)
  {
    const std::string filterName = assignment.at("filter").get<std::string>();
    const std::string uuidString = assignment.at("uuid").get<std::string>();
    const std::string pipelineName = assignment.at("pipeline").get<std::string>();
    const usize stepIndex = assignment.at("step").get<usize>();

    INFO("Filter: " << filterName);
    REQUIRE(registeredFilters.contains(filterName));
    REQUIRE(assignedFilters.insert(filterName).second);

    const std::optional<Uuid> uuid = Uuid::FromString(uuidString);
    REQUIRE(uuid.has_value());
    REQUIRE(Application::Instance()->getFilterList()->createFilter(*uuid) != nullptr);

    const fs::path pipelinePath = exampleDir / pipelineName;
    INFO("Pipeline: " << pipelinePath.string());
    REQUIRE(fs::exists(pipelinePath));
    assignedPipelines.insert(pipelineName);

    const nlohmann::json pipelineJson = ReadJsonFile(pipelinePath);
    const nlohmann::json& steps = pipelineJson.at("pipeline");
    REQUIRE(steps.is_array());
    REQUIRE(stepIndex < steps.size());

    const nlohmann::json& step = steps.at(stepIndex);
    REQUIRE_FALSE(step.value("isDisabled", false));
    REQUIRE(step.at("filter").at("name").get<std::string>() == "nx::core::" + filterName);
    REQUIRE(step.at("filter").at("uuid").get<std::string>() == uuidString);

    const Result<Pipeline> pipelineResult = Pipeline::FromJson(pipelineJson);
    if(pipelineResult.invalid())
    {
      for(const Error& error : pipelineResult.errors())
      {
        UNSCOPED_INFO("Pipeline parse error " << error.code << ": " << error.message);
      }
    }
    REQUIRE(pipelineResult.valid());
  }

  REQUIRE(assignedFilters == registeredFilters);
  REQUIRE(assignedPipelines.size() == 16);
  for(const std::string& pipelineName : assignedPipelines)
  {
    ValidateCompanionFiles(exampleDir, pipelineName);
  }
}
