#include "SimplnxCore/Filters/MinNeighbors.hpp"
#include "SimplnxCore/Filters/FindNeighbors.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{

const std::vector<int32> k_NumberElements = {
    0,    1858, 152,   31,   70,   4183, 691,  8359, 646,  38,   124,  1064,  25,    34,   640,  1361, 74,   4014,  4,    129,  952,  2623,  17,   667,   6,     724,  3709,  344,  1149, 432,  114,
    439,  25,   64,    591,  1,    191,  6752, 374,  29,   32,   2695, 28,    44,    88,   231,  22,   412,  62,    180,  2371, 681,  753,   286,  1818,  50,    744,  648,   2464, 150,  277,  1303,
    87,   5480, 110,   1560, 206,  54,   56,   340,  469,  464,  47,   24,    132,   165,  150,  2322, 3583, 30,    44,   744,  14,   1866,  847,  749,   411,   149,  3371,  13,   42,   9205, 917,
    33,   226,  21,    8408, 309,  216,  43,   269,  279,  138,  11,   148,   459,   198,  50,   836,  118,  39,    67,   930,  1972, 1080,  395,  40,    171,   65,   3019,  4457, 20,   2344, 12,
    3391, 91,   622,   25,   588,  1916, 67,   107,  3647, 9,    875,  3439,  1,     49,   5538, 1998, 1525, 416,   1818, 67,   546,  3,     302,  419,   47,    87,   77,    175,  647,  1316, 969,
    678,  3814, 334,   197,  318,  640,  36,   135,  581,  133,  1674, 4,     273,   139,  40,   3566, 1078, 112,   695,  35,   1094, 17,    1256, 7545,  4668,  34,   223,   1411, 1486, 6,    126,
    42,   566,  338,   971,  177,  620,  537,  354,  595,  5249, 2519, 174,   134,   1146, 2417, 8218, 3,    339,   840,  9,    2018, 756,   600,  837,   566,   346,  195,   274,  358,  19,   2023,
    2357, 84,   7033,  87,   24,   1954, 79,   1464, 569,  1259, 168,  16046, 17,    7,    341,  328,  7896, 391,   3,    257,  39,   567,   46,   90,    1202,  1577, 27,    361,  305,  499,  26,
    19,   8,    144,   121,  2745, 595,  350,  16,   854,  4451, 367,  200,   574,   11,   1,    2185, 964,  2046,  3892, 56,   1212, 164,   2,    206,   2814,  776,  122,   46,   1248, 945,  7,
    559,  2269, 21,    533,  59,   920,  106,  55,   786,  57,   157,  2401,  368,   985,  4036, 352,  571,  1555,  4957, 1358, 1319, 144,   437,  53,    600,   407,  16492, 8347, 32,   7906, 40,
    104,  8,    2165,  2439, 3674, 543,  1716, 4947, 1786, 2407, 1405, 1425,  8312,  88,   50,   186,  17,   83,    1131, 30,   1197, 135,   126,  421,   52,    828,  2,     38,   96,   337,  2318,
    233,  46,   414,   266,  3755, 2997, 230,  1390, 35,   71,   278,  864,   14401, 400,  129,  360,  778,  21568, 28,   74,   2447, 1139,  853,  607,   1571,  1195, 278,   116,  2422, 1087, 577,
    206,  38,   166,   3663, 298,  285,  113,  975,  1184, 620,  3689, 114,   54,    1986, 124,  683,  80,   166,   4496, 98,   750,  45,    267,  43,    210,   565,  418,   30,   159,  56,   55,
    2497, 767,  13891, 2389, 380,  72,   1888, 273,  563,  424,  1708, 14,    1320,  1,    1257, 148,  159,  303,   4,    467,  1855, 16821, 128,  178,   28,    2515, 7665,  491,  124,  364,  1813,
    700,  206,  8399,  2107, 103,  7453, 111,  2750, 1090, 151,  183,  30,    427,   711,  1658, 360,  162,  47,    27,   405,  981,  52,    357,  9,     799,   6,    1285,  421,  6202, 2410, 3307,
    740,  1542, 878,   2240, 479,  1025, 4164, 1341, 1968, 8213, 30,   4,     5990,  23,   817,  1577, 90,   11567, 83,   505,  1402, 61,    309,  2148,  1263,  2,    258,   2514, 406,  2,    175,
    460,  253,  376,   352,  110,  400,  88,   5772, 92,   702,  489,  36,    78,    60,   2626, 1104, 1306, 47,    273,  2288, 1486, 19411, 7,    137,   186,   427,  13,    102,  178,  2523, 13105,
    399,  462,  1,     222,  5163, 14,   19,   32,   15,   55,   56,   1089,  994,   260,  161,  1,    11,   1254,  1428, 1215, 809,  6856,  3344, 1363,  550,   23,   16,    564,  1,    1551, 24,
    137,  35,   1241,  1058, 666,  2150, 38,   80,   1138, 40,   931,  10096, 2,     408,  485,  710,  197,  461,   137,  920,  7162, 9,     383,  75,    18789, 386,  1626,  5814, 182,  316,  120,
    8659, 970,  262,   5,    38,   6947, 4206, 214,  5083, 35,   284,  3,     4,     89,   112,  34,   7910, 65,    40,   556,  981,  941,   937,  76,    312,   1070, 1650,  339,  113,  6319, 34,
    136,  65,   533,   64,   286,  36,   3,    15,   201,  2508, 763,  3773,  1685,  5195, 198,  323,  249,  36,    79,   271,  375,  3,     4,    619,   54,    3355, 65,    4738, 211,  114,  231,
    1619, 100,  567,   254,  20,   328,  37,   57,   230,  184,  398,  4,     1397,  124,  495,  6,    1,    2,     28,   9230, 33,   85,    2253, 899,   1,     361,  57,    1591, 1247, 5,    684,
    2099, 2,    341,   319,  40,   69,   1704, 4262, 453,  1791, 21,   11713, 352,   1486, 287,  120,  74,   2069,  809,  3708, 2613, 531,   1147, 15630, 235,   620,  28,    1258, 104,  590,  88,
    90,   626,  660,   1913, 521,  628,  234,  370,  229,  1236, 1257, 1033,  63,    97,   1782, 59,   1222, 123,   1,    1863, 951,  605,   108,  1091,  961,   325,  3151,  787,  513,  1966, 1307,
    86,   1741, 1049,  8,    92,   10,   1429, 745,  4807, 3676, 952,  1801,  183,   952,  1010, 1888, 1208, 14,    260,  43,   273,  2959,  187,  248,   22,    102,  1551,  523,  2986, 244,  8509,
    1417, 3294, 1410,  147,  5,    1787, 2368, 877,  2682, 880,  4646, 770,   4178,  72,   43,   211};

} // namespace

