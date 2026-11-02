#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"
#include "simplnx/Plugin/PluginLoader.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "ImageProcessing/ImageProcessingPlugin.hpp"
#include "ImageProcessing/ImageProcessing_test_dirs.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

/**
 * @file SimplConversionCoverageTest.cpp
 * @brief Proves that every legacy mapping the ImageProcessing plugin publishes still works.
 *
 * The plugin publishes two maps. The SIMPL map converts a DREAM3D 6.x filter into a current filter.
 * The replacement map redirects a retired simplnx filter Uuid to the current filter Uuid.
 *
 * The ImageProcessing plugin took both maps over from the retired ITKImageProcessing plugin. The
 * deletion of that plugin dropped 59 SIMPL conversions, and no test reported the loss. The tests
 * below read the live maps instead of a copied table. An entry that disappears therefore fails a
 * test, and an entry that stops working fails a test.
 */

namespace
{
/**
 * @brief Holds the directory that keeps the SIMPL pipeline fixtures.
 */
const fs::path k_ConversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

/**
 * @brief Names the DREAM3D 6.x pipeline versions that the fixtures cover.
 */
const std::array<std::string, 2> k_Versions = {"6_4", "6_5"};

/**
 * @brief Gives the number of entries that the SIMPL map must hold.
 *
 * Two entries come from SimplnxCore. The other 59 come from the retired ITKImageProcessing plugin.
 */
constexpr usize k_ExpectedSimplMapSize = 61;

/**
 * @brief Gives the number of entries that the filter replacement map must hold.
 *
 * The retired ITKImageProcessing plugin declared 88 filter source files.
 */
constexpr usize k_ExpectedReplacementMapSize = 88;

/**
 * @brief Gives the data container name that every restored fixture selects.
 */
const DataPath k_FixtureGeometryPath({"DataContainer"});

/**
 * @brief Gives the cell array path that every restored fixture selects.
 */
const DataPath k_FixtureArrayPath({"DataContainer", "CellData", "TestArray"});

/**
 * @brief Gives the output array name that most restored fixtures set.
 */
constexpr StringLiteral k_FixtureOutputName = "TestName";

/**
 * @brief Builds a FilterList that holds the ImageProcessing plugin only.
 *
 * The retired plugin is absent from this list. A redirect therefore runs through the replacement
 * map instead of resolving the old Uuid directly.
 *
 * @return The filter list.
 */
std::shared_ptr<FilterList> MakeImageProcessingOnlyFilterList()
{
  auto filterList = std::make_shared<FilterList>();
  const Result<> addResult = filterList->addPlugin(std::make_shared<InMemoryPluginLoader>(std::make_shared<ImageProcessingPlugin>()));
  REQUIRE(addResult.valid());
  return filterList;
}

/**
 * @brief Builds a saved pipeline node that carries a retired filter Uuid and no arguments.
 *
 * Pipeline loading substitutes a default for each absent argument, so an empty argument object is
 * enough to prove that the redirect resolves the filter.
 *
 * @param uuid The retired filter Uuid.
 * @return The saved node.
 */
nlohmann::json MakeSavedFilterNode(const Uuid& uuid)
{
  nlohmann::json node;
  node["filter"] = {{"uuid", uuid.str()}, {"name", "Retired Filter"}};
  node["args"] = nlohmann::json::object();
  node["comments"] = "";
  node["isDisabled"] = false;
  return node;
}

/**
 * @brief Gives the file name that holds the fixture of one filter.
 *
 * @param filterList The filter list that owns the filter.
 * @param uuid The current filter Uuid.
 * @return The class name of the filter.
 */
std::string ClassNameFor(const FilterList& filterList, const Uuid& uuid)
{
  auto filter = filterList.createFilter(uuid);
  REQUIRE(filter != nullptr);
  return filter->className();
}
} // namespace

