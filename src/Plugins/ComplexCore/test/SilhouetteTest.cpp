#include <catch2/catch.hpp>

#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/SilhouetteFilter.hpp"

using namespace complex;

TEST_CASE("ComplexCore::SilhouetteFilter: Valid Filter Execution", "[ComplexCore][SilhouetteFilter]")
{
  DataStructure dataStructure;
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    SilhouetteFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(SilhouetteFilter::k_DistanceMetric_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(SilhouetteFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(DataPath{}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }
}
