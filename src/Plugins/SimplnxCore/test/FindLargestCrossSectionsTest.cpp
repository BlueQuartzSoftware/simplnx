#include <catch2/catch.hpp>

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

#include "SimplnxCore/Filters/FindLargestCrossSectionsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_LargestCrossSections = "LargestCrossSections";

DataStructure CreateValidTestDataStructure()
{
  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, k_ImageGeometry);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({.25, .25, .25});
  std::vector<usize> dims = {6, 6, 6};
  imageGeom->setDimensions(dims);

  auto* cellAM = AttributeMatrix::Create(ds, k_CellData, dims, imageGeom->getId());
  auto* cellFeatureAM = AttributeMatrix::Create(ds, k_CellFeatureData, {6}, imageGeom->getId());

  auto* featureIds = Int32Array::CreateWithStore<Int32DataStore>(ds, k_FeatureIds, dims, {1}, cellAM->getId());
  auto& featureIdsDataStore = featureIds->getDataStoreRef();
  featureIdsDataStore[0] = 2;
  featureIdsDataStore[1] = 2;
  featureIdsDataStore[2] = 2;
  featureIdsDataStore[3] = 3;
  featureIdsDataStore[4] = 0;
  featureIdsDataStore[5] = 0;
  featureIdsDataStore[6] = 1;
  featureIdsDataStore[7] = 2;
  featureIdsDataStore[8] = 0;
  featureIdsDataStore[9] = 0;
  featureIdsDataStore[10] = 5;
  featureIdsDataStore[11] = 5;
  featureIdsDataStore[12] = 1;
  featureIdsDataStore[13] = 1;
  featureIdsDataStore[14] = 5;
  featureIdsDataStore[15] = 5;
  featureIdsDataStore[16] = 5;
  featureIdsDataStore[17] = 5;
  featureIdsDataStore[18] = 1;
  featureIdsDataStore[19] = 1;
  featureIdsDataStore[20] = 1;
  featureIdsDataStore[21] = 5;
  featureIdsDataStore[22] = 5;
  featureIdsDataStore[23] = 5;
  featureIdsDataStore[24] = 1;
  featureIdsDataStore[25] = 1;
  featureIdsDataStore[26] = 5;
  featureIdsDataStore[27] = 5;
  featureIdsDataStore[28] = 5;
  featureIdsDataStore[29] = 5;
  featureIdsDataStore[30] = 1;
  featureIdsDataStore[31] = 1;
  featureIdsDataStore[32] = 5;
  featureIdsDataStore[33] = 5;
  featureIdsDataStore[34] = 5;
  featureIdsDataStore[35] = 5;
  featureIdsDataStore[36] = 2;
  featureIdsDataStore[37] = 2;
  featureIdsDataStore[38] = 2;
  featureIdsDataStore[39] = 3;
  featureIdsDataStore[40] = 5;
  featureIdsDataStore[41] = 5;
  featureIdsDataStore[42] = 2;
  featureIdsDataStore[43] = 2;
  featureIdsDataStore[44] = 0;
  featureIdsDataStore[45] = 5;
  featureIdsDataStore[46] = 5;
  featureIdsDataStore[47] = 5;
  featureIdsDataStore[48] = 1;
  featureIdsDataStore[49] = 1;
  featureIdsDataStore[50] = 5;
  featureIdsDataStore[51] = 5;
  featureIdsDataStore[52] = 5;
  featureIdsDataStore[53] = 5;
  featureIdsDataStore[54] = 1;
  featureIdsDataStore[55] = 1;
  featureIdsDataStore[56] = 5;
  featureIdsDataStore[57] = 5;
  featureIdsDataStore[58] = 5;
  featureIdsDataStore[59] = 5;
  featureIdsDataStore[60] = 1;
  featureIdsDataStore[61] = 1;
  featureIdsDataStore[62] = 5;
  featureIdsDataStore[63] = 5;
  featureIdsDataStore[64] = 5;
  featureIdsDataStore[65] = 5;
  featureIdsDataStore[66] = 1;
  featureIdsDataStore[67] = 1;
  featureIdsDataStore[68] = 5;
  featureIdsDataStore[69] = 5;
  featureIdsDataStore[70] = 5;
  featureIdsDataStore[71] = 5;
  featureIdsDataStore[72] = 2;
  featureIdsDataStore[73] = 2;
  featureIdsDataStore[74] = 0;
  featureIdsDataStore[75] = 3;
  featureIdsDataStore[76] = 5;
  featureIdsDataStore[77] = 5;
  featureIdsDataStore[78] = 2;
  featureIdsDataStore[79] = 2;
  featureIdsDataStore[80] = 2;
  featureIdsDataStore[81] = 5;
  featureIdsDataStore[82] = 5;
  featureIdsDataStore[83] = 5;
  featureIdsDataStore[84] = 2;
  featureIdsDataStore[85] = 2;
  featureIdsDataStore[86] = 5;
  featureIdsDataStore[87] = 5;
  featureIdsDataStore[88] = 5;
  featureIdsDataStore[89] = 5;
  featureIdsDataStore[90] = 2;
  featureIdsDataStore[91] = 2;
  featureIdsDataStore[92] = 5;
  featureIdsDataStore[93] = 5;
  featureIdsDataStore[94] = 5;
  featureIdsDataStore[95] = 5;
  featureIdsDataStore[96] = 2;
  featureIdsDataStore[97] = 2;
  featureIdsDataStore[98] = 5;
  featureIdsDataStore[99] = 5;
  featureIdsDataStore[100] = 5;
  featureIdsDataStore[101] = 5;
  featureIdsDataStore[102] = 1;
  featureIdsDataStore[103] = 5;
  featureIdsDataStore[104] = 5;
  featureIdsDataStore[105] = 5;
  featureIdsDataStore[106] = 5;
  featureIdsDataStore[107] = 5;
  featureIdsDataStore[108] = 2;
  featureIdsDataStore[109] = 2;
  featureIdsDataStore[110] = 2;
  featureIdsDataStore[111] = 0;
  featureIdsDataStore[112] = 5;
  featureIdsDataStore[113] = 5;
  featureIdsDataStore[114] = 2;
  featureIdsDataStore[115] = 2;
  featureIdsDataStore[116] = 2;
  featureIdsDataStore[117] = 5;
  featureIdsDataStore[118] = 5;
  featureIdsDataStore[119] = 5;
  featureIdsDataStore[120] = 2;
  featureIdsDataStore[121] = 2;
  featureIdsDataStore[122] = 2;
  featureIdsDataStore[123] = 5;
  featureIdsDataStore[124] = 5;
  featureIdsDataStore[125] = 5;
  featureIdsDataStore[126] = 2;
  featureIdsDataStore[127] = 2;
  featureIdsDataStore[128] = 5;
  featureIdsDataStore[129] = 5;
  featureIdsDataStore[130] = 5;
  featureIdsDataStore[131] = 5;
  featureIdsDataStore[132] = 2;
  featureIdsDataStore[133] = 2;
  featureIdsDataStore[134] = 5;
  featureIdsDataStore[135] = 5;
  featureIdsDataStore[136] = 5;
  featureIdsDataStore[137] = 5;
  featureIdsDataStore[138] = 1;
  featureIdsDataStore[139] = 5;
  featureIdsDataStore[140] = 5;
  featureIdsDataStore[141] = 5;
  featureIdsDataStore[142] = 5;
  featureIdsDataStore[143] = 5;
  featureIdsDataStore[144] = 2;
  featureIdsDataStore[145] = 2;
  featureIdsDataStore[146] = 2;
  featureIdsDataStore[147] = 2;
  featureIdsDataStore[148] = 5;
  featureIdsDataStore[149] = 5;
  featureIdsDataStore[150] = 2;
  featureIdsDataStore[151] = 2;
  featureIdsDataStore[152] = 2;
  featureIdsDataStore[153] = 5;
  featureIdsDataStore[154] = 5;
  featureIdsDataStore[155] = 5;
  featureIdsDataStore[156] = 2;
  featureIdsDataStore[157] = 2;
  featureIdsDataStore[158] = 2;
  featureIdsDataStore[159] = 5;
  featureIdsDataStore[160] = 5;
  featureIdsDataStore[161] = 5;
  featureIdsDataStore[162] = 2;
  featureIdsDataStore[163] = 2;
  featureIdsDataStore[164] = 5;
  featureIdsDataStore[165] = 5;
  featureIdsDataStore[166] = 5;
  featureIdsDataStore[167] = 5;
  featureIdsDataStore[168] = 2;
  featureIdsDataStore[169] = 2;
  featureIdsDataStore[170] = 5;
  featureIdsDataStore[171] = 5;
  featureIdsDataStore[172] = 5;
  featureIdsDataStore[173] = 5;
  featureIdsDataStore[174] = 1;
  featureIdsDataStore[175] = 0;
  featureIdsDataStore[176] = 5;
  featureIdsDataStore[177] = 5;
  featureIdsDataStore[178] = 5;
  featureIdsDataStore[179] = 5;
  featureIdsDataStore[180] = 2;
  featureIdsDataStore[181] = 2;
  featureIdsDataStore[182] = 2;
  featureIdsDataStore[183] = 2;
  featureIdsDataStore[184] = 5;
  featureIdsDataStore[185] = 5;
  featureIdsDataStore[186] = 2;
  featureIdsDataStore[187] = 2;
  featureIdsDataStore[188] = 2;
  featureIdsDataStore[189] = 2;
  featureIdsDataStore[190] = 5;
  featureIdsDataStore[191] = 5;
  featureIdsDataStore[192] = 2;
  featureIdsDataStore[193] = 2;
  featureIdsDataStore[194] = 2;
  featureIdsDataStore[195] = 5;
  featureIdsDataStore[196] = 5;
  featureIdsDataStore[197] = 5;
  featureIdsDataStore[198] = 2;
  featureIdsDataStore[199] = 2;
  featureIdsDataStore[200] = 5;
  featureIdsDataStore[201] = 5;
  featureIdsDataStore[202] = 5;
  featureIdsDataStore[203] = 5;
  featureIdsDataStore[204] = 2;
  featureIdsDataStore[205] = 4;
  featureIdsDataStore[206] = 5;
  featureIdsDataStore[207] = 5;
  featureIdsDataStore[208] = 5;
  featureIdsDataStore[209] = 5;
  featureIdsDataStore[210] = 4;
  featureIdsDataStore[211] = 4;
  featureIdsDataStore[212] = 5;
  featureIdsDataStore[213] = 5;
  featureIdsDataStore[214] = 5;
  featureIdsDataStore[215] = 5;

  return ds;
}

