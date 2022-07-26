/**
 * This file is auto generated from the original Generic/FindSurfaceFeatures
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[FindSurfaceFeatures][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/DynamicTableParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CreateImageGeometry.hpp"
#include "ComplexCore/Filters/FindSurfaceFeatures.hpp"
#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
const DataPath k_FeatureGeometryPath({"Feature Geometry"});
const DataPath k_FeatureIDsPath({"Feature IDs"});
const DataPath k_SurfaceFeaturesExemplaryPath({"Surface Features Exemplary"});
const DataPath k_SurfaceFeaturesArrayPath({"Surface Features"});
const std::string k_FeatureIdsFileName = "FeatureIds.raw";
const std::string k_FeatureIds2DFileName = "FeatureIds_2D.raw";
const std::string k_SurfaceFeatures2DExemplaryFileName = "SurfaceFeatures2D.raw";
const std::string k_SurfaceFeatures3DExemplaryFileName = "SurfaceFeatures3D.raw";

void test_impl(const std::vector<uint64>& geometryDims, const std::string& featureIdsFileName, usize featureIdsSize, const std::string& exemplaryFileName)
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindSurfaceFeatures filter;
  DataStructure ds;
  Arguments cigArgs;

  CreateImageGeometry cigFilter;
  cigArgs.insertOrAssign(CreateImageGeometry::k_GeometryDataPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  cigArgs.insertOrAssign(CreateImageGeometry::k_Dimensions_Key, geometryDims);

  auto result = cigFilter.execute(ds, cigArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  RawBinaryReaderFilter rbrFilter;
  Arguments rbrArgs;
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(featureIdsFileName));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int32);
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{static_cast<double>(featureIdsSize)}}, {}, {}}));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(ds, rbrArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  // Change Feature 470 to 0
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_FeatureIDsPath));
  Int32Array& featureIds = ds.getDataRefAs<Int32Array>(k_FeatureIDsPath);
  std::replace(featureIds.begin(), featureIds.end(), 470, 0);

  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(exemplaryFileName));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int8);
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{796}}, {}, {}}));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_SurfaceFeaturesExemplaryPath));

  result = rbrFilter.execute(ds, rbrArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  // Create default Parameters for the filter.
  Arguments args;
  args.insertOrAssign(FindSurfaceFeatures::k_FeatureGeometryPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  args.insertOrAssign(FindSurfaceFeatures::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));
  args.insertOrAssign(FindSurfaceFeatures::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_SurfaceFeaturesArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(ds.getDataRefAs<BoolArray>(k_SurfaceFeaturesArrayPath));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath));

  BoolArray& surfaceFeatures = ds.getDataRefAs<BoolArray>(k_SurfaceFeaturesArrayPath);
  Int8Array& surfaceFeaturesExemplary = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath);
  REQUIRE(surfaceFeatures.getSize() == surfaceFeaturesExemplary.getSize());
  REQUIRE(surfaceFeatures.getSize() == 796);
  REQUIRE(surfaceFeaturesExemplary.getSize() == 796);

  REQUIRE(surfaceFeaturesExemplary[0] == 1); // This is due to a bug in legacy DREAM.3D that sets featureID 0 as a surface feature (DREAM3D issue #989)
  REQUIRE(surfaceFeatures[0] == 0);          // This bug is fixed in complex.

  for(usize i = 1; i < surfaceFeatures.getSize(); i++)
  {
    INFO(fmt::format("i = {}", i));
    REQUIRE(surfaceFeatures[i] == surfaceFeaturesExemplary[i]);
  }
}
} // namespace

TEST_CASE("ComplexCore::FindSurfaceFeatures: Instantiation and Parameter Check", "[Core][FindSurfaceFeatures]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindSurfaceFeatures filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindSurfaceFeatures::k_FeatureGeometryPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindSurfaceFeatures::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindSurfaceFeatures::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
}

TEST_CASE("ComplexCore::FindSurfaceFeatures: Valid filter execution in 3D", "[Core][FindSurfaceFeatures]")
{
  test_impl(std::vector<uint64>({100, 100, 100}), k_FeatureIdsFileName, 1000000, k_SurfaceFeatures3DExemplaryFileName);
}

TEST_CASE("ComplexCore::FindSurfaceFeatures: Valid filter execution in 2D - XY Plane", "[Core][FindSurfaceFeatures]")
{
  test_impl(std::vector<uint64>({100, 100, 1}), k_FeatureIds2DFileName, 10000, k_SurfaceFeatures2DExemplaryFileName);
}

TEST_CASE("ComplexCore::FindSurfaceFeatures: Valid filter execution in 2D - XZ Plane", "[Core][FindSurfaceFeatures]")
{
  test_impl(std::vector<uint64>({100, 1, 100}), k_FeatureIds2DFileName, 10000, k_SurfaceFeatures2DExemplaryFileName);
}

TEST_CASE("ComplexCore::FindSurfaceFeatures: Valid filter execution in 2D - YZ Plane", "[Core][FindSurfaceFeatures]")
{
  test_impl(std::vector<uint64>({1, 100, 100}), k_FeatureIds2DFileName, 10000, k_SurfaceFeatures2DExemplaryFileName);
}
