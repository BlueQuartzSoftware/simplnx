#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/ComputeMomentInvariants2D.hpp"
#include "SimplnxCore/Filters/ComputeMomentInvariants2DFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
// -----------------------------------------------------------------------------
DataStructure CreateInvalidTestData()
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeometry);
  constexpr size_t dimsIn[3] = {5, 5, 5};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  std::vector<size_t> dims(3, 0);
  dims[0] = 5;
  dims[1] = 5;
  dims[2] = 5;
  AttributeMatrix* cellAm = AttributeMatrix::Create(dataStructure, k_CellData, dims, imageGeom->getId());

  Int32Array* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIds, dims, std::vector<usize>{1}, cellAm->getId());
  featureIds->fill(0);
  (*featureIds)[6] = 1;
  (*featureIds)[7] = 1;
  (*featureIds)[8] = 1;
  (*featureIds)[11] = 1;
  (*featureIds)[12] = 1;
  (*featureIds)[13] = 1;
  (*featureIds)[16] = 1;
  (*featureIds)[17] = 1;
  (*featureIds)[18] = 1;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAm = AttributeMatrix::Create(dataStructure, k_FeatureData, dims, imageGeom->getId());

#if 0
      0, 0, 0, 0, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 0, 0, 0, 0;
#endif
  const std::vector<size_t> compDims(1, 6);
  UInt32Array* rect = UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, k_RectCoords, dims, compDims, featureAm->getId());
  rect->fill(0);
  (*rect)[6] = 1;
  (*rect)[7] = 1;
  (*rect)[8] = 0;
  (*rect)[9] = 3;
  (*rect)[10] = 3;
  (*rect)[11] = 0;

  return dataStructure;
}

// -----------------------------------------------------------------------------
DataStructure CreateTestData()
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeometry);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;
  AttributeMatrix* cellAm = AttributeMatrix::Create(dataStructure, k_CellData, dims, imageGeom->getId());

  Int32Array* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIds, dims, std::vector<usize>{1}, cellAm->getId());
  featureIds->fill(0);
  (*featureIds)[6] = 1;
  (*featureIds)[7] = 1;
  (*featureIds)[8] = 1;
  (*featureIds)[11] = 1;
  (*featureIds)[12] = 1;
  (*featureIds)[13] = 1;
  (*featureIds)[16] = 1;
  (*featureIds)[17] = 1;
  (*featureIds)[18] = 1;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAm = AttributeMatrix::Create(dataStructure, k_FeatureData, dims, imageGeom->getId());

#if 0
      0, 0, 0, 0, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 0, 0, 0, 0;
#endif
  const std::vector<size_t> compDims(1, 6);
  UInt32Array* rect = UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, k_RectCoords, dims, compDims, featureAm->getId());
  rect->fill(0);
  (*rect)[6] = 1;
  (*rect)[7] = 1;
  (*rect)[8] = 0;
  (*rect)[9] = 3;
  (*rect)[10] = 3;
  (*rect)[11] = 0;

  return dataStructure;
}

const DataPath k_Omega1Path({k_ImageGeometry, k_FeatureData, k_Omega1});
const DataPath k_Omega2Path({k_ImageGeometry, k_FeatureData, k_Omega2});
} // namespace

TEST_CASE("SimplnxCore::ComputeMomentInvariants2DFilter: Valid Filter Execution", "[SimplnxCore][ComputeMomentInvariants2DFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeMomentInvariants2DFilter filter;
  DataStructure ds = CreateTestData();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_CellData, k_FeatureIds})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureRectArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_FeatureData, k_RectCoords})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_NormalizeMomentInvariants_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_FeatureData})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega1ArrayName_Key, std::make_any<std::string>(k_Omega1));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega2ArrayName_Key, std::make_any<std::string>(k_Omega2));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_SaveCentralMoments_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_CentralMomentsArrayName_Key, std::make_any<std::string>(k_CentralMoments));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const Float32Array* omega1 = ds.getDataAs<Float32Array>(k_Omega1Path);
  const Float32Array* omega2 = ds.getDataAs<Float32Array>(k_Omega2Path);
  REQUIRE(omega1 != nullptr);
  REQUIRE(omega2 != nullptr);
  float32 value1 = (*omega1)[1];
  float32 value2 = (*omega2)[1];
  REQUIRE(std::fabs(value1 - 0.95493) < 0.00001);
  REQUIRE(std::fabs(value2 - 0.911891) < 0.00001);
}

TEST_CASE("SimplnxCore::ComputeMomentInvariants2DFilter: InValid Filter Execution", "[SimplnxCore][ComputeMomentInvariants2DFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeMomentInvariants2DFilter filter;
  DataStructure ds = CreateInvalidTestData(); // create invalid data structure input with 3D Image geometry
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_CellData, k_FeatureIds})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureRectArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_FeatureData, k_RectCoords})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_NormalizeMomentInvariants_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_FeatureData})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega1ArrayName_Key, std::make_any<std::string>(k_Omega1));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega2ArrayName_Key, std::make_any<std::string>(k_Omega2));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_SaveCentralMoments_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_CentralMomentsArrayName_Key, std::make_any<std::string>(k_CentralMoments));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
