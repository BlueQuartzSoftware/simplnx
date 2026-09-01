#include "SimplnxCore/Filters/ComputeSurfaceFeaturesFilter.hpp"
#include "SimplnxCore/Filters/CreateImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/ReadRawBinaryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_SurfaceFeatures("Surface Features");

const DataPath k_FeatureGeometryPath({"Feature Geometry"});
const DataPath k_FeatureIDsPath = k_FeatureGeometryPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath("Feature IDs");
const DataPath k_CellFeatureAMPath = k_FeatureGeometryPath.createChildPath(Constants::k_CellFeatureData);
const DataPath k_SurfaceFeaturesExemplaryPath = k_CellFeatureAMPath.createChildPath("Surface Features Exemplary");
const DataPath k_SurfaceFeaturesArrayPath = k_CellFeatureAMPath.createChildPath(k_SurfaceFeatures);
const std::string k_FeatureIds2DFileName = "FindSurfaceFeaturesTest/FeatureIds_2D.raw";
const std::string k_SurfaceFeatures2DExemplaryFileName = "FindSurfaceFeaturesTest/SurfaceFeatures2D.raw";

template <typename T, typename K>
std::vector<K> ConvertVectorReverse(const std::vector<T>& input)
{
  std::vector<K> result;
  result.reserve(input.size());
  for(auto it = input.rbegin(); it != input.rend(); ++it)
  {
    result.emplace_back(static_cast<K>(*it));
  }
  return result;
}

