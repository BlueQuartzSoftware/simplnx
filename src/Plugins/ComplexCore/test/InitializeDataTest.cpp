#include "ComplexCore/Filters/InitializeData.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
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
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(0)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_InitValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
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
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(0)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_InitValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
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
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(0)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_InitValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
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
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(KMeansFilter::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
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
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(KMeansFilter::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
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
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(KMeansFilter::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 7: Single Component Incremental-Subtraction Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(KMeansFilter::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 8: Multi Component Single-Incremental-Subtraction Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(KMeansFilter::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 9: Multi Component Multi-Incremental-Subtraction Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(KMeansFilter::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 10: Single Component Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(3)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_UseSeed_key, std::make_any<bool>(true));
    // ommit seed value to use default seed type
    // ommit seed array name since it will not be used
    args.insertOrAssign(KMeansFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));
    args.insertOrAssign(KMeansFilter::k_InitStartRange_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_InitEndRange_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 11: Multi Component Single-Value Standardized-Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(3)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_UseSeed_key, std::make_any<bool>(true));
    // ommit seed value to use default seed type
    // ommit seed array name since it will not be used
    args.insertOrAssign(KMeansFilter::k_StandardizeSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(KMeansFilter::k_InitStartRange_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_InitEndRange_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 12: Multi Component Single-Value Non-Standardized-Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(3)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_UseSeed_key, std::make_any<bool>(true));
    // ommit seed value to use default seed type
    // ommit seed array name since it will not be used
    args.insertOrAssign(KMeansFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));
    args.insertOrAssign(KMeansFilter::k_InitStartRange_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_InitEndRange_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 13: Multi Component Multi-Value Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(3)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_UseSeed_key, std::make_any<bool>(true));
    // ommit seed value to use default seed type
    // ommit seed array name since it will not be used
    args.insertOrAssign(KMeansFilter::k_StandardizeSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(KMeansFilter::k_InitStartRange_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_InitEndRange_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 14: Boolean Multi Component Single-Value Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(0)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_InitValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 15: Boolean Multi Component Incremental-Addition Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insertOrAssign(KMeansFilter::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 16: Boolean Multi Component Incremental-Subtraction Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(1)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_StartingFillValue_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_StepOperation_Key, std::make_any<uint64>(1));
    args.insertOrAssign(KMeansFilter::k_StepValue_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 17: Boolean Multi Component Standardized-Random-With-Range Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(3)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_UseSeed_key, std::make_any<bool>(true));
    // ommit seed value to use default seed type
    // ommit seed array name since it will not be used
    args.insertOrAssign(KMeansFilter::k_StandardizeSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(KMeansFilter::k_InitStartRange_Key, std::make_any<std::string>(""));
    args.insertOrAssign(KMeansFilter::k_InitEndRange_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 18: Single Component Random Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(2)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_UseSeed_key, std::make_any<bool>(true));
    // ommit seed value to use default seed type
    // ommit seed array name since it will not be used
    args.insertOrAssign(KMeansFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 19: Multi Component Standardized-Random Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(2)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_UseSeed_key, std::make_any<bool>(true));
    // ommit seed value to use default seed type
    // ommit seed array name since it will not be used
    args.insertOrAssign(KMeansFilter::k_StandardizeSeed_Key, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}

TEST_CASE("ComplexCore::InitializeData 20: Multi Component Non-Standardized-Random Initialization", "[ComplexCore][InitializeData]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/initialize_data_test_files/7_0_single_component_fill.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    InitializeData filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_ArrayPath_Key, std::make_any<DataPath>({}));
    args.insertOrAssign(KMeansFilter::k_InitType_Key, std::make_any<uint64>(2)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_UseSeed_key, std::make_any<bool>(true));
    // ommit seed value to use default seed type
    // ommit seed array name since it will not be used
    args.insertOrAssign(KMeansFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result  
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, generatedDatapath);
}
