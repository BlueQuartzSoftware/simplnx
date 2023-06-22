#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

#include "OrientationAnalysis/Filters/MergeColoniesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::MergeColoniesFilter: Valid Filter Execution", "[ComplexCore][MergeColoniesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  MergeColoniesFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(MergeColoniesFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(false));
  args.insertOrAssign(MergeColoniesFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_AxisTolerance_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(MergeColoniesFilter::k_AngleTolerance_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(MergeColoniesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_CellParentIdsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_NewCellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_FeatureParentIdsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_ActiveArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}
