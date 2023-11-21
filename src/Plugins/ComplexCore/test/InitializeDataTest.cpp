#include "ComplexCore/Filters/InitializeData.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;

namespace
{
} // namespace

TEST_CASE("ComplexCore::InitializeData 1: Single Component Fill Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(0)); // Default Seed
    args.insertOrAssign(InitializeData::k_InitValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath{});
}

TEST_CASE("ComplexCore::InitializeData 2: Multi Component Single-Value Fill Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(0)); // Default Seed
    args.insertOrAssign(InitializeData::k_InitValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath{});
}

TEST_CASE("ComplexCore::InitializeData 3: Multi Component Multi-Value Fill Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(0)); // Default Seed
    args.insertOrAssign(InitializeData::k_InitValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath{});
}

TEST_CASE("ComplexCore::InitializeData 4: Single Component Incremental-Addition Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(InitializeData::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(InitializeData::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeData::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath{});
}

TEST_CASE("ComplexCore::InitializeData 5: Multi Component Single-Incremental-Addition Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(InitializeData::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(InitializeData::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeData::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath{});
}

TEST_CASE("ComplexCore::InitializeData 6: Multi Component Multi-Incremental-Addition Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(InitializeData::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(InitializeData::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeData::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath{});
}

TEST_CASE("ComplexCore::InitializeData 7: Single Component Incremental-Subtraction Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_inc_sub.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeData::k_StartingFillValue_Key, std::make_any<std::string>("0.567"));
    args.insertOrAssign(InitializeData::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeData::k_StepValue_Key, std::make_any<std::string>("1.43"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 8: Multi Component Single-Incremental-Subtraction Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_single_val_inc_sub.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeData::k_StartingFillValue_Key, std::make_any<std::string>("7"));
    args.insertOrAssign(InitializeData::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeData::k_StepValue_Key, std::make_any<std::string>("-1"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 9: Multi Component Multi-Value Incremental-Subtraction Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_multi_val_inc_sub.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeData::k_StartingFillValue_Key, std::make_any<std::string>("100;0;-1"));
    args.insertOrAssign(InitializeData::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeData::k_StepValue_Key, std::make_any<std::string>("2;16;-10"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 10: Single Component Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeData::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeData::k_SeedArrayName_Key, std::make_any<std::string>("InitializeData SeedValue Test"));
    args.insertOrAssign(InitializeData::k_StandardizeSeed_Key, std::make_any<bool>(false));
    args.insertOrAssign(InitializeData::k_InitStartRange_Key, std::make_any<std::string>("2.62"));
    args.insertOrAssign(InitializeData::k_InitEndRange_Key, std::make_any<std::string>("6666.66"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 11: Multi Component Single-Value Standardized Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_single_val_stand_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeData::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeData::k_SeedArrayName_Key, std::make_any<std::string>("InitializeData SeedValue Test"));
    args.insertOrAssign(InitializeData::k_StandardizeSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_InitStartRange_Key, std::make_any<std::string>("-6.283185")); // -2 pi
    args.insertOrAssign(InitializeData::k_InitEndRange_Key, std::make_any<std::string>("6.283185"));    // 2 pi

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 12: Multi Component Single-Value Non-Standardized Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_single_val_non_stand_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeData::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeData::k_SeedArrayName_Key, std::make_any<std::string>("InitializeData SeedValue Test"));
    args.insertOrAssign(InitializeData::k_StandardizeSeed_Key, std::make_any<bool>(false));
    args.insertOrAssign(InitializeData::k_InitStartRange_Key, std::make_any<std::string>("-1000"));
    args.insertOrAssign(InitializeData::k_InitEndRange_Key, std::make_any<std::string>("1000"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 13: Multi Component Multi-Value Non-Standardized Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_multi_val_non_stand_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeData::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeData::k_SeedArrayName_Key, std::make_any<std::string>("InitializeData SeedValue Test"));
    args.insertOrAssign(InitializeData::k_StandardizeSeed_Key, std::make_any<bool>(false));
    args.insertOrAssign(InitializeData::k_InitStartRange_Key, std::make_any<std::string>("-500;0;19"));
    args.insertOrAssign(InitializeData::k_InitEndRange_Key, std::make_any<std::string>("-1;0;1000"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 14: Boolean Multi Component Single-Value Fill Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_bool_single_val_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeData::k_InitValue_Key, std::make_any<std::string>("False"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<bool>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 15: Boolean Multi Component Incremental-Addition Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_bool_inc_addition.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeData::k_StartingFillValue_Key, std::make_any<std::string>("1;0;0"));
    args.insertOrAssign(InitializeData::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeData::k_StepValue_Key, std::make_any<std::string>("1;0;1"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<bool>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 16: Boolean Multi Component Incremental-Subtraction Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_bool_inc_subtraction.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeData::k_StartingFillValue_Key, std::make_any<std::string>("0;1;1"));
    args.insertOrAssign(InitializeData::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeData::k_StepValue_Key, std::make_any<std::string>("1;0;1"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<bool>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 17: Boolean Multi Component Standardized-Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_bool_stand_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeData::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeData::k_SeedArrayName_Key, std::make_any<std::string>("InitializeData SeedValue Test"));
    args.insertOrAssign(InitializeData::k_StandardizeSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_InitStartRange_Key, std::make_any<std::string>("0"));
    args.insertOrAssign(InitializeData::k_InitEndRange_Key, std::make_any<std::string>("1;0;1"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<bool>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 18: Single Component Random Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_rand.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(2));
    args.insertOrAssign(InitializeData::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeData::k_SeedArrayName_Key, std::make_any<std::string>("InitializeData SeedValue Test"));
    args.insertOrAssign(InitializeData::k_StandardizeSeed_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<uint8>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 19: Multi Component Standardized-Random Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_stand_rand.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(2));
    args.insertOrAssign(InitializeData::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeData::k_SeedArrayName_Key, std::make_any<std::string>("InitializeData SeedValue Test"));
    args.insertOrAssign(InitializeData::k_StandardizeSeed_Key, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<uint32>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 20: Multi Component Non-Standardized-Random Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_non_stand_rand.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(2));
    args.insertOrAssign(InitializeData::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeData::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeData::k_SeedArrayName_Key, std::make_any<std::string>("InitializeData SeedValue Test"));
    args.insertOrAssign(InitializeData::k_StandardizeSeed_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}

TEST_CASE("ComplexCore::InitializeData 21: Boolean Single Component Fill Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                             "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_bool_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeData::k_ArrayPath_Key, std::make_any<DataPath>(DataPath({"baseline"})));
    args.insertOrAssign(InitializeData::k_InitType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeData::k_InitValue_Key, std::make_any<std::string>("False"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<bool>(dataStructure, DataPath({"exemplar"}), DataPath({"baseline"}));
}
