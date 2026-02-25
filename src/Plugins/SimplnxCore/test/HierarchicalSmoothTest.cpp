#include "SimplnxCore/Filters/HierarchicalSmoothFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_TriangleGeometryName = "[Triangle Geometry]";
const std::string k_FaceDataName = INodeGeometry2D::k_FaceAttributeMatrixName;
const std::string k_VertexDataName = INodeGeometry0D::k_VertexAttributeMatrixName;
const std::string k_NodeTypeName = "NodeType";
const std::string k_FaceLabelsName = "FaceLabels";

const fs::path k_ExampleDataDir = fs::path("/Users/mjackson/Workspace5/HierarchicalSmooth/examples/ex1");
const fs::path k_SharedVertexListFile = k_ExampleDataDir / "SharedVertexList.txt";
const fs::path k_SharedTriListFile = k_ExampleDataDir / "SharedTriList.txt";
const fs::path k_FaceLabelsFile = k_ExampleDataDir / "FaceLabels.txt";
const fs::path k_NodeTypeFile = k_ExampleDataDir / "NodeType.txt";
const fs::path k_SmoothedVertexListFile = k_ExampleDataDir / "SmoothedVertexLlist.txt";

// Helper to read a 2D matrix of values from a tab/space separated text file
template <typename T>
std::vector<std::vector<T>> readTextFile(const fs::path& filePath)
{
  std::vector<std::vector<T>> data;
  std::ifstream file(filePath);
  REQUIRE(file.is_open());
  std::string line;
  while(std::getline(file, line))
  {
    if(line.empty())
    {
      continue;
    }
    std::istringstream iss(line);
    std::vector<T> row;
    T val;
    while(iss >> val)
    {
      row.push_back(val);
    }
    if(!row.empty())
    {
      data.push_back(row);
    }
  }
  return data;
}

} // namespace

