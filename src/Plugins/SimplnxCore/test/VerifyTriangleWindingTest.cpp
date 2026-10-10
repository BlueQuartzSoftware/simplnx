#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/LabelTriangleGeometryFilter.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/Filters/VerifyTriangleWindingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_TriangleGeomPath = DataPath({Constants::k_DataContainer});
const DataPath k_FaceDataPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data);

/**
 * @brief Creates four resident tuples without consulting storage preferences.
 * @tparam T Value type of the fixture array.
 * @param dataStructure Owns the new array and its existing parent.
 * @param path Identifies the array below an existing geometry or attribute matrix.
 * @param componentCount Number of values per tuple.
 * @param values Supplies all four tuples.
 * @return Array owned by dataStructure.
 */
template <typename T>
DataArray<T>* CreateDisabledNormalsArray(DataStructure& dataStructure, const DataPath& path, usize componentCount, nonstd::span<const T> values)
{
  REQUIRE(values.size() == 4 * componentCount);
  const auto* parentPtr = dataStructure.getData(path.getParent());
  REQUIRE(parentPtr != nullptr);
  auto store = std::make_shared<DataStore<T>>(ShapeType{4}, ShapeType{componentCount}, T{});
  auto writeResult = store->copyFromBuffer(0, values);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  auto* arrayPtr = DataArray<T>::Create(dataStructure, path.getTargetName(), store, parentPtr->getId());
  REQUIRE(arrayPtr != nullptr);
  return arrayPtr;
}

/**
 * @brief Checks every value, shape, and identity while the DataStructure owns the array.
 * @tparam T Value type of the fixture array.
 * @param dataStructure Owns the array under test.
 * @param path Identifies the original array location.
 * @param expectedId Original DataObject ID.
 * @param expectedStorePtr Original store address, used only for comparison.
 * @param componentCount Expected values per tuple.
 * @param expectedValues Independent expected values for all four tuples.
 */
template <typename T>
void CheckDisabledNormalsArray(const DataStructure& dataStructure, const DataPath& path, DataObject::IdType expectedId, const AbstractDataStore<T>* expectedStorePtr, usize componentCount,
                               nonstd::span<const T> expectedValues)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(path));
  const auto& array = dataStructure.getDataRefAs<DataArray<T>>(path);
  REQUIRE(array.getId() == expectedId);
  REQUIRE(array.getDataStore() == expectedStorePtr);
  REQUIRE(array.getDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(array.getTupleShape() == ShapeType{4});
  REQUIRE(array.getComponentShape() == ShapeType{componentCount});
  REQUIRE(array.getSize() == expectedValues.size());
  std::vector<T> actualValues(expectedValues.size(), T{});
  auto readResult = array.getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(actualValues.data(), actualValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
  {
    CAPTURE(path.toString(), valueIndex);
    CHECK(actualValues[valueIndex] == expectedValues[valueIndex]);
  }
}
} // namespace

