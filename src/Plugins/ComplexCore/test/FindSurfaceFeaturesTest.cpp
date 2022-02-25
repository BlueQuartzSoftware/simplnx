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

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
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
const std::string k_SurfaceFeatures1DExemplaryFileName = "SurfaceFeatures1D.raw";
const std::string k_SurfaceFeatures2DXYExemplaryFileName = "SurfaceFeatures2D_XY.raw";
const std::string k_SurfaceFeatures2DXZExemplaryFileName = "SurfaceFeatures2D_XZ.raw";
const std::string k_SurfaceFeatures2DYZExemplaryFileName = "SurfaceFeatures2D_YZ.raw";
const std::string k_SurfaceFeatures3DExemplaryFileName = "SurfaceFeatures3D.raw";
} // namespace

TEST_CASE("Core::FindSurfaceFeatures: Instantiation and Parameter Check", "[Core][FindSurfaceFeatures]")
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

TEST_CASE("Core::FindSurfaceFeatures: Valid filter execution in 3D", "[Core][FindSurfaceFeatures]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindSurfaceFeatures filter;
  DataStructure ds;
  Arguments cigArgs;

  CreateImageGeometry cigFilter;
  cigArgs.insertOrAssign(CreateImageGeometry::k_GeometryDataPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  cigArgs.insertOrAssign(CreateImageGeometry::k_Dimensions_Key, std::vector<uint64>({100, 100, 100}));

  auto result = cigFilter.execute(ds, cigArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  RawBinaryReaderFilter rbrFilter;
  Arguments rbrArgs;
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_FeatureIdsFileName));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int32);
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{1000000}}, {}, {}}));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(ds, rbrArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_SurfaceFeatures3DExemplaryFileName));
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

  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath));

  Int8Array& surfaceFeatures = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath);
  Int8Array& surfaceFeaturesExemplary = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath);
  REQUIRE(surfaceFeatures.getSize() == surfaceFeaturesExemplary.getSize());
  REQUIRE(surfaceFeatures.getSize() == 796);
  REQUIRE(surfaceFeaturesExemplary.getSize() == 796);

  for(usize i = 0; i < surfaceFeatures.getSize(); i++)
  {
    REQUIRE(surfaceFeatures[i] == surfaceFeaturesExemplary[i]);
  }
}

TEST_CASE("Core::FindSurfaceFeatures: Valid filter execution in 2D - XY Plane", "[Core][FindSurfaceFeatures]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindSurfaceFeatures filter;
  DataStructure ds;
  Arguments cigArgs;

  CreateImageGeometry cigFilter;
  cigArgs.insertOrAssign(CreateImageGeometry::k_GeometryDataPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  cigArgs.insertOrAssign(CreateImageGeometry::k_Dimensions_Key, std::vector<uint64>({1000, 1000, 1}));

  auto result = cigFilter.execute(ds, cigArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  RawBinaryReaderFilter rbrFilter;
  Arguments rbrArgs;
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_FeatureIdsFileName));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int32);
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{1000000}}, {}, {}}));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(ds, rbrArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_SurfaceFeatures2DXYExemplaryFileName));
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

  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath));

  Int8Array& surfaceFeatures = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath);
  Int8Array& surfaceFeaturesExemplary = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath);
  REQUIRE(surfaceFeatures.getSize() == surfaceFeaturesExemplary.getSize());
  REQUIRE(surfaceFeatures.getSize() == 796);
  REQUIRE(surfaceFeaturesExemplary.getSize() == 796);

  for(usize i = 0; i < surfaceFeatures.getSize(); i++)
  {
    REQUIRE(surfaceFeatures[i] == surfaceFeaturesExemplary[i]);
  }
}

