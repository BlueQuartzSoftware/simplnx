#include "SimplnxCore/Filters/InitializeDataFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
const DataPath k_BaselinePath = DataPath({"baseline"});
const DataPath k_ExemplarPath = DataPath({"exemplar"});

template <typename T, bool Standardized = false>
void BoundsCheck(const DataArray<T>& dataArray, const std::vector<T>& compBounds)
{
  usize numTup = dataArray.getNumberOfTuples();
  usize numComp = dataArray.getNumberOfComponents();

  REQUIRE(compBounds.size() == numComp * 2);

  for(usize tup = 0; tup < numTup; tup++)
  {
    T currentComp = dataArray[tup * numComp];
    for(usize comp = 0; comp < numComp; comp++)
    {
      T value = dataArray[tup * numComp + comp];
      if constexpr(!std::is_same_v<T, bool>)
      {
        REQUIRE(value >= compBounds[comp * 2]);
        REQUIRE(value <= compBounds[comp * 2 + 1]);

        if(comp != 0)
        {
          if constexpr(Standardized)
          {
            REQUIRE(currentComp == value);
          }

          if constexpr(!Standardized)
          {
            REQUIRE(currentComp != value);
          }
        }
      }

      if constexpr(std::is_same_v<T, bool>)
      {
        REQUIRE((value == compBounds[comp * 2] || value == compBounds[comp * 2 + 1]));
      }
    }
  }
}

} // namespace