/**
 * @brief
 *  Create a Cell and Cell Feature attribute Matrix. The Cell AttributeMatrix will have a
 *  featureIds DataArray that has values of 10. The Cell Feature AttributeMatrix will have
 *  only 6 tuples in it. This means that the maximum value of FeatureIds would be 5. By having
 *  values in the Feature Ids = 10, the preflight would pass but the execute would fail.
 * @param geomIs3d
 * @return
 */
DataStructure CreateInvalidTestDataStructure(bool geomIs3d)
{
  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, k_ImageGeometry);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({.25, .25, .25});
  std::vector<usize> dims = {6, 6, 1};
  if(geomIs3d)
  {
    dims[2] = 6;
  }
  imageGeom->setDimensions(dims);

  auto* cellAM = AttributeMatrix::Create(ds, k_CellData, dims, imageGeom->getId());
  auto* cellFeatureAM = AttributeMatrix::Create(ds, k_CellFeatureData, {6}, imageGeom->getId());
  auto* testAM = AttributeMatrix::Create(ds, k_CellEnsembleData, {2}, imageGeom->getId());

  auto* featureIds = Int32Array::CreateWithStore<Int32DataStore>(ds, k_FeatureIds, dims, {1}, cellAM->getId());
  auto& featureIdsDataStore = featureIds->getDataStoreRef();
  featureIdsDataStore.fill(10);
  return ds;
}
} // namespace

