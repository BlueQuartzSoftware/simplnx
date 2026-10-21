#include "SimplnxCore/Filters/SharedFeatureFaceFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <utility>
#include <vector>

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

/**
 * @brief Builds one disjoint triangle per label pair, runs the filter without randomization, and checks every output.
 * @param pairs Face label pair for each triangle.
 * @param expectedIds Expected shared feature face ID for each triangle.
 * @param expectedLabels Expected label pair for each boundary row, including the reserved row 0.
 * @param expectedCounts Expected triangle count for each boundary row, including the reserved row 0.
 */
void ExecuteAndCheckLabelPairs(const std::vector<std::pair<int32, int32>>& pairs, const std::vector<int32>& expectedIds, const std::vector<std::pair<int32, int32>>& expectedLabels,
                               const std::vector<int32>& expectedCounts)
{
  DataStructure ds;
  auto* meshGeomPtr = TriangleGeom::Create(ds, "Mesh");
  REQUIRE(meshGeomPtr != nullptr);
  auto* verticesPtr = UnitTest::CreateTestDataArray<float32>(ds, "Vertices", {pairs.size() * 3}, {3}, meshGeomPtr->getId());
  auto* trianglesPtr = UnitTest::CreateTestDataArray<uint64>(ds, "Triangles", {pairs.size()}, {3}, meshGeomPtr->getId());
  auto* faceDataPtr = AttributeMatrix::Create(ds, "Faces", ShapeType{pairs.size()}, meshGeomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(trianglesPtr != nullptr);
  REQUIRE(faceDataPtr != nullptr);
  meshGeomPtr->setVertices(*verticesPtr);
  meshGeomPtr->setFaceList(*trianglesPtr);
  meshGeomPtr->setFaceDataId(faceDataPtr->getId());
  auto* labelsPtr = UnitTest::CreateTestDataArray<int32>(ds, "Labels", {pairs.size()}, {2}, faceDataPtr->getId());
  REQUIRE(labelsPtr != nullptr);
  for(usize tupleIdx = 0; tupleIdx < pairs.size(); ++tupleIdx)
  {
    const auto x = static_cast<float32>(2 * tupleIdx);
    const std::array<float32, 9> xyz{x, 0, 0, x + 1, 0, 0, x, 1, 0};
    for(usize compIdx = 0; compIdx < xyz.size(); ++compIdx)
    {
      (*verticesPtr)[9 * tupleIdx + compIdx] = xyz[compIdx];
    }
    for(usize compIdx = 0; compIdx < 3; ++compIdx)
    {
      (*trianglesPtr)[3 * tupleIdx + compIdx] = 3 * tupleIdx + compIdx;
    }
    (*labelsPtr)[2 * tupleIdx] = pairs[tupleIdx].first;
    (*labelsPtr)[2 * tupleIdx + 1] = pairs[tupleIdx].second;
  }
  SharedFeatureFaceFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SharedFeatureFaceFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"Mesh"})));
  args.insertOrAssign(SharedFeatureFaceFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath({"Mesh", "Faces", "Labels"})));
  args.insertOrAssign(SharedFeatureFaceFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureFaceIdsArrayName_Key, std::make_any<std::string>("Ids"));
  args.insertOrAssign(SharedFeatureFaceFilter::k_GrainBoundaryAttributeMatrixName_Key, std::make_any<std::string>("Boundaries"));
  args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureFaceLabelsArrayName_Key, std::make_any<std::string>("Labels"));
  args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureNumTrianglesArrayName_Key, std::make_any<std::string>("Counts"));
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Faces", "Ids"})));
  const auto& idsRef = ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Faces", "Ids"}));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Boundaries", "Labels"})));
  const auto& outLabelsRef = ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Boundaries", "Labels"}));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Boundaries", "Counts"})));
  const auto& countsRef = ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Boundaries", "Counts"}));
  REQUIRE(idsRef.getNumberOfTuples() == expectedIds.size());
  REQUIRE(outLabelsRef.getNumberOfTuples() == expectedLabels.size());
  REQUIRE(countsRef.getNumberOfTuples() == expectedCounts.size());
  for(usize tupleIdx = 0; tupleIdx < expectedIds.size(); ++tupleIdx)
  {
    CHECK(idsRef[tupleIdx] == expectedIds[tupleIdx]);
  }
  for(usize row = 0; row < expectedLabels.size(); ++row)
  {
    CHECK(outLabelsRef[2 * row] == expectedLabels[row].first);
    CHECK(outLabelsRef[2 * row + 1] == expectedLabels[row].second);
    CHECK(countsRef[row] == expectedCounts[row]);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

} // namespace SharedFeatureFaceFilterTest

using namespace SharedFeatureFaceFilterTest;

TEST_CASE("SimplnxCore::SharedFeatureFaceFilter", "[SimplnxCore][SharedFeatureFaceFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "12_IN625_GBCD.tar.gz", "12_IN625_GBCD");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/12_IN625_GBCD/12_IN625_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

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
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the results
  {
    const std::string kExemplarArrayName = "FeatureFaceId";
    const DataPath kExemplarArrayPath = k_GeometryPath.createChildPath(k_FaceAttributeMatrixName).createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_GeometryPath.createChildPath(k_FaceAttributeMatrixName).createChildPath(k_FeatureFaceIdsArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);

    UnitTest::CompareDataArrays<int32>(kExemplarsArray, kNxArray);
  }
  {
    const std::string kExemplarArrayName = "FaceLabels";
    const DataPath kExemplarArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryName).createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryAttributeMatrixName).createChildPath(k_FeatureFaceLabelsArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);

    UnitTest::CompareDataArrays<int32>(kExemplarsArray, kNxArray);
  }
  {
    const std::string kExemplarArrayName = "NumTriangles";
    const DataPath kExemplarArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryName).createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryAttributeMatrixName).createChildPath(k_FeatureNumTrianglesArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);

    UnitTest::CompareDataArrays<int32>(kExemplarsArray, kNxArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/shared_feature_faces.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SharedFeatureFace: Missing zero pair rotation race", "[SimplnxCore][SharedFeatureFaceFilter][Regression]")
{
  UnitTest::LoadPlugins();
  // The first distinct pairs create map keys 2 and 1. Key zero is absent, so the reserved row must receive
  // count zero without changing the shared map.
  const std::vector<std::pair<int32, int32>> pairs{{0, 2}, {0, 2}, {0, 1}};
  const std::vector<int32> expectedIds{1, 1, 2};
  const std::vector<std::pair<int32, int32>> expectedLabels{{0, 0}, {0, 2}, {0, 1}};
  const std::vector<int32> expectedCounts{0, 2, 1};
  for(usize repetition = 0; repetition < 300; ++repetition)
  {
    CAPTURE(repetition);
    ExecuteAndCheckLabelPairs(pairs, expectedIds, expectedLabels, expectedCounts);
  }
}

TEST_CASE("SimplnxCore::SharedFeatureFace: Real zero pair counts", "[SimplnxCore][SharedFeatureFaceFilter]")
{
  UnitTest::LoadPlugins();
  // Pair (0,0) creates map key zero, so the reserved row and boundary row 1 share that key and both report
  // its two triangles.
  const std::vector<std::pair<int32, int32>> pairs{{0, 0}, {0, 2}, {0, 0}};
  const std::vector<int32> expectedIds{1, 2, 1};
  const std::vector<std::pair<int32, int32>> expectedLabels{{0, 0}, {0, 0}, {0, 2}};
  const std::vector<int32> expectedCounts{2, 2, 1};
  ExecuteAndCheckLabelPairs(pairs, expectedIds, expectedLabels, expectedCounts);
}
