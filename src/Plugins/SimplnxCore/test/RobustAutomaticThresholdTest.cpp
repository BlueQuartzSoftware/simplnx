
#include "SimplnxCore/Filters/RobustAutomaticThresholdFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
class ThresholdPlanManager : public IDataIOManager
{
public:
  static inline constexpr StringLiteral k_Format = "threshold-plan-test";

  ThresholdPlanManager()
  {
    addDataStoreCreationFnc(k_Format.str(),
                            []([[maybe_unused]] DataType type, [[maybe_unused]] const ShapeType& tuples, [[maybe_unused]] const ShapeType& components,
                               [[maybe_unused]] const std::optional<ShapeType>& chunks) -> std::unique_ptr<IDataStore> { throw std::runtime_error("metadata routing must not allocate values"); });
  }

  std::string formatName() const override
  {
    return "threshold-plan-test-manager";
  }
};

class ThresholdResolver : public IDataStoreFormatResolver
{
public:
  enum class Mode
  {
    Adaptive,
    ForceInCore,
    ForceOutOfCore
  };

  Mode mode = Mode::Adaptive;
  mutable std::vector<DataPath> paths;
  mutable std::vector<uint64> bytes;

  std::string resolveFormat([[maybe_unused]] const DataStructure& dataStructure, [[maybe_unused]] const DataPath& path, [[maybe_unused]] DataType type, uint64 logicalBytes) const override
  {
    paths.push_back(path);
    bytes.push_back(logicalBytes);
    if(mode == Mode::ForceInCore)
    {
      return {};
    }
    if(mode == Mode::ForceOutOfCore)
    {
      return ThresholdPlanManager::k_Format.str();
    }
    return logicalBytes >= 4ULL * 1024 * 1024 * 1024 ? ThresholdPlanManager::k_Format.str() : std::string{};
  }
};
} // namespace

TEST_CASE("SimplnxCore::RobustAutomaticThresholdFilter: Missing/Empty DataPaths", "[RobustAutomaticThresholdFilter]")
{
  UnitTest::LoadPlugins();

  RobustAutomaticThresholdFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  {
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_InputArrayPath_Key, std::make_any<DataPath>(inputPath));

  {
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_GradientMagnitudePath_Key, std::make_any<DataPath>(gradientMagnitudePath));

  {
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, 1);
    UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, std::vector<DataPath>{inputPath.replaceName("mask")});
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("StorageFormatPlan: threshold output policy uses output bytes", "[RobustAutomaticThresholdFilter][StorageFormatPlan][T23][T24]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::GetOrCreateInstance()->getIOCollection().addIOManager(std::make_shared<ThresholdPlanManager>()).valid());
  constexpr usize k_Tuples = 2ULL * 1024 * 1024 * 1024;
  DataStructure dataStructure;
  auto resolver = std::make_shared<ThresholdResolver>();
  dataStructure.setFormatResolver(resolver);
  auto inputResult = Float32Array::CreatePlanned(dataStructure, "Input", {k_Tuples}, {1}, ThresholdPlanManager::k_Format.str());
  SIMPLNX_RESULT_REQUIRE_VALID(inputResult);
  auto gradientResult = Float32Array::CreatePlanned(dataStructure, "Gradient", {k_Tuples}, {1}, ThresholdPlanManager::k_Format.str());
  SIMPLNX_RESULT_REQUIRE_VALID(gradientResult);

  RobustAutomaticThresholdFilter filter;
  Arguments args;
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_InputArrayPath_Key, std::make_any<DataPath>(DataPath({"Input"})));
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_GradientMagnitudePath_Key, std::make_any<DataPath>(DataPath({"Gradient"})));
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_ArrayCreationName_Key, std::make_any<std::string>("MaskLarge"));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  UnitTest::RequireAutomaticCreateArrayActions(preflight.outputActions, std::vector<DataPath>{DataPath({"MaskLarge"})});
  const auto* create = dynamic_cast<const CreateArrayAction*>(preflight.outputActions.value().actions.front().get());
  REQUIRE(create != nullptr);
  CHECK(create->type() == DataType::boolean);
  CHECK(create->dims() == ShapeType{k_Tuples});
  CHECK(create->componentDims() == ShapeType{1});
  CHECK(create->dataFormat().empty());
  auto applyResult = preflight.outputActions.value().applyAll(dataStructure, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(DataPath({"MaskLarge"})));
  const auto& mask = dataStructure.getDataRefAs<BoolArray>(DataPath({"MaskLarge"}));
  CHECK(mask.getStoreType() == IDataStore::StoreType::Empty);
  CHECK(mask.getDataFormat().empty());
  CHECK(mask.getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  CHECK(dataStructure.getDataRefAs<Float32Array>(DataPath({"Input"})).getDataFormat() == ThresholdPlanManager::k_Format.str());
  REQUIRE_FALSE(resolver->paths.empty());
  CHECK(resolver->paths.back() == DataPath({"MaskLarge"}));
  CHECK(resolver->bytes.back() == 2ULL * 1024 * 1024 * 1024);
}

TEST_CASE("StorageFormatPlan: output policy covers inverse size and forced modes", "[StorageFormatPlan][T23][T24]")
{
  REQUIRE(Application::GetOrCreateInstance()->getIOCollection().addIOManager(std::make_shared<ThresholdPlanManager>()).valid());
  DataStructure dataStructure;
  auto resolver = std::make_shared<ThresholdResolver>();
  dataStructure.setFormatResolver(resolver);

  resolver->mode = ThresholdResolver::Mode::Adaptive;
  auto smallResult = UInt8Array::CreatePlanned(dataStructure, "AdaptiveSmall", {1024}, {1}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(smallResult);
  CHECK(smallResult.value()->getDataFormat().empty());
  auto largeResult = UInt8Array::CreatePlanned(dataStructure, "AdaptiveLarge", {5ULL * 1024 * 1024 * 1024}, {1}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(largeResult);
  CHECK(largeResult.value()->getDataFormat() == ThresholdPlanManager::k_Format.str());

  resolver->mode = ThresholdResolver::Mode::ForceInCore;
  auto forcedMemory = UInt8Array::CreatePlanned(dataStructure, "ForcedMemory", {5ULL * 1024 * 1024 * 1024}, {1}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(forcedMemory);
  CHECK(forcedMemory.value()->getDataFormat().empty());

  resolver->mode = ThresholdResolver::Mode::ForceOutOfCore;
  auto forcedOoc = UInt8Array::CreatePlanned(dataStructure, "ForcedOoc", {1024}, {1}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(forcedOoc);
  CHECK(forcedOoc.value()->getDataFormat() == ThresholdPlanManager::k_Format.str());
}

TEST_CASE("SimplnxCore::RobustAutomaticThresholdFilter: Test Algorithm", "[RobustAutomaticThresholdFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  RobustAutomaticThresholdFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  args.insertOrAssign(RobustAutomaticThresholdFilter::k_InputArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_GradientMagnitudePath_Key, std::make_any<DataPath>(gradientMagnitudePath));
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_ArrayCreationName_Key, std::make_any<std::string>("Created Array"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath.replaceName("Created Array")));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RobustAutomaticThresholdFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RobustAutomaticThresholdFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RobustAutomaticThresholdFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RobustAutomaticThresholdFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<RobustAutomaticThresholdFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(RobustAutomaticThresholdFilter::k_InputArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RobustAutomaticThresholdFilter::k_GradientMagnitudePath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(RobustAutomaticThresholdFilter::k_ArrayCreationName_Key) == "TestArray");
    }
  }
}