TEST_CASE("ImageProcessing::every filter replacement map entry redirects a retired Uuid", "[ImageProcessing][BackwardsCompatibility][PipelineUuidRedirect]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  const ImageProcessingPlugin plugin;
  const AbstractPlugin::FilterReplacementMapType replacements = plugin.getFilterReplacementMap();

  // A dropped entry shrinks the map. The size check reports that loss before the loop hides it.
  REQUIRE(replacements.size() == k_ExpectedReplacementMapSize);

  auto filterList = MakeImageProcessingOnlyFilterList();

  // Two retired Uuids must never map to one filter, and a target must never be absent.
  std::set<Uuid> seenTargets;

  for(const auto& [oldUuid, newUuid] : replacements)
  {
    DYNAMIC_SECTION("redirect " + oldUuid.str())
    {
      // The retired Uuid must not resolve on its own, or the redirect never runs.
      REQUIRE(filterList->createFilter(oldUuid) == nullptr);

      // The target must be a filter that this plugin registers.
      auto target = filterList->createFilter(newUuid);
      REQUIRE(target != nullptr);

      const nlohmann::json node = MakeSavedFilterNode(oldUuid);
      Result<std::unique_ptr<PipelineFilter>> result = PipelineFilter::FromJson(node, *filterList);
      REQUIRE(result.valid());

      const std::unique_ptr<PipelineFilter>& pipelineFilter = result.value();
      REQUIRE(pipelineFilter != nullptr);
      REQUIRE(pipelineFilter->getFilter() != nullptr);
      REQUIRE(pipelineFilter->getFilter()->uuid() == newUuid);
    }

    seenTargets.insert(newUuid);
  }

  // Each retired Uuid must reach its own filter. A shared target means an entry points at the wrong filter.
  REQUIRE(seenTargets.size() == replacements.size());
}

TEST_CASE("ImageProcessing::every SIMPL map entry converts a DREAM3D 6.x fixture", "[ImageProcessing][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const ImageProcessingPlugin plugin;
  const AbstractPlugin::SIMPLMapType simplMap = plugin.getSimplToSimplnxMap();

  // A dropped entry shrinks the map. This is the exact loss that went unreported before.
  REQUIRE(simplMap.size() == k_ExpectedSimplMapSize);

  usize fixturesExercised = 0;
  std::set<std::string> fixturesUsed;

  for(const auto& [simplUuid, conversion] : simplMap)
  {
    const Uuid newUuid = conversion.simplnxUuid;
    const std::string className = ClassNameFor(*filterList, newUuid);

    // Every entry must own at least one fixture, or the entry is untested.
    usize fixturesForEntry = 0;

    for(const std::string& version : k_Versions)
    {
      const fs::path fixturePath = k_ConversionDir / version / (className + ".json");
      if(!fs::exists(fixturePath))
      {
        continue;
      }
      fixturesForEntry++;
      fixturesExercised++;
      fixturesUsed.insert(version + "/" + className + ".json");

      DYNAMIC_SECTION(version + "::" + className)
      {
        auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
        REQUIRE(pipelineResult.valid());

        auto& pipeline = pipelineResult.value();
        REQUIRE(pipeline.size() == 1);

        auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
        REQUIRE(pipelineFilter != nullptr);

        const IFilter* filter = pipelineFilter->getFilter();
        REQUIRE(filter != nullptr);
        REQUIRE(filter->uuid() == newUuid);

        // The conversion must move data. A conversion that silently produces the defaults resolves
        // the filter but loses every saved value, so compare the arguments with the defaults.
        const Arguments& args = pipelineFilter->getArguments();
        REQUIRE(filter->toJson(args) != filter->toJson(filter->getDefaultArguments()));

        // The restored fixtures all select DataContainer/CellData/TestArray. Check the keys that
        // the converted filter declares, so a converter that writes the wrong key fails here.
        const Parameters params = filter->parameters();
        if(params.contains("input_image_geometry_path"))
        {
          REQUIRE(args.value<DataPath>("input_image_geometry_path") == k_FixtureGeometryPath);
        }
        if(params.contains("input_image_data_path"))
        {
          REQUIRE(args.value<DataPath>("input_image_data_path") == k_FixtureArrayPath);
        }
        if(params.contains("output_array_name"))
        {
          REQUIRE(args.value<std::string>("output_array_name") == k_FixtureOutputName.str());
        }
      }
    }

    INFO("SIMPL Uuid " << simplUuid.str() << " converts to " << className);
    REQUIRE(fixturesForEntry > 0);
  }

  // Every fixture on disk must belong to a map entry. An orphan fixture means a lost map entry.
  usize fixturesOnDisk = 0;
  for(const std::string& version : k_Versions)
  {
    const fs::path versionDir = k_ConversionDir / version;
    if(!fs::is_directory(versionDir))
    {
      continue;
    }
    for(const fs::directory_entry& entry : fs::directory_iterator(versionDir))
    {
      if(!entry.is_regular_file() || entry.path().extension() != ".json")
      {
        continue;
      }
      fixturesOnDisk++;
      const std::string key = version + "/" + entry.path().filename().string();
      INFO("Fixture " << key << " has no entry in the SIMPL map");
      REQUIRE(fixturesUsed.count(key) == 1);
    }
  }

  REQUIRE(fixturesExercised == fixturesOnDisk);
}