TEST_CASE("SimplnxCore::MinNeighbors", "[SimplnxCore][MinNeighbors]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1.tar.gz", "6_5_test_data_1");
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1/6_5_test_data_1.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataAttributeMatrix = smallIn100Group.createChildPath(k_CellData);
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath cellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
  std::string numNeighborName = "NumNeighbors2";
  std::string neighborListName = "NeighborList2";
  std::string sharedSurfaceAreaListName = "SharedSurfaceAreaList2";
  std::string boundaryCellsName = "BoundaryCells_computed";
  std::string surfaceFeaturesName = "SurfaceFeatures_computed";
  DataPath numNeighborPath = cellFeatureAttributeMatrixPath.createChildPath(numNeighborName);
  DataPath neighborListPath = cellFeatureAttributeMatrixPath.createChildPath(neighborListName);
  DataPath sharedSurfaceAreaListPath = cellFeatureAttributeMatrixPath.createChildPath(sharedSurfaceAreaListName);
  DataPath boundaryCellsPath = cellFeatureAttributeMatrixPath.createChildPath(boundaryCellsName);
  DataPath surfaceFeaturesPath = cellFeatureAttributeMatrixPath.createChildPath(surfaceFeaturesName);

  {
    FindNeighbors filter;
    Arguments args;

    args.insertOrAssign(FindNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(FindNeighbors::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(FindNeighbors::k_CellFeaturesPath_Key, std::make_any<DataPath>(cellFeatureAttributeMatrixPath));

    args.insertOrAssign(FindNeighbors::k_StoreBoundary_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindNeighbors::k_BoundaryCellsName_Key, std::make_any<std::string>(boundaryCellsName));

    args.insertOrAssign(FindNeighbors::k_StoreSurface_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindNeighbors::k_SurfaceFeaturesName_Key, std::make_any<std::string>(surfaceFeaturesName));

    args.insertOrAssign(FindNeighbors::k_NumNeighborsName_Key, std::make_any<std::string>(numNeighborName));
    args.insertOrAssign(FindNeighbors::k_NeighborListName_Key, std::make_any<std::string>(neighborListName));
    args.insertOrAssign(FindNeighbors::k_SharedSurfaceAreaName_Key, std::make_any<std::string>(sharedSurfaceAreaListName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    MinNeighbors filter;
    Arguments args;

    args.insertOrAssign(MinNeighbors::k_MinNumNeighbors_Key, std::make_any<uint64>(3));
    args.insertOrAssign(MinNeighbors::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
    // args.insertOrAssign(MinNeighbors::k_PhaseNumber_Key, std::make_any<uint64>(0));
    // args.insertOrAssign(MinNeighbors::k_FeaturePhases_Key, std::make_any<DataPath>(k_FeaturePhases));
    args.insertOrAssign(MinNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(MinNeighbors::k_CellDataAttributeMatrixPath_Key, std::make_any<DataPath>(cellDataAttributeMatrix));
    args.insertOrAssign(MinNeighbors::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(MinNeighbors::k_NumNeighborsPath_Key, std::make_any<DataPath>(numNeighborPath));
    // args.insertOrAssign(MinNeighbors::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // there currently is not an exemplar data set to compare against. Compare at
    // least the new number of features.

    DataPath numElementsPath = cellFeatureAttributeMatrixPath.createChildPath("NumElements");
    const Int32Array& createdFeatureArray = dataStructure.getDataRefAs<Int32Array>(numElementsPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == 791);

    for(usize i = 0; i < createdFeatureArray.getSize(); i++)
    {
      REQUIRE(k_NumberElements[i] == createdFeatureArray[i]);
    }
  }

  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/minimum_neighbors_test.dream3d", unit_test::k_BinaryTestOutputDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }
}

#if 0
TEST_CASE("SimplnxCore::MinNeighbors: Bad Phase Number", "[MinNeighbors]")
{
  MinNeighbors filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  DataPath k_ImageGeomPath({k_ImageGeomName});
  bool k_ApplyToSinglePhase = true;
  uint64 k_PhaseNumber = 500;
  DataPath k_FeatureIdsPath({k_ImageGeomName, k_CellDataAttributeMatrix, k_FeatureIdsName});
  DataPath k_FeaturePhases({k_ImageGeomName, k_CellDataAttributeMatrix, k_FeaturePhasesName});
  DataPath k_NumNeighbors({k_ImageGeomName, k_CellDataAttributeMatrix, k_NumNeighborsName});
  uint64 k_MinNumNeighbors = 1;
  std::vector<DataPath> k_VoxelArrays = k_VoxelArrayPaths;

  args.insertOrAssign(MinNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(MinNeighbors::k_ApplyToSinglePhase_Key, std::make_any<bool>(k_ApplyToSinglePhase));
  args.insertOrAssign(MinNeighbors::k_PhaseNumber_Key, std::make_any<uint64>(k_PhaseNumber));
  args.insertOrAssign(MinNeighbors::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(MinNeighbors::k_FeaturePhases_Key, std::make_any<DataPath>(k_FeaturePhases));
  args.insertOrAssign(MinNeighbors::k_NumNeighbors_Key, std::make_any<DataPath>(k_NumNeighbors));
  args.insertOrAssign(MinNeighbors::k_MinNumNeighbors_Key, std::make_any<uint64>(k_MinNumNeighbors));
  args.insertOrAssign(MinNeighbors::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
}

TEST_CASE("SimplnxCore::MinNeighbors: Phase Array", "[MinNeighbors]")
{
  MinNeighbors filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  DataPath k_ImageGeomPath({k_ImageGeomName});
  bool k_ApplyToSinglePhase = false;
  uint64 k_PhaseNumber = 500;
  DataPath k_FeatureIdsPath({k_ImageGeomName, k_CellDataAttributeMatrix, k_FeatureIdsName});
  DataPath k_FeaturePhases({k_ImageGeomName, k_CellDataAttributeMatrix, k_FeaturePhasesName});
  DataPath k_NumNeighbors({k_ImageGeomName, k_CellDataAttributeMatrix, k_NumNeighborsName});
  DataPath k_CellDataAttributeMatrixPath({k_ImageGeomName, k_CellDataAttributeMatrix});
  uint64 k_MinNumNeighbors = 1;
  std::vector<DataPath> k_VoxelArrays = k_VoxelArrayPaths;

  args.insertOrAssign(MinNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(MinNeighbors::k_ApplyToSinglePhase_Key, std::make_any<bool>(k_ApplyToSinglePhase));
  args.insertOrAssign(MinNeighbors::k_PhaseNumber_Key, std::make_any<uint64>(k_PhaseNumber));
  args.insertOrAssign(MinNeighbors::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(MinNeighbors::k_FeaturePhases_Key, std::make_any<DataPath>(k_FeaturePhases));
  args.insertOrAssign(MinNeighbors::k_NumNeighbors_Key, std::make_any<DataPath>(k_NumNeighbors));
  args.insertOrAssign(MinNeighbors::k_MinNumNeighbors_Key, std::make_any<uint64>(k_MinNumNeighbors));
  args.insertOrAssign(MinNeighbors::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));
  args.insertOrAssign(MinNeighbors::k_CellDataAttributeMatrix_Key, std::make_any<DataPath>(k_CellDataAttributeMatrixPath));
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
#endif
