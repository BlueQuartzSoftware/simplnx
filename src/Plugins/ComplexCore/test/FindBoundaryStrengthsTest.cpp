#include <catch2/catch.hpp>

#include "complex/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/FindBoundaryStrengthsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::FindBoundaryStrengthsFilter: Valid Filter Execution", "[OrientationAnalysis][FindBoundaryStrengthsFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindBoundaryStrengthsFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindBoundaryStrengthsFilter::k_Loading_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshF1sArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshF1sptsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshF7sArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshmPrimesArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());
}
