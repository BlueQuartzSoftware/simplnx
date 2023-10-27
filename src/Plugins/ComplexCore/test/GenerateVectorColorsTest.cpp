#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/GenerateVectorColorsFilter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

namespace
{
DataPath ebsdPath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData});
DataPath eulerAnglesPath = ebsdPath.createChildPath(Constants::k_EulerAngles);
const std::string k_VecColorsNX = "Vector Colors";
} // namespace

TEST_CASE("ComplexCore::GenerateVectorColorsFilter: Valid Filter Execution", "[ComplexCore][GenerateVectorColorsFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "generate_vector_colors.tar.gz", "generate_vector_colors");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/generate_vector_colors/6_6_generate_vector_colors.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    GenerateVectorColorsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(GenerateVectorColorsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(GenerateVectorColorsFilter::k_VectorsArrayPath_Key, std::make_any<DataPath>(eulerAnglesPath));
    args.insertOrAssign(GenerateVectorColorsFilter::k_CellVectorColorsArrayName_Key, std::make_any<std::string>(k_VecColorsNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<uint8>(dataStructure, ebsdPath.createChildPath("VectorColor"), ebsdPath.createChildPath(k_VecColorsNX));
}
