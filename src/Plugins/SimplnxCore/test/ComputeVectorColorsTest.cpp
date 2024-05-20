#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ComputeVectorColorsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;

namespace
{
DataPath ebsdPath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData});
DataPath eulerAnglesPath = ebsdPath.createChildPath(Constants::k_EulerAngles);
const std::string k_VecColorsNX = "Vector Colors";
} // namespace

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: Valid Filter Execution", "[SimplnxCore][ComputeVectorColorsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "generate_vector_colors.tar.gz", "generate_vector_colors");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/generate_vector_colors/6_6_generate_vector_colors.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeVectorColorsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeVectorColorsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeVectorColorsFilter::k_VectorsArrayPath_Key, std::make_any<DataPath>(eulerAnglesPath));
    args.insertOrAssign(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key, std::make_any<std::string>(k_VecColorsNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<uint8>(dataStructure, ebsdPath.createChildPath("VectorColor"), ebsdPath.createChildPath(k_VecColorsNX));
}