TEST_CASE("SimplnxCore::InitializeDataFilter 1: Single Component Fill Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeDataFilter::k_InitValue_Key, std::make_any<std::string>("-3.14"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 2: Multi Component Single-Value Fill Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_single_val_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeDataFilter::k_InitValue_Key, std::make_any<std::string>("53"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 3: Multi Component Multi-Value Fill Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_multi_val_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeDataFilter::k_InitValue_Key, std::make_any<std::string>("123;0;-38"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 4: Single Component Incremental-Addition Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_inc_add.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StartingFillValue_Key, std::make_any<std::string>("-2.09"));
    args.insertOrAssign(InitializeDataFilter::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeDataFilter::k_StepValue_Key, std::make_any<std::string>("10.67"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 5: Multi Component Single-Value Incremental-Addition Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_single_val_inc_add.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StartingFillValue_Key, std::make_any<std::string>("-126"));
    args.insertOrAssign(InitializeDataFilter::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeDataFilter::k_StepValue_Key, std::make_any<std::string>("43"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 6: Multi Component Multi-Value Incremental-Addition Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_multi_val_inc_add.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StartingFillValue_Key, std::make_any<std::string>("34;0;-71"));
    args.insertOrAssign(InitializeDataFilter::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeDataFilter::k_StepValue_Key, std::make_any<std::string>("-3;0;7"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 7: Single Component Incremental-Subtraction Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_inc_sub.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StartingFillValue_Key, std::make_any<std::string>("0.567"));
    args.insertOrAssign(InitializeDataFilter::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StepValue_Key, std::make_any<std::string>("1.43"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 8: Multi Component Single-Value Incremental-Subtraction Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_single_val_inc_sub.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StartingFillValue_Key, std::make_any<std::string>("7"));
    args.insertOrAssign(InitializeDataFilter::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StepValue_Key, std::make_any<std::string>("-1"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 9: Multi Component Multi-Value Incremental-Subtraction Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_multi_val_inc_sub.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StartingFillValue_Key, std::make_any<std::string>("100;0;-1"));
    args.insertOrAssign(InitializeDataFilter::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StepValue_Key, std::make_any<std::string>("2;16;-10"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 10: Single Component Random-With-Range Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeDataFilter::k_SeedArrayName_Key, std::make_any<std::string>("InitializeDataFilter SeedValue Test"));
    args.insertOrAssign(InitializeDataFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));
    args.insertOrAssign(InitializeDataFilter::k_InitStartRange_Key, std::make_any<std::string>("2.62"));
    args.insertOrAssign(InitializeDataFilter::k_InitEndRange_Key, std::make_any<std::string>("6666.66"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ::BoundsCheck<float32, false>(dataStructure.getDataRefAs<Float32Array>(::k_BaselinePath), {2.62f, 6666.66f});
}

TEST_CASE("SimplnxCore::InitializeDataFilter 11: Multi Component Single-Value Standardized Random-With-Range Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_single_val_stand_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeDataFilter::k_SeedArrayName_Key, std::make_any<std::string>("InitializeDataFilter SeedValue Test"));
    args.insertOrAssign(InitializeDataFilter::k_StandardizeSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_InitStartRange_Key, std::make_any<std::string>("-6.283185")); // -2 pi
    args.insertOrAssign(InitializeDataFilter::k_InitEndRange_Key, std::make_any<std::string>("6.283185"));    // 2 pi

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ::BoundsCheck<float32, true>(dataStructure.getDataRefAs<Float32Array>(::k_BaselinePath), {-6.283185f, 6.283185f, -6.28318f, 6.283185f, -6.28318f, 6.283185f});
}

TEST_CASE("SimplnxCore::InitializeDataFilter 12: Multi Component Single-Value Non-Standardized Random-With-Range Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_single_val_non_stand_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeDataFilter::k_SeedArrayName_Key, std::make_any<std::string>("InitializeDataFilter SeedValue Test"));
    args.insertOrAssign(InitializeDataFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));
    args.insertOrAssign(InitializeDataFilter::k_InitStartRange_Key, std::make_any<std::string>("-1000"));
    args.insertOrAssign(InitializeDataFilter::k_InitEndRange_Key, std::make_any<std::string>("1000"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ::BoundsCheck<int32, false>(dataStructure.getDataRefAs<Int32Array>(::k_BaselinePath), {-1000, 1000, -1000, 1000, -1000, 1000});
}

TEST_CASE("SimplnxCore::InitializeDataFilter 13: Multi Component Multi-Value Non-Standardized Random-With-Range Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_multi_val_non_stand_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeDataFilter::k_SeedArrayName_Key, std::make_any<std::string>("InitializeDataFilter SeedValue Test"));
    args.insertOrAssign(InitializeDataFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));
    args.insertOrAssign(InitializeDataFilter::k_InitStartRange_Key, std::make_any<std::string>("-500;0;19"));
    args.insertOrAssign(InitializeDataFilter::k_InitEndRange_Key, std::make_any<std::string>("-1;0;1000"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ::BoundsCheck<int32, false>(dataStructure.getDataRefAs<Int32Array>(::k_BaselinePath), {-500, -1, 0, 0, 19, 1000});
}

TEST_CASE("SimplnxCore::InitializeDataFilter 14: Boolean Multi Component Single-Value Fill Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_bool_single_val_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeDataFilter::k_InitValue_Key, std::make_any<std::string>("False"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<bool>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 15: Boolean Multi Component Incremental-Addition Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_bool_inc_addition.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StartingFillValue_Key, std::make_any<std::string>("1;0;0"));
    args.insertOrAssign(InitializeDataFilter::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeDataFilter::k_StepValue_Key, std::make_any<std::string>("1;0;1"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<bool>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 16: Boolean Multi Component Incremental-Subtraction Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_bool_inc_subtraction.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StartingFillValue_Key, std::make_any<std::string>("0;1;1"));
    args.insertOrAssign(InitializeDataFilter::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(InitializeDataFilter::k_StepValue_Key, std::make_any<std::string>("1;0;1"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<bool>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}

TEST_CASE("SimplnxCore::InitializeDataFilter 17: Boolean Multi Component Standardized Random-With-Range Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_bool_stand_rwr.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(3));
    args.insertOrAssign(InitializeDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeDataFilter::k_SeedArrayName_Key, std::make_any<std::string>("InitializeDataFilter SeedValue Test"));
    args.insertOrAssign(InitializeDataFilter::k_StandardizeSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_InitStartRange_Key, std::make_any<std::string>("0"));
    args.insertOrAssign(InitializeDataFilter::k_InitEndRange_Key, std::make_any<std::string>("1;0;1"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ::BoundsCheck<bool, true>(dataStructure.getDataRefAs<BoolArray>(::k_BaselinePath), {false, true, false, false, false, true});
}

TEST_CASE("SimplnxCore::InitializeDataFilter 18: Single Component Random Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_rand.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(2));
    args.insertOrAssign(InitializeDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeDataFilter::k_SeedArrayName_Key, std::make_any<std::string>("InitializeDataFilter SeedValue Test"));
    args.insertOrAssign(InitializeDataFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ::BoundsCheck<uint8, false>(dataStructure.getDataRefAs<UInt8Array>(::k_BaselinePath), {std::numeric_limits<uint8>::min(), std::numeric_limits<uint8>::max()});
}

TEST_CASE("SimplnxCore::InitializeDataFilter 19: Multi Component Standardized-Random Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_stand_rand.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(2));
    args.insertOrAssign(InitializeDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeDataFilter::k_SeedArrayName_Key, std::make_any<std::string>("InitializeDataFilter SeedValue Test"));
    args.insertOrAssign(InitializeDataFilter::k_StandardizeSeed_Key, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ::BoundsCheck<uint32, true>(dataStructure.getDataRefAs<UInt32Array>(::k_BaselinePath), {std::numeric_limits<uint32>::min(), std::numeric_limits<uint32>::max(), std::numeric_limits<uint32>::min(),
                                                                                          std::numeric_limits<uint32>::max(), std::numeric_limits<uint32>::min(), std::numeric_limits<uint32>::max()});
}

TEST_CASE("SimplnxCore::InitializeDataFilter 20: Multi Component Non-Standardized-Random Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_multi_comp_non_stand_rand.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(2));
    args.insertOrAssign(InitializeDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(InitializeDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(InitializeDataFilter::k_SeedArrayName_Key, std::make_any<std::string>("InitializeDataFilter SeedValue Test"));
    args.insertOrAssign(InitializeDataFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ::BoundsCheck<float32, false>(dataStructure.getDataRefAs<Float32Array>(::k_BaselinePath),
                                {(-1 * (std::numeric_limits<float32>::max() - 1)), std::numeric_limits<float32>::max(), (-1 * (std::numeric_limits<float32>::max() - 1)),
                                 std::numeric_limits<float32>::max(), (-1 * (std::numeric_limits<float32>::max() - 1)), std::numeric_limits<float32>::max()});
}

TEST_CASE("SimplnxCore::InitializeDataFilter 21: Boolean Single Component Fill Initialization", "[SimplnxCore][InitializeDataFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "initialize_data_test_files.tar.gz",
                                                              "initialize_data_test_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_comp_bool_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(InitializeDataFilter::k_ArrayPath_Key, std::make_any<DataPath>(::k_BaselinePath));
    args.insertOrAssign(InitializeDataFilter::k_InitType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(InitializeDataFilter::k_InitValue_Key, std::make_any<std::string>("False"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<bool>(dataStructure, ::k_ExemplarPath, ::k_BaselinePath);
}
