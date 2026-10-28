#include "SimplnxCore/Filters/SharedFeatureFaceFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <array>
#include <string>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace SharedFeatureFaceFilterTest
{
const std::string k_TriangleGeometryName = "TriangleDataContainer";
const std::string k_FaceLabelsName = "FaceLabels";
const std::string k_FaceFeatureName = "FaceFeatureData";
const std::string k_FaceAttributeMatrixName = "FaceData";
const std::string k_GrainBoundaryName = "GrainBoundaryData";

const std::string k_FeatureFaceIdsArrayName = "SharedFeatureFaceId [NX Computed]";
const std::string k_GrainBoundaryAttributeMatrixName = "SharedFeatureFace [NX Computed]";
const std::string k_FeatureNumTrianglesArrayName = "NumTriangles [NX Computed]";
const std::string k_FeatureFaceLabelsArrayName = "FaceLabels [NX Computed]";

const DataPath k_GeometryPath = DataPath({k_TriangleGeometryName});
const DataPath k_FeatureAttributeMatrixPath = k_GeometryPath.createChildPath(k_FaceFeatureName);
const DataPath k_FaceLabelsPath = k_GeometryPath.createChildPath(k_FaceAttributeMatrixName).createChildPath(k_FaceLabelsName);

} // namespace SharedFeatureFaceFilterTest

using namespace SharedFeatureFaceFilterTest;

