#include "SimplnxCore/Filters/ComputeDifferencesMapFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

TEST_CASE("SimplnxCore::ComputeDifferencesMapFilter: Instantiate Filter", "[ComputeDifferencesMapFilter]")
{
  UnitTest::LoadPlugins();

  ComputeDifferencesMapFilter filter;
  DataStructure dataStructure;
  Arguments args;

  DataPath firstInputPath;
  DataPath secondInputPath;
  DataPath createdArrayPath;

  args.insertOrAssign(ComputeDifferencesMapFilter::k_FirstInputArrayPath_Key, std::make_any<DataPath>(firstInputPath));
  args.insertOrAssign(ComputeDifferencesMapFilter::k_SecondInputArrayPath_Key, std::make_any<DataPath>(secondInputPath));
  args.insertOrAssign(ComputeDifferencesMapFilter::k_DifferenceMapArrayPath_Key, std::make_any<DataPath>(createdArrayPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(!executeResult.result.valid());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeDifferencesMapFilter: Test Algorithm", "[ComputeDifferencesMapFilter]")
{
  UnitTest::LoadPlugins();

  ComputeDifferencesMapFilter filter;
  DataStructure dataStructure;
  Arguments args;

  DataPath firstInputPath({"First Input"});
  DataPath secondInputPath({"Second Input"});
  DataPath createdArrayPath({"Created Array"});
  auto* firstInputArray = UnitTest::CreateTestDataArray<int32>(dataStructure, firstInputPath.getTargetName(), {4}, {3});
  auto* secondInputArray = UnitTest::CreateTestDataArray<int32>(dataStructure, secondInputPath.getTargetName(), {4}, {3});

  args.insertOrAssign(ComputeDifferencesMapFilter::k_FirstInputArrayPath_Key, std::make_any<DataPath>(firstInputPath));
  {

    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  args.insertOrAssign(ComputeDifferencesMapFilter::k_SecondInputArrayPath_Key, std::make_any<DataPath>(secondInputPath));

  {
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  args.insertOrAssign(ComputeDifferencesMapFilter::k_DifferenceMapArrayPath_Key, std::make_any<DataPath>(createdArrayPath));
  {

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, 1);
    UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, std::vector<DataPath>{createdArrayPath});
  }

  const usize numValues = firstInputArray->getSize();
  for(usize index = 0; index < numValues; index++)
  {
    (*firstInputArray)[index] = static_cast<int32>(index % 7) - 3;
    (*secondInputArray)[index] = static_cast<int32>(index % 5) - 2;
  }

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(createdArrayPath));
  const auto& differenceMap = dataStructure.getDataRefAs<Int32Array>(createdArrayPath);
  REQUIRE(differenceMap.getSize() == numValues);
  for(usize index = 0; index < numValues; index++)
  {
    const int32 firstValue = (*firstInputArray)[index];
    const int32 secondValue = (*secondInputArray)[index];
    const int32 expected = firstValue > secondValue ? firstValue - secondValue : secondValue - firstValue;
    REQUIRE(differenceMap[index] == expected);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeDifferencesMapFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeDifferencesMapFilter][BackwardsCompatibility]")
{
  const auto version = GENERATE("6_4", "6_5");
  CAPTURE(version);
  const fs::path fixturePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion" / version / "ComputeDifferencesMapFilter.json";
  const auto args = UnitTest::LoadSIMPLConversionArguments(fixturePath, FilterTraits<ComputeDifferencesMapFilter>::uuid);
  CHECK(args.value<DataPath>(ComputeDifferencesMapFilter::k_FirstInputArrayPath_Key) == DataPath({"Scan", "Cells", "Before"}));
  CHECK(args.value<DataPath>(ComputeDifferencesMapFilter::k_SecondInputArrayPath_Key) == DataPath({"Scan", "Cells", "After"}));
  CHECK(args.value<DataPath>(ComputeDifferencesMapFilter::k_DifferenceMapArrayPath_Key) == DataPath({"Scan", "Cells", "Changes"}));
}