TEST_CASE("SimplnxCore::VerifyTriangleWindingFilter: Valid Face Labels Execution", "[SimplnxCore][VerifyTriangleWindingFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "STL_Models.tar.gz", "STL_Models");

  DataStructure dataStructure = {};
  {
    Arguments args;
    ReadStlFileFilter filter;

    std::string inputFile = fmt::format("{}/STL_Models/Example_Triangle_Geometry.stl", unit_test::k_TestFilesDir);

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<fs::path>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
    args.insertOrAssign(ReadStlFileFilter::k_CreateFaceLabels_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadStlFileFilter::k_FaceLabelsName_Key, std::make_any<std::string>(Constants::k_FaceLabels));
    args.insertOrAssign(ReadStlFileFilter::k_FaceNormalsName_Key, std::make_any<std::string>(Constants::k_FaceNormals));
    args.insertOrAssign(ReadStlFileFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Face_Data));
    args.insertOrAssign(ReadStlFileFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Vertex_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    auto& triGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    auto& triangles = triGeom.getFacesRef();

    for(usize i = 0; i < triangles.getNumberOfTuples(); i++)
    {
      if(i % 4 == 0)
      {
        IGeometry::MeshIndexType temp = triangles[(i * 3) + 0];
        triangles[(i * 3) + 0] = triangles[(i * 3) + 2];
        triangles[(i * 3) + 2] = temp;
      }
    }
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    LabelTriangleGeometryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(::k_TriangleGeomPath));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_CreatedRegionIdsPath_Key, std::make_any<DataPath>(::k_FaceDataPath.createChildPath(Constants::k_FeatureIds)));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Region Data"));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_NumTrianglesName_Key, std::make_any<std::string>("Num Triangles"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // copy region ids into the surface labels
    auto& faceLabels = dataStructure.getDataRefAs<Int32Array>(k_FaceDataPath.createChildPath(Constants::k_FaceLabels));
    auto& regionIds = dataStructure.getDataRefAs<Int32Array>(k_FaceDataPath.createChildPath(Constants::k_FeatureIds));
    for(usize i = 0; i < faceLabels.getNumberOfTuples(); i++)
    {
      faceLabels[(i * 2) + 0] = 0;
      faceLabels[(i * 2) + 1] = regionIds[i];
    }
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    VerifyTriangleWindingFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(VerifyTriangleWindingFilter::k_LabelsPath_Key, std::make_any<DataPath>(k_FaceDataPath.createChildPath(Constants::k_FaceLabels)));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_RepairNormals_Key, std::make_any<bool>(true));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_TriangleNormalsPath_Key, std::make_any<DataPath>(k_FaceDataPath.createChildPath(Constants::k_FaceNormals)));

    const DataPath labelsPath = k_FaceDataPath.createChildPath(Constants::k_FaceLabels);
    const DataPath normalsPath = k_FaceDataPath.createChildPath(Constants::k_FaceNormals);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    REQUIRE(triangleGeom.getVertices() != nullptr);
    REQUIRE(triangleGeom.getFaces() != nullptr);
    scope.requireExpectedStore(*triangleGeom.getVertices());
    scope.requireExpectedStore(*triangleGeom.getFaces());
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(labelsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(normalsPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(labelsPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(normalsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    REQUIRE(executeResult.result.valid());

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& repairedTriangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    REQUIRE(repairedTriangleGeom.getVertices() != nullptr);
    REQUIRE(repairedTriangleGeom.getFaces() != nullptr);
    scope.requireExpectedStore(*repairedTriangleGeom.getVertices());
    scope.requireExpectedStore(*repairedTriangleGeom.getFaces());
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(labelsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(normalsPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(labelsPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(normalsPath));
  }

  DataStructure exemplarDataStructure = {};
  {
    Arguments args;
    ReadStlFileFilter filter;

    std::string inputFile = fmt::format("{}/STL_Models/Example_Triangle_Geometry.stl", unit_test::k_TestFilesDir);

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<fs::path>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
    args.insertOrAssign(ReadStlFileFilter::k_CreateFaceLabels_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadStlFileFilter::k_FaceLabelsName_Key, std::make_any<std::string>(Constants::k_FaceLabels));
    args.insertOrAssign(ReadStlFileFilter::k_FaceNormalsName_Key, std::make_any<std::string>(Constants::k_FaceNormals));
    args.insertOrAssign(ReadStlFileFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Face_Data));
    args.insertOrAssign(ReadStlFileFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Vertex_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(exemplarDataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(exemplarDataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const DataPath normalsPath = k_FaceDataPath.createChildPath(Constants::k_FaceNormals);

  UnitTest::CompareDataArrays<float64>(dataStructure.getDataRefAs<Float64Array>(normalsPath), exemplarDataStructure.getDataRefAs<Float64Array>(normalsPath));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::VerifyTriangleWindingFilter: Valid Region Ids Execution", "[SimplnxCore][VerifyTriangleWindingFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "STL_Models.tar.gz", "STL_Models");

  DataStructure dataStructure = {};
  {
    Arguments args;
    ReadStlFileFilter filter;

    std::string inputFile = fmt::format("{}/STL_Models/Example_Triangle_Geometry.stl", unit_test::k_TestFilesDir);

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<fs::path>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
    args.insertOrAssign(ReadStlFileFilter::k_CreateFaceLabels_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadStlFileFilter::k_FaceLabelsName_Key, std::make_any<std::string>(Constants::k_FaceLabels));
    args.insertOrAssign(ReadStlFileFilter::k_FaceNormalsName_Key, std::make_any<std::string>(Constants::k_FaceNormals));
    args.insertOrAssign(ReadStlFileFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Face_Data));
    args.insertOrAssign(ReadStlFileFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Vertex_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    auto& triGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    auto& triangles = triGeom.getFacesRef();

    for(usize i = 0; i < triangles.getNumberOfTuples(); i++)
    {
      if(i % 4 == 0)
      {
        IGeometry::MeshIndexType temp = triangles[(i * 3) + 0];
        triangles[(i * 3) + 0] = triangles[(i * 3) + 2];
        triangles[(i * 3) + 2] = temp;
      }
    }
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    LabelTriangleGeometryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(::k_TriangleGeomPath));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_CreatedRegionIdsPath_Key, std::make_any<DataPath>(::k_FaceDataPath.createChildPath(Constants::k_FeatureIds)));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Region Data"));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_NumTrianglesName_Key, std::make_any<std::string>("Num Triangles"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    VerifyTriangleWindingFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(VerifyTriangleWindingFilter::k_LabelsPath_Key, std::make_any<DataPath>(k_FaceDataPath.createChildPath(Constants::k_FeatureIds)));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_RepairNormals_Key, std::make_any<bool>(true));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_TriangleNormalsPath_Key, std::make_any<DataPath>(k_FaceDataPath.createChildPath(Constants::k_FaceNormals)));

    const DataPath labelsPath = k_FaceDataPath.createChildPath(Constants::k_FeatureIds);
    const DataPath normalsPath = k_FaceDataPath.createChildPath(Constants::k_FaceNormals);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    REQUIRE(triangleGeom.getVertices() != nullptr);
    REQUIRE(triangleGeom.getFaces() != nullptr);
    scope.requireExpectedStore(*triangleGeom.getVertices());
    scope.requireExpectedStore(*triangleGeom.getFaces());
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(labelsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(normalsPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(labelsPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(normalsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    REQUIRE(executeResult.result.valid());

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& repairedTriangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    REQUIRE(repairedTriangleGeom.getVertices() != nullptr);
    REQUIRE(repairedTriangleGeom.getFaces() != nullptr);
    scope.requireExpectedStore(*repairedTriangleGeom.getVertices());
    scope.requireExpectedStore(*repairedTriangleGeom.getFaces());
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(labelsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(normalsPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(labelsPath));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(normalsPath));
  }

  DataStructure exemplarDataStructure = {};
  {
    Arguments args;
    ReadStlFileFilter filter;

    std::string inputFile = fmt::format("{}/STL_Models/Example_Triangle_Geometry.stl", unit_test::k_TestFilesDir);

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<fs::path>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
    args.insertOrAssign(ReadStlFileFilter::k_CreateFaceLabels_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadStlFileFilter::k_FaceLabelsName_Key, std::make_any<std::string>(Constants::k_FaceLabels));
    args.insertOrAssign(ReadStlFileFilter::k_FaceNormalsName_Key, std::make_any<std::string>(Constants::k_FaceNormals));
    args.insertOrAssign(ReadStlFileFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Face_Data));
    args.insertOrAssign(ReadStlFileFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Vertex_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(exemplarDataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(exemplarDataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const DataPath normalsPath = k_FaceDataPath.createChildPath(Constants::k_FaceNormals);

  UnitTest::CompareDataArrays<float64>(dataStructure.getDataRefAs<Float64Array>(normalsPath), exemplarDataStructure.getDataRefAs<Float64Array>(normalsPath));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::VerifyTriangleWindingFilter: Duplicate Vertices Catch", "[SimplnxCore][VerifyTriangleWindingFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "STL_Models.tar.gz", "STL_Models");

  DataStructure dataStructure = {};
  {
    Arguments args;
    ReadStlFileFilter filter;

    std::string inputFile = fmt::format("{}/STL_Models/Example_Triangle_Geometry.stl", unit_test::k_TestFilesDir);

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<fs::path>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
    args.insertOrAssign(ReadStlFileFilter::k_CreateFaceLabels_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadStlFileFilter::k_FaceLabelsName_Key, std::make_any<std::string>(Constants::k_FaceLabels));
    args.insertOrAssign(ReadStlFileFilter::k_FaceNormalsName_Key, std::make_any<std::string>(Constants::k_FaceNormals));
    args.insertOrAssign(ReadStlFileFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Face_Data));
    args.insertOrAssign(ReadStlFileFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Vertex_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    auto& triGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    auto& vertexList = triGeom.getVerticesRef();

    constexpr usize currentVert = 3;
    constexpr usize prevVert = 0;

    // Replace the vertex with a duplicate point. This test can invalidate faces.
    vertexList[currentVert] = vertexList[prevVert];
    vertexList[currentVert + 1] = vertexList[prevVert + 1];
    vertexList[currentVert + 2] = vertexList[prevVert + 2];
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    VerifyTriangleWindingFilter filter;
    Arguments args;

    const DataPath faceDataPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data);

    // Create default Parameters for the filter.
    args.insertOrAssign(VerifyTriangleWindingFilter::k_LabelsPath_Key, std::make_any<DataPath>(faceDataPath.createChildPath(Constants::k_FaceLabels)));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_RepairNormals_Key, std::make_any<bool>(true));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_TriangleNormalsPath_Key, std::make_any<DataPath>(faceDataPath.createChildPath(Constants::k_FaceNormals)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::VerifyTriangleWindingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][VerifyTriangleWindingFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "VerifyTriangleWindingFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<VerifyTriangleWindingFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(VerifyTriangleWindingFilter::k_LabelsPath_Key) == DataPath({"DataContainer", "FaceData", "FaceLabels"}));
    }
  }
}

TEST_CASE("SimplnxCore::VerifyTriangleWindingFilter: winding repair preserves disabled normals", "[SimplnxCore][VerifyTriangleWindingFilter]")
{
  UnitTest::LoadPlugins();
  const bool useFaceLabels = GENERATE(false, true);
  const bool hasNormals = GENERATE(false, true);
  DYNAMIC_SECTION("Face labels: " << useFaceLabels << ", existing normals: " << hasNormals)
  {
    const DataPath geometryPath({"DisabledNormalsTetrahedron"});
    const DataPath faceDataPath = geometryPath.createChildPath("FaceData");
    const DataPath vertexDataPath = geometryPath.createChildPath("VertexData");
    const DataPath facesPath = geometryPath.createChildPath("Faces");
    const DataPath verticesPath = geometryPath.createChildPath("Vertices");
    const DataPath idsPath = faceDataPath.createChildPath("Ids");
    const DataPath normalsPath = faceDataPath.createChildPath("Normals");
    const usize idComponentCount = useFaceLabels ? 2 : 1;
    const std::vector<int32> expectedIds = useFaceLabels ? std::vector<int32>{0, 1, 0, 1, 0, 1, 0, 1} : std::vector<int32>{1, 1, 1, 1};
    constexpr std::array<float32, 12> vertices = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};
    // Each expected edge has an opposite edge on its neighboring face. These faces enclose a unit tetrahedron with outward normals.
    constexpr std::array<IGeometry::MeshIndexType, 12> expectedFaces = {0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3};
    // Faces 1 and 3 start reversed; face 0 retains the outward seed orientation.
    constexpr std::array<IGeometry::MeshIndexType, 12> inputFaces = {0, 2, 1, 3, 1, 0, 0, 3, 2, 3, 2, 1};
    constexpr std::array<float64, 12> sentinelNormals = {11, 12, 13, 21, 22, 23, 31, 32, 33, 41, 42, 43};
    REQUIRE(inputFaces != expectedFaces);

    DataStructure dataStructure;
    auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, geometryPath.getTargetName());
    REQUIRE(triangleGeomPtr != nullptr);
    const auto geometryId = triangleGeomPtr->getId();
    auto* faceDataPtr = AttributeMatrix::Create(dataStructure, faceDataPath.getTargetName(), {4}, geometryId);
    auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, vertexDataPath.getTargetName(), {4}, geometryId);
    REQUIRE(faceDataPtr != nullptr);
    REQUIRE(vertexDataPtr != nullptr);
    triangleGeomPtr->setFaceAttributeMatrix(*faceDataPtr);
    triangleGeomPtr->setVertexAttributeMatrix(*vertexDataPtr);

    auto* verticesPtr = CreateDisabledNormalsArray<float32>(dataStructure, verticesPath, 3, nonstd::span<const float32>(vertices.data(), vertices.size()));
    auto* facesPtr = CreateDisabledNormalsArray<IGeometry::MeshIndexType>(dataStructure, facesPath, 3, nonstd::span<const IGeometry::MeshIndexType>(inputFaces.data(), inputFaces.size()));
    auto* idsPtr = CreateDisabledNormalsArray<int32>(dataStructure, idsPath, idComponentCount, nonstd::span<const int32>(expectedIds.data(), expectedIds.size()));
    triangleGeomPtr->setVertices(*verticesPtr);
    triangleGeomPtr->setFaceList(*facesPtr);
    const auto verticesId = verticesPtr->getId();
    const auto facesId = facesPtr->getId();
    const auto idsId = idsPtr->getId();
    const auto* verticesStorePtr = verticesPtr->getDataStore();
    const auto* facesStorePtr = facesPtr->getDataStore();
    const auto* idsStorePtr = idsPtr->getDataStore();
    std::optional<DataObject::IdType> normalsId;
    const AbstractDataStore<float64>* normalsStorePtr = nullptr;
    if(hasNormals)
    {
      auto* normalsPtr = CreateDisabledNormalsArray<float64>(dataStructure, normalsPath, 3, nonstd::span<const float64>(sentinelNormals.data(), sentinelNormals.size()));
      normalsId = normalsPtr->getId();
      normalsStorePtr = normalsPtr->getDataStore();
    }

    const auto checkUnchangedInputs = [&]() {
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(geometryPath));
      const auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(geometryPath);
      REQUIRE(geometry.getId() == geometryId);
      REQUIRE(geometry.getVertices() == verticesPtr);
      REQUIRE(geometry.getFaces() == facesPtr);
      REQUIRE(geometry.getFaceAttributeMatrix() == faceDataPtr);
      REQUIRE(geometry.getVertexAttributeMatrix() == vertexDataPtr);
      REQUIRE(dataStructure.getData(faceDataPath) == faceDataPtr);
      REQUIRE(dataStructure.getData(vertexDataPath) == vertexDataPtr);
      REQUIRE(geometry.getFaceAttributeMatrix()->getShape() == ShapeType{4});
      REQUIRE(geometry.getVertexAttributeMatrix()->getShape() == ShapeType{4});
      CheckDisabledNormalsArray<float32>(dataStructure, verticesPath, verticesId, verticesStorePtr, 3, nonstd::span<const float32>(vertices.data(), vertices.size()));
      CheckDisabledNormalsArray<int32>(dataStructure, idsPath, idsId, idsStorePtr, idComponentCount, nonstd::span<const int32>(expectedIds.data(), expectedIds.size()));
      if(hasNormals)
      {
        REQUIRE(normalsId.has_value());
        CheckDisabledNormalsArray<float64>(dataStructure, normalsPath, normalsId.value(), normalsStorePtr, 3, nonstd::span<const float64>(sentinelNormals.data(), sentinelNormals.size()));
      }
      else
      {
        REQUIRE(dataStructure.getData(normalsPath) == nullptr);
      }
    };
    checkUnchangedInputs();
    CheckDisabledNormalsArray<IGeometry::MeshIndexType>(dataStructure, facesPath, facesId, facesStorePtr, 3, nonstd::span<const IGeometry::MeshIndexType>(inputFaces.data(), inputFaces.size()));

    VerifyTriangleWindingFilter filter;
    Arguments args;
    args.insertOrAssign(VerifyTriangleWindingFilter::k_LabelsPath_Key, std::make_any<DataPath>(idsPath));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_RepairNormals_Key, std::make_any<bool>(false));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_TriangleNormalsPath_Key, std::make_any<DataPath>(normalsPath));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    // Normal repair owns the algorithm selector. This disabled branch uses the shared winding implementation.
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    checkUnchangedInputs();
    CheckDisabledNormalsArray<IGeometry::MeshIndexType>(dataStructure, facesPath, facesId, facesStorePtr, 3, nonstd::span<const IGeometry::MeshIndexType>(expectedFaces.data(), expectedFaces.size()));
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