TEST_CASE("SimplnxCore::HierarchicalSmoothFilter: Valid filter execution", "[SurfaceMeshing][HierarchicalSmoothFilter]")
{
  UnitTest::LoadPlugins();

  // Skip if test data doesn't exist
  if(!fs::exists(k_SharedVertexListFile))
  {
    WARN("Skipping HierarchicalSmooth test: example data not found at " + k_ExampleDataDir.string());
    return;
  }

  // Read input data files
  auto vertexData = readTextFile<double>(k_SharedVertexListFile);
  auto triData = readTextFile<int>(k_SharedTriListFile);
  auto faceLabelData = readTextFile<int>(k_FaceLabelsFile);
  auto nodeTypeData = readTextFile<int>(k_NodeTypeFile);

  usize numVertices = vertexData.size();
  usize numFaces = triData.size();

  REQUIRE(numVertices > 0);
  REQUIRE(numFaces > 0);
  REQUIRE(faceLabelData.size() == numFaces);
  REQUIRE(nodeTypeData.size() == numVertices);

  // Build the DataStructure with a TriangleGeom
  DataStructure dataStructure;
  TriangleGeom& triangleGeom = *TriangleGeom::Create(dataStructure, k_TriangleGeometryName);

  // Create attribute matrices
  AttributeMatrix* faceAttrMatrix = AttributeMatrix::Create(dataStructure, k_FaceDataName, {numFaces}, triangleGeom.getId());
  triangleGeom.setFaceAttributeMatrix(*faceAttrMatrix);
  AttributeMatrix* vertexAttrMatrix = AttributeMatrix::Create(dataStructure, k_VertexDataName, {numVertices}, triangleGeom.getId());
  triangleGeom.setVertexAttributeMatrix(*vertexAttrMatrix);

  // Create and populate vertex list
  auto vertexStore = std::make_unique<DataStore<float32>>(std::vector<usize>{numVertices}, std::vector<usize>{3}, 0.0f);
  auto* vertexArray = IGeometry::SharedVertexList::Create(dataStructure, "Vertices", std::move(vertexStore), vertexAttrMatrix->getId());
  REQUIRE(vertexArray != nullptr);
  auto vertexStorePtr = vertexArray->getDataStorePtr().lock();
  for(usize i = 0; i < numVertices; i++)
  {
    vertexStorePtr->setValue(3 * i + 0, static_cast<float32>(vertexData[i][0]));
    vertexStorePtr->setValue(3 * i + 1, static_cast<float32>(vertexData[i][1]));
    vertexStorePtr->setValue(3 * i + 2, static_cast<float32>(vertexData[i][2]));
  }
  triangleGeom.setVertices(*vertexArray);

  // Create and populate face list
  auto faceStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{numFaces}, std::vector<usize>{3}, static_cast<IGeometry::MeshIndexType>(0));
  auto* faceArray = IGeometry::SharedFaceList::Create(dataStructure, "Faces", std::move(faceStore), faceAttrMatrix->getId());
  REQUIRE(faceArray != nullptr);
  auto faceStorePtr = faceArray->getDataStorePtr().lock();
  for(usize i = 0; i < numFaces; i++)
  {
    faceStorePtr->setValue(3 * i + 0, static_cast<IGeometry::MeshIndexType>(triData[i][0]));
    faceStorePtr->setValue(3 * i + 1, static_cast<IGeometry::MeshIndexType>(triData[i][1]));
    faceStorePtr->setValue(3 * i + 2, static_cast<IGeometry::MeshIndexType>(triData[i][2]));
  }
  triangleGeom.setFaceList(*faceArray);

  // Create and populate Node Type array (Int8)
  Int8Array* nodeTypeArray = UnitTest::CreateTestDataArray<int8>(dataStructure, k_NodeTypeName, {numVertices}, {1}, vertexAttrMatrix->getId());
  for(usize i = 0; i < numVertices; i++)
  {
    (*nodeTypeArray)[i] = static_cast<int8>(nodeTypeData[i][0]);
  }

  // Create and populate Face Labels array (Int32, 2-component)
  Int32Array* faceLabelsArray = UnitTest::CreateTestDataArray<int32>(dataStructure, k_FaceLabelsName, {numFaces}, {2}, faceAttrMatrix->getId());
  for(usize i = 0; i < numFaces; i++)
  {
    (*faceLabelsArray)[2 * i + 0] = static_cast<int32>(faceLabelData[i][0]);
    (*faceLabelsArray)[2 * i + 1] = static_cast<int32>(faceLabelData[i][1]);
  }

  // Set up filter arguments
  DataPath triangleGeomPath({k_TriangleGeometryName});
  DataPath nodeTypePath = triangleGeomPath.createChildPath(k_VertexDataName).createChildPath(k_NodeTypeName);
  DataPath faceLabelsPath = triangleGeomPath.createChildPath(k_FaceDataName).createChildPath(k_FaceLabelsName);

  HierarchicalSmoothFilter filter;
  Arguments args;
  args.insertOrAssign(HierarchicalSmoothFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(triangleGeomPath));
  args.insertOrAssign(HierarchicalSmoothFilter::k_SurfaceMeshNodeTypeArrayPath_Key, std::make_any<DataPath>(nodeTypePath));
  args.insertOrAssign(HierarchicalSmoothFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabelsPath));
  args.insertOrAssign(HierarchicalSmoothFilter::k_MaxIterations_Key, std::make_any<int32>(53));
  args.insertOrAssign(HierarchicalSmoothFilter::k_ErrorThreshold_Key, std::make_any<float64>(2.0));

  // Preflight
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute
  auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Verify results
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(triangleGeomPath));
  const auto& resultGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomPath);
  const auto& resultVerticesRef = resultGeom.getVertices()->getDataStoreRef();

  // Check no NaN or inf values
  for(usize i = 0; i < numVertices * 3; i++)
  {
    REQUIRE_FALSE(std::isnan(resultVerticesRef[i]));
    REQUIRE_FALSE(std::isinf(resultVerticesRef[i]));
  }

  // Verify that the smoothing actually modified some vertices
  // (i.e., the output is not identical to the input)
  usize modifiedCount = 0;
  for(usize i = 0; i < numVertices; i++)
  {
    for(usize j = 0; j < 3; j++)
    {
      if(std::fabs(static_cast<double>(resultVerticesRef[3 * i + j]) - vertexData[i][j]) > 1.0e-6)
      {
        modifiedCount++;
        break;
      }
    }
  }
  INFO("Modified vertices: " << modifiedCount << " of " << numVertices);
  CHECK(modifiedCount > 0);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/hierarchical_smooth_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