TEST_CASE("Core::FindSurfaceFeatures: Valid filter execution in 2D - XZ Plane", "[Core][FindSurfaceFeatures]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindSurfaceFeatures filter;
  DataStructure ds;
  Arguments cigArgs;

  CreateImageGeometry cigFilter;
  cigArgs.insertOrAssign(CreateImageGeometry::k_GeometryDataPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  cigArgs.insertOrAssign(CreateImageGeometry::k_Dimensions_Key, std::vector<uint64>({10000, 1, 100}));

  auto result = cigFilter.execute(ds, cigArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  RawBinaryReaderFilter rbrFilter;
  Arguments rbrArgs;
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_FeatureIdsFileName));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int32);
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{1000000}}, {}, {}}));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(ds, rbrArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_SurfaceFeatures2DXZExemplaryFileName));
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

  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath));

  Int8Array& surfaceFeatures = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath);
  Int8Array& surfaceFeaturesExemplary = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath);
  REQUIRE(surfaceFeatures.getSize() == surfaceFeaturesExemplary.getSize());
  REQUIRE(surfaceFeatures.getSize() == 796);
  REQUIRE(surfaceFeaturesExemplary.getSize() == 796);

  for(usize i = 0; i < surfaceFeatures.getSize(); i++)
  {
    REQUIRE(surfaceFeatures[i] == surfaceFeaturesExemplary[i]);
  }
}

TEST_CASE("Core::FindSurfaceFeatures: Valid filter execution in 2D - YZ Plane", "[Core][FindSurfaceFeatures]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindSurfaceFeatures filter;
  DataStructure ds;
  Arguments cigArgs;

  CreateImageGeometry cigFilter;
  cigArgs.insertOrAssign(CreateImageGeometry::k_GeometryDataPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  cigArgs.insertOrAssign(CreateImageGeometry::k_Dimensions_Key, std::vector<uint64>({1, 10, 100000}));

  auto result = cigFilter.execute(ds, cigArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  RawBinaryReaderFilter rbrFilter;
  Arguments rbrArgs;
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_FeatureIdsFileName));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int32);
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{1000000}}, {}, {}}));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(ds, rbrArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_SurfaceFeatures2DYZExemplaryFileName));
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

  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath));

  Int8Array& surfaceFeatures = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath);
  Int8Array& surfaceFeaturesExemplary = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath);
  REQUIRE(surfaceFeatures.getSize() == surfaceFeaturesExemplary.getSize());
  REQUIRE(surfaceFeatures.getSize() == 796);
  REQUIRE(surfaceFeaturesExemplary.getSize() == 796);

  for(usize i = 0; i < surfaceFeatures.getSize(); i++)
  {
    REQUIRE(surfaceFeatures[i] == surfaceFeaturesExemplary[i]);
  }
}

TEST_CASE("Core::FindSurfaceFeatures: Valid filter execution in 1D", "[Core][FindSurfaceFeatures]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindSurfaceFeatures filter;
  DataStructure ds;
  Arguments cigArgs;

  CreateImageGeometry cigFilter;
  cigArgs.insertOrAssign(CreateImageGeometry::k_GeometryDataPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  cigArgs.insertOrAssign(CreateImageGeometry::k_Dimensions_Key, std::vector<uint64>({1, 1000000, 1}));

  auto result = cigFilter.execute(ds, cigArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  RawBinaryReaderFilter rbrFilter;
  Arguments rbrArgs;
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_FeatureIdsFileName));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int32);
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{1000000}}, {}, {}}));
  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(ds, rbrArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  rbrArgs.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_SurfaceFeatures1DExemplaryFileName));
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

  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath));

  Int8Array& surfaceFeatures = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesArrayPath);
  Int8Array& surfaceFeaturesExemplary = ds.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath);
  REQUIRE(surfaceFeatures.getSize() == surfaceFeaturesExemplary.getSize());
  REQUIRE(surfaceFeatures.getSize() == 796);
  REQUIRE(surfaceFeaturesExemplary.getSize() == 796);

  for(usize i = 0; i < surfaceFeatures.getSize(); i++)
  {
    REQUIRE(surfaceFeatures[i] == surfaceFeaturesExemplary[i]);
  }
}
