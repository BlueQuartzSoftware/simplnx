#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindFeaturePhasesBinaryFilter.hpp"

using namespace complex;

namespace
{
const std::string k_BinaryFeaturePhasesName = "BinaryFeaturePhases";
}

TEST_CASE("ComplexCore::FindFeaturePhasesBinaryFilter: Valid Filter Execution", "[ComplexCore][FindFeaturePhasesBinaryFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindFeaturePhasesBinaryFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindFeaturePhasesBinaryFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindFeaturePhasesBinaryFilter::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindFeaturePhasesBinaryFilter::k_CellDataAMPath_Key, std::make_any<DataPath>(DataPath{}));

  args.insertOrAssign(FindFeaturePhasesBinaryFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_BinaryFeaturePhasesName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());
}
