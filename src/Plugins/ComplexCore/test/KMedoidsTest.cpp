#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/KMedoidsFilter.hpp"

using namespace complex;

TEST_CASE("ComplexCore::KMedoidsFilter: Valid Filter Execution", "[ComplexCore][KMedoidsFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  KMedoidsFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(KMedoidsFilter::k_InitClusters_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(KMedoidsFilter::k_DistanceMetric_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(KMedoidsFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(KMedoidsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(KMedoidsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(KMedoidsFilter::k_FeatureIdsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(KMedoidsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(KMedoidsFilter::k_MedoidsArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ComplexCore::KMedoidsFilter: InValid Filter Execution")
//{
//
// }