void test_impl(UnitTest::AlgorithmTestScope& scope, const std::vector<uint64>& geometryDims, const std::string& featureIdsFileName, const std::string& exemplaryFileName)
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "FindSurfaceFeaturesTest.tar.gz", "ComputeSurfaceFeaturesTest");

  // Configure the filter arguments.
  ComputeSurfaceFeaturesFilter filter;
  DataStructure dataStructure;
  Arguments cigArgs;

  CreateImageGeometryFilter cigFilter;
  cigArgs.insertOrAssign(CreateImageGeometryFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  cigArgs.insertOrAssign(CreateImageGeometryFilter::k_Dimensions_Key, geometryDims);
  auto result = cigFilter.execute(dataStructure, cigArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result)

  ReadRawBinaryFilter rbrFilter;
  Arguments rbrArgs;
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, fs::path(unit_test::k_TestFilesDir.str()).append(featureIdsFileName));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, NumericType::int32);
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({ConvertVectorReverse<uint64, double>(geometryDims)}));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.0}}));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(dataStructure, rbrArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  // Change Feature 470 to 0
  REQUIRE(dataStructure.getDataAs<Int32Array>(k_FeatureIDsPath) != nullptr);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIDsPath));
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
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{796.0}}));
  rbrArgs.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_SurfaceFeaturesExemplaryPath));

  result = rbrFilter.execute(dataStructure, rbrArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result)

  Arguments args;
  args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_FeatureGeometryPath_Key, std::make_any<DataPath>(k_FeatureGeometryPath));
  args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));
  args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_SurfaceFeaturesArrayName_Key, std::make_any<std::string>(k_SurfaceFeaturesArrayPath.getTargetName()));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_SurfaceFeaturesArrayPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath));

  UInt8Array& surfaceFeatures = dataStructure.getDataRefAs<UInt8Array>(k_SurfaceFeaturesArrayPath);
  Int8Array& surfaceFeaturesExemplary = dataStructure.getDataRefAs<Int8Array>(k_SurfaceFeaturesExemplaryPath);
  REQUIRE(surfaceFeatures.getSize() == surfaceFeaturesExemplary.getSize());
  REQUIRE(surfaceFeatures.getSize() == 796);
  REQUIRE(surfaceFeaturesExemplary.getSize() == 796);

  REQUIRE(surfaceFeaturesExemplary[0] == 1); // This is due to a bug in legacy DREAM3D-NX that sets featureID 0 as a surface feature (DREAM3D issue #989)
  REQUIRE(surfaceFeatures[0] == false);      // This bug is fixed in simplnx.

  for(usize i = 1; i < surfaceFeatures.getSize(); i++)
  {
    INFO(fmt::format("i = {}", i));
    REQUIRE(static_cast<int8>(surfaceFeatures[i]) == surfaceFeaturesExemplary[i]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

constexpr usize k_BenchDim = 200;
constexpr usize k_BenchTotalVoxels = k_BenchDim * k_BenchDim * k_BenchDim;
constexpr usize k_BlockSize = 25;
constexpr usize k_BlocksPerDim = k_BenchDim / k_BlockSize;                                                 // 8
constexpr int32 k_NumBlockFeatures = static_cast<int32>(k_BlocksPerDim * k_BlocksPerDim * k_BlocksPerDim); // 512
const std::string k_BenchGeomName = "ImageGeom";

void BuildBlockFeatureIds(DataStructure& ds)
{
  const ShapeType cellTupleShape = {k_BenchDim, k_BenchDim, k_BenchDim};

  auto* imageGeom = ImageGeom::Create(ds, k_BenchGeomName);
  imageGeom->setDimensions({k_BenchDim, k_BenchDim, k_BenchDim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, Constants::k_CellData, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath featureIdsPath = DataPath({k_BenchGeomName, Constants::k_CellData, Constants::k_FeatureIds});
  auto store = DataStoreUtilities::CreateDataStore<int32>(ds, featureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = DataArray<int32>::Create(ds, Constants::k_FeatureIds, store, cellAM->getId());
  auto& fidsRef = featureIds->getDataStoreRef();
  const usize sliceSize = k_BenchDim * k_BenchDim;
  std::vector<int32> sliceBuffer(sliceSize);
  for(usize iz = 0; iz < k_BenchDim; iz++)
  {
    for(usize iy = 0; iy < k_BenchDim; iy++)
    {
      for(usize ix = 0; ix < k_BenchDim; ix++)
      {
        const usize bx = ix / k_BlockSize;
        const usize by = iy / k_BlockSize;
        const usize bz = iz / k_BlockSize;
        const int32 blockId = static_cast<int32>(bz * k_BlocksPerDim * k_BlocksPerDim + by * k_BlocksPerDim + bx + 1);
        sliceBuffer[iy * k_BenchDim + ix] = blockId;
      }
    }
    fidsRef.copyFromBuffer(iz * sliceSize, nonstd::span<const int32>(sliceBuffer.data(), sliceSize));
  }

  AttributeMatrix::Create(ds, Constants::k_CellFeatureData, {static_cast<usize>(k_NumBlockFeatures + 1)}, imageGeom->getId());
}
} // namespace

TEST_CASE("SimplnxCore::ComputeSurfaceFeaturesFilter: 3D", "[SimplnxCore][ComputeSurfaceFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  const std::string k_SurfaceFeaturesExemplar("SurfaceFeatures");
  DataPath computedSurfaceFeaturesPath = Constants::k_CellFeatureDataPath.createChildPath(k_SurfaceFeatures);

  {
    ComputeSurfaceFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_FeatureGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
    args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Constants::k_FeatureIdsArrayPath));
    args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Constants::k_CellFeatureDataPath));
    args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_SurfaceFeaturesArrayName_Key, std::make_any<std::string>(k_SurfaceFeatures));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(computedSurfaceFeaturesPath));

    DataPath exemplaryDataPath = Constants::k_CellFeatureDataPath.createChildPath(k_SurfaceFeaturesExemplar);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(exemplaryDataPath));
    const BoolArray& featureArrayExemplary = dataStructure.getDataRefAs<BoolArray>(exemplaryDataPath);

    const UInt8Array& createdFeatureArray = dataStructure.getDataRefAs<UInt8Array>(computedSurfaceFeaturesPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      uint8 value = featureArrayExemplary[i] ? 1 : 0;
      REQUIRE(value == createdFeatureArray[i]);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceFeaturesFilter: 2D(XY Plane)", "[SimplnxCore][ComputeSurfaceFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  test_impl(scope, std::vector<uint64>({100, 100, 1}), k_FeatureIds2DFileName, k_SurfaceFeatures2DExemplaryFileName);
}

TEST_CASE("SimplnxCore::ComputeSurfaceFeaturesFilter: 2D(XZ Plane)", "[SimplnxCore][ComputeSurfaceFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  test_impl(scope, std::vector<uint64>({100, 1, 100}), k_FeatureIds2DFileName, k_SurfaceFeatures2DExemplaryFileName);
}

TEST_CASE("SimplnxCore::ComputeSurfaceFeaturesFilter: 2D(YZ Plane)", "[SimplnxCore][ComputeSurfaceFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  test_impl(scope, std::vector<uint64>({1, 100, 100}), k_FeatureIds2DFileName, k_SurfaceFeatures2DExemplaryFileName);
}

TEST_CASE("SimplnxCore::ComputeSurfaceFeaturesFilter: Generate Large Test Dataset", "[SimplnxCore][ComputeSurfaceFeaturesFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  DataStructure buildDS;
  BuildBlockFeatureIds(buildDS);

  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data";
  fs::create_directories(outputDir);
  const auto outputFile = outputDir / "compute_surface_features_data.dream3d";
  UnitTest::WriteTestDataStructure(buildDS, outputFile);
}

TEST_CASE("SimplnxCore::ComputeSurfaceFeaturesFilter: 200x200x200 block features", "[SimplnxCore][ComputeSurfaceFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  BuildBlockFeatureIds(dataStructure);

  const DataPath benchGeomPath({k_BenchGeomName});
  const DataPath benchFeatureIdsPath = benchGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);
  const DataPath benchCellFeatureAMPath = benchGeomPath.createChildPath(Constants::k_CellFeatureData);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(benchFeatureIdsPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(benchFeatureIdsPath));

  {
    ComputeSurfaceFeaturesFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_FeatureGeometryPath_Key, std::make_any<DataPath>(benchGeomPath));
    args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(benchFeatureIdsPath));
    args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(benchCellFeatureAMPath));
    args.insertOrAssign(ComputeSurfaceFeaturesFilter::k_SurfaceFeaturesArrayName_Key, std::make_any<std::string>("SurfaceFeatures"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Verify: 512 features, 296 surface + 216 interior
  const DataPath surfaceFeaturesPath = benchCellFeatureAMPath.createChildPath("SurfaceFeatures");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(surfaceFeaturesPath));
  const auto& surfaceFeatures = dataStructure.getDataRefAs<UInt8Array>(surfaceFeaturesPath).getDataStoreRef();
  usize surfaceCount = 0;
  usize interiorCount = 0;
  for(usize f = 1; f <= static_cast<usize>(k_NumBlockFeatures); f++)
  {
    if(surfaceFeatures[f])
    {
      surfaceCount++;
    }
    else
    {
      interiorCount++;
    }
  }
  REQUIRE(surfaceCount > 0);
  REQUIRE(interiorCount > 0);
  // 8x8x8 grid: 6x6x6=216 interior, 512-216=296 surface
  REQUIRE(interiorCount == 216);
  REQUIRE(surfaceCount == 296);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
