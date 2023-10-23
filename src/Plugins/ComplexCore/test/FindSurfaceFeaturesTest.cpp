#include "ComplexCore/Filters/FindSurfaceFeatures.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CreateImageGeometry.hpp"
#include "ComplexCore/Filters/ReadRawBinaryFilter.hpp"

#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
const std::string k_SurfaceFeatures("Surface Features");

const DataPath k_FeatureGeometryPath({"Feature Geometry"});
const DataPath k_FeatureIDsPath = k_FeatureGeometryPath.createChildPath(ImageGeom::k_CellDataName).createChildPath("Feature IDs");
const DataPath k_CellFeatureAMPath = k_FeatureGeometryPath.createChildPath(Constants::k_CellFeatureData);
const DataPath k_SurfaceFeaturesExemplaryPath = k_CellFeatureAMPath.createChildPath("Surface Features Exemplary");
const DataPath k_SurfaceFeaturesArrayPath = k_CellFeatureAMPath.createChildPath(k_SurfaceFeatures);
const std::string k_FeatureIds2DFileName = "FindSurfaceFeaturesTest/FeatureIds_2D.raw";
const std::string k_SurfaceFeatures2DExemplaryFileName = "FindSurfaceFeaturesTest/SurfaceFeatures2D.raw";

void test_impl(const std::vector<uint64>& geometryDims, const std::string& featureIdsFileName, usize featureIdsSize, const std::string& exemplaryFileName)
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "FindSurfaceFeaturesTest.tar.gz", "FindSurfaceFeaturesTest");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindSurfaceFeatures filter;
  DataStructure dataStructure;
  Arguments cigArgs;

  CreateImageGeometry cigFilter;
  cigArgs.insertOrAssign(CreateImageGeometry::k_GeometryDataPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  cigArgs.insertOrAssign(CreateImageGeometry::k_Dimensions_Key, geometryDims);
  auto result = cigFilter.execute(dataStructure, cigArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result)

  ReadRawBinaryFilter rbrFilter;
  Arguments rbrArgs;
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, fs::path(unit_test::k_TestFilesDir.str()).append(featureIdsFileName));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, NumericType::int32);
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{static_cast<double>(featureIdsSize)}}));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(dataStructure, rbrArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  // Change Feature 470 to 0
  REQUIRE(dataStructure.getDataAs<Int32Array>(k_FeatureIDsPath) != nullptr);
  Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIDsPath);
  std::replace(featureIds.begin(), featureIds.end(), 470, 0);

  // Create the cell feature attribute matrix
  usize featureIdsMaxIdx = std::distance(featureIds.begin(), std::max_element(featureIds.begin(), featureIds.end()));
  usize maxValue = featureIds[featureIdsMaxIdx];
  auto imageGeomId = dataStructure.getId(k_FeatureGeometryPath);
  REQUIRE(imageGeomId.has_value());
  AttributeMatrix::Create(dataStructure, Constants::k_CellFeatureData, std::vector<usize>{maxValue + 1}, imageGeomId.value());

  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, fs::path(unit_test::k_TestFilesDir.str()).append(exemplaryFileName));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, NumericType::int8);
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{796}}));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_SurfaceFeaturesExemplaryPath));

  result = rbrFilter.execute(dataStructure, rbrArgs);
  COMPLEX_RESULT_REQUIRE_VALID(result.result)

  // Create default Parameters for the filter.
  Arguments args;
  args.insertOrAssign(FindSurfaceFeatures::k_FeatureGeometryPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  args.insertOrAssign(FindSurfaceFeatures::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));
  args.insertOrAssign(FindSurfaceFeatures::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insertOrAssign(FindSurfaceFeatures::k_SurfaceFeaturesArrayPath_Key, std::make_any<std::string>(k_SurfaceFeaturesArrayPath.getTargetName()));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(k_SurfaceFeaturesArrayPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath));

  BoolArray& surfaceFeatures = dataStructure.getDataRefAs<BoolArray>(k_SurfaceFeaturesArrayPath);
  Int8Array& surfaceFeaturesExemplary = dataStructure.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath);
  REQUIRE(surfaceFeatures.getSize() == surfaceFeaturesExemplary.getSize());
  REQUIRE(surfaceFeatures.getSize() == 796);
  REQUIRE(surfaceFeaturesExemplary.getSize() == 796);

  REQUIRE(surfaceFeaturesExemplary[0] == 1); // This is due to a bug in legacy DREAM.3D that sets featureID 0 as a surface feature (DREAM3D issue #989)
  REQUIRE(surfaceFeatures[0] == false);      // This bug is fixed in complex.

  for(usize i = 1; i < surfaceFeatures.getSize(); i++)
  {
    INFO(fmt::format("i = {}", i));
    REQUIRE(static_cast<int8>(surfaceFeatures[i]) == surfaceFeaturesExemplary[i]);
  }
}
} // namespace

TEST_CASE("ComplexCore::FindSurfaceFeatures: Valid filter execution in 3D", "[ComplexCore][FindSurfaceFeatures]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_5_test_data_1.tar.gz", "6_5_test_data_1");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1/6_5_test_data_1.dream3d", complex::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  const std::string k_SurfaceFeaturesExemplar("SurfaceFeatures");
  DataPath computedSurfaceFeaturesPath = Constants::k_CellFeatureDataPath.createChildPath(k_SurfaceFeatures);

  {
    FindSurfaceFeatures filter;
    Arguments args;

    args.insertOrAssign(FindSurfaceFeatures::k_FeatureGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
    args.insertOrAssign(FindSurfaceFeatures::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Constants::k_FeatureIdsArrayPath));
    args.insertOrAssign(FindSurfaceFeatures::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Constants::k_CellFeatureDataPath));
    args.insertOrAssign(FindSurfaceFeatures::k_SurfaceFeaturesArrayPath_Key, std::make_any<std::string>(k_SurfaceFeatures));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(computedSurfaceFeaturesPath));

    DataPath exemplaryDataPath = Constants::k_CellFeatureDataPath.createChildPath(k_SurfaceFeaturesExemplar);
    const DataArray<bool>& featureArrayExemplary = dataStructure.getDataRefAs<DataArray<bool>>(exemplaryDataPath);

    const DataArray<bool>& createdFeatureArray = dataStructure.getDataRefAs<DataArray<bool>>(computedSurfaceFeaturesPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
    }
  }
}

TEST_CASE("ComplexCore::FindSurfaceFeatures: Valid filter execution in 2D - XY Plane", "[ComplexCore][FindSurfaceFeatures]")
{
  test_impl(std::vector<uint64>({100, 100, 1}), k_FeatureIds2DFileName, 10000, k_SurfaceFeatures2DExemplaryFileName);
}

TEST_CASE("ComplexCore::FindSurfaceFeatures: Valid filter execution in 2D - XZ Plane", "[ComplexCore][FindSurfaceFeatures]")
{
  test_impl(std::vector<uint64>({100, 1, 100}), k_FeatureIds2DFileName, 10000, k_SurfaceFeatures2DExemplaryFileName);
}

TEST_CASE("ComplexCore::FindSurfaceFeatures: Valid filter execution in 2D - YZ Plane", "[ComplexCore][FindSurfaceFeatures]")
{
  test_impl(std::vector<uint64>({1, 100, 100}), k_FeatureIds2DFileName, 10000, k_SurfaceFeatures2DExemplaryFileName);
}