TEST_CASE("SimplnxCore::SharedFeatureFaceFilter", "[SimplnxCore][SharedFeatureFaceFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "12_IN625_GBCD.tar.gz", "12_IN625_GBCD");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/12_IN625_GBCD/12_IN625_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_FaceLabelsPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(k_FaceLabelsPath));

  {
    // Instantiate the filter and an Arguments Object
    SharedFeatureFaceFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(SharedFeatureFaceFilter::k_TriGeometryDataPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(k_GeometryPath));
    args.insertOrAssign(SharedFeatureFaceFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    // Output Variables
    args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureFaceIdsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FeatureFaceIdsArrayName));
    args.insertOrAssign(SharedFeatureFaceFilter::k_GrainBoundaryAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_GrainBoundaryAttributeMatrixName));
    args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureNumTrianglesArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FeatureNumTrianglesArrayName));
    args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureFaceLabelsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FeatureFaceLabelsArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the results
  {
    const std::string kExemplarArrayName = "FeatureFaceId";
    const DataPath kExemplarArrayPath = k_GeometryPath.createChildPath(k_FaceAttributeMatrixName).createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_GeometryPath.createChildPath(k_FaceAttributeMatrixName).createChildPath(k_FeatureFaceIdsArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);
    scope.requireExpectedStore(kNxArray);

    UnitTest::CompareDataArrays<int32>(kExemplarsArray, kNxArray);
  }
  {
    const std::string kExemplarArrayName = "FaceLabels";
    const DataPath kExemplarArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryName).createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryAttributeMatrixName).createChildPath(k_FeatureFaceLabelsArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);
    scope.requireExpectedStore(kNxArray);

    UnitTest::CompareDataArrays<int32>(kExemplarsArray, kNxArray);
  }
  {
    const std::string kExemplarArrayName = "NumTriangles";
    const DataPath kExemplarArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryName).createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryAttributeMatrixName).createChildPath(k_FeatureNumTrianglesArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);
    scope.requireExpectedStore(kNxArray);

    UnitTest::CompareDataArrays<int32>(kExemplarsArray, kNxArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/shared_feature_faces.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SharedFeatureFaceFilter: hand-counted labels and reserved row", "[SimplnxCore][SharedFeatureFaceFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool realZeroPair = GENERATE(false, true);
  CAPTURE(scenario, realZeroPair);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath geometryPath({"Mesh"});
  const DataPath faceDataPath = geometryPath.createChildPath("FaceData");
  const DataPath labelsPath = faceDataPath.createChildPath("Labels");
  auto* geometryPtr = TriangleGeom::Create(dataStructure, "Mesh");
  REQUIRE(geometryPtr != nullptr);
  auto* faceDataPtr = AttributeMatrix::Create(dataStructure, "FaceData", {6}, geometryPtr->getId());
  REQUIRE(faceDataPtr != nullptr);
  geometryPtr->setFaceAttributeMatrix(*faceDataPtr);
  auto connectivityStore = DataStoreUtilities::CreateDataStore<IGeometry::MeshIndexType>(dataStructure, geometryPath.createChildPath("Faces"), {6}, {3});
  auto* connectivityPtr = DataArray<IGeometry::MeshIndexType>::Create(dataStructure, "Faces", connectivityStore, geometryPtr->getId());
  REQUIRE(connectivityPtr != nullptr);
  geometryPtr->setFaceList(*connectivityPtr);
  auto inputStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, labelsPath, {6}, {2});
  auto* inputPtr = Int32Array::Create(dataStructure, "Labels", inputStore, faceDataPtr->getId());
  REQUIRE(inputPtr != nullptr);
  const std::array<int32, 12> inputLabels{2, 1, 1, 2, 3, -1, 9, 4, 4, 9, 0, realZeroPair ? 0 : 5};
  REQUIRE(inputStore->copyFromBuffer(0, {inputLabels.data(), inputLabels.size()}).valid());
  scope.requireExpectedStore(*inputPtr);
  SharedFeatureFaceFilter filter;
  Arguments args;
  args.insertOrAssign(SharedFeatureFaceFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(SharedFeatureFaceFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(labelsPath));
  args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureFaceIdsArrayName_Key, std::make_any<std::string>("Ids"));
  args.insertOrAssign(SharedFeatureFaceFilter::k_GrainBoundaryAttributeMatrixName_Key, std::make_any<std::string>("FeatureData"));
  args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureNumTrianglesArrayName_Key, std::make_any<std::string>("Counts"));
  args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureFaceLabelsArrayName_Key, std::make_any<std::string>("Pairs"));
  args.insertOrAssign(SharedFeatureFaceFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
  const auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  const DataPath idsPath = faceDataPath.createChildPath("Ids");
  const DataPath pairsPath = geometryPath.createChildPath("FeatureData").createChildPath("Pairs");
  const DataPath countsPath = geometryPath.createChildPath("FeatureData").createChildPath("Counts");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(idsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(pairsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(idsPath);
  const auto& pairs = dataStructure.getDataRefAs<Int32Array>(pairsPath);
  const auto& counts = dataStructure.getDataRefAs<Int32Array>(countsPath);
  scope.requireExpectedStore(ids);
  scope.requireExpectedStore(pairs);
  scope.requireExpectedStore(counts);
  REQUIRE(ids.getSize() == 6);
  REQUIRE(pairs.getSize() == 10);
  REQUIRE(counts.getSize() == 5);
  std::array<int32, 6> actualIds{};
  std::array<int32, 10> actualPairs{};
  std::array<int32, 5> actualCounts{};
  REQUIRE(ids.getDataStoreRef().copyIntoBuffer(0, {actualIds.data(), actualIds.size()}).valid());
  REQUIRE(pairs.getDataStoreRef().copyIntoBuffer(0, {actualPairs.data(), actualPairs.size()}).valid());
  REQUIRE(counts.getDataStoreRef().copyIntoBuffer(0, {actualCounts.data(), actualCounts.size()}).valid());
  const std::array<int32, 6> expectedIds{1, 1, 2, 3, 3, 4};
  const std::array<int32, 10> expectedPairs{0, 0, 1, 2, -1, 3, 4, 9, 0, realZeroPair ? 0 : 5};
  const std::array<int32, 5> expectedCounts{realZeroPair ? 1 : 0, 2, 1, 2, 1};
  CHECK(actualIds == expectedIds);
  CHECK(actualPairs == expectedPairs);
  CHECK(actualCounts == expectedCounts);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