TEST_CASE("SimplnxCore::FindLargestCrossSectionsFilter: Valid Filter Execution", "[SimplnxCore][FindLargestCrossSectionsFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindLargestCrossSectionsFilter filter;
  DataStructure ds = CreateValidTestDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindLargestCrossSectionsFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(FindLargestCrossSectionsFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));
  args.insertOrAssign(FindLargestCrossSectionsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_CellData, k_FeatureIds})));
  args.insertOrAssign(FindLargestCrossSectionsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_CellFeatureData})));
  args.insertOrAssign(FindLargestCrossSectionsFilter::k_LargestCrossSectionsArrayName_Key, std::make_any<std::string>(k_LargestCrossSections));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* largestCrossSection = ds.getDataAs<Float32Array>(DataPath({k_ImageGeometry, k_CellFeatureData, k_LargestCrossSections}));
  REQUIRE(largestCrossSection != nullptr);
  auto crossSection0 = (*largestCrossSection)[0];
  auto crossSection1 = (*largestCrossSection)[1];
  auto crossSection2 = (*largestCrossSection)[2];
  auto crossSection3 = (*largestCrossSection)[3];
  auto crossSection4 = (*largestCrossSection)[4];
  auto crossSection5 = (*largestCrossSection)[5];
  REQUIRE(std::fabs(crossSection0 - 0.0f) < UnitTest::EPSILON);
  REQUIRE(std::fabs(crossSection1 - 0.625f) < UnitTest::EPSILON);
  REQUIRE(std::fabs(crossSection2 - 0.875f) < UnitTest::EPSILON);
  REQUIRE(std::fabs(crossSection3 - 0.0625f) < UnitTest::EPSILON);
  REQUIRE(std::fabs(crossSection4 - 0.1875f) < UnitTest::EPSILON);
  REQUIRE(std::fabs(crossSection5 - 1.375f) < UnitTest::EPSILON);
}

TEST_CASE("SimplnxCore::FindLargestCrossSectionsFilter: InValid Filter Execution")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindLargestCrossSectionsFilter filter;
  Arguments args;
  args.insertOrAssign(FindLargestCrossSectionsFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(FindLargestCrossSectionsFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));
  args.insertOrAssign(FindLargestCrossSectionsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_CellData, k_FeatureIds})));
  args.insertOrAssign(FindLargestCrossSectionsFilter::k_LargestCrossSectionsArrayName_Key, std::make_any<std::string>(k_LargestCrossSections));

  SECTION("Invalid Image Geometry (should be 3D)")
  {
    DataStructure ds = CreateInvalidTestDataStructure(false);
    args.insertOrAssign(FindLargestCrossSectionsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_CellFeatureData})));
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  }
  SECTION("Invalid Cell Feature Attribute Matrix")
  {
    DataStructure ds = CreateInvalidTestDataStructure(true);
    args.insertOrAssign(FindLargestCrossSectionsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_CellEnsembleData})));
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  }
}
