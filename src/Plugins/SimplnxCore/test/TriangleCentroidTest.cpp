#include "SimplnxCore/Filters/TriangleCentroidFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
constexpr float64 k_MaxDifference = 0.00001;
constexpr usize k_VertexTupleCount = 11;
constexpr usize k_VertexCompCount = 3;
constexpr usize k_FaceTupleCount = 4;
constexpr usize k_FaceCompCount = 3;
constexpr StringLiteral k_CentroidsName = "Centroids";

static const IGeometry::SharedVertexList* CreateVertexList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto dataStructure = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{k_VertexTupleCount}, std::vector<usize>{k_VertexCompCount}, 0.0f);
  auto* dataArr = IGeometry::SharedVertexList::Create(*dataStructure, "Vertices", std::move(dataStore), parentId);
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<IGeometry::SharedVertexList*>(dataArr);
}

static const IGeometry::SharedFaceList* CreateFaceList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto dataStructure = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{k_FaceTupleCount}, std::vector<usize>{k_FaceCompCount}, 0);
  auto* dataArr = IGeometry::SharedFaceList::Create(*dataStructure, "Faces", std::move(dataStore), parentId);
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<IGeometry::SharedFaceList*>(dataArr);
}

} // namespace

TEST_CASE("SimplnxCore::TriangleCentroidFilter", "[SimplnxCore][TriangleCentroidFilter]")
{
  DataStructure dataStructure;
  TriangleGeom& acuteTriangle = *TriangleGeom::Create(dataStructure, k_TriangleGeometryName);
  AttributeMatrix* faceData = AttributeMatrix::Create(dataStructure, INodeGeometry2D::k_FaceDataName, {k_FaceTupleCount}, acuteTriangle.getId());
  acuteTriangle.setFaceAttributeMatrix(*faceData);
  AttributeMatrix* vertData = AttributeMatrix::Create(dataStructure, INodeGeometry0D::k_VertexDataName, {k_VertexTupleCount}, acuteTriangle.getId());
  acuteTriangle.setVertexAttributeMatrix(*vertData);
  auto vertexList = CreateVertexList(acuteTriangle, vertData->getId());
  auto facesList = CreateFaceList(acuteTriangle, faceData->getId());
  auto underlyingStoreV = vertexList->getDataStorePtr().lock();
  auto underlyingStoreF = facesList->getDataStorePtr().lock();

  // load Vertex list
  std::vector<float32> vertexes = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 3.0f, 0.0f,  0.0f, 4.0f, 1.0f,
                                   0.0f, 6.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 9.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f};
  auto count = 0;
  for(auto element : vertexes)
  {
    underlyingStoreV->setValue(count++, element);
  }
  // load face list
  std::vector<IGeometry::MeshIndexType> faces = {0, 1, 2, 3, 4, 5, 6, 7, 7, 8, 9, 10};
  count = 0;
  for(auto element : faces)
  {
    underlyingStoreF->setValue(count++, element);
  }
  acuteTriangle.setFaceList(*facesList);
  acuteTriangle.setVertices(*vertexList);

  const TriangleCentroidFilter filter;
  Arguments args;

  DataPath geometryPath = dataStructure.getDataPathsForId(acuteTriangle.getId())[0];
  DataPath triangleCentroidsDataPath = geometryPath.createChildPath(INodeGeometry2D::k_FaceDataName).createChildPath(k_CentroidsName);

  // Create default Parameters for the filter.
  args.insertOrAssign(TriangleCentroidFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(TriangleCentroidFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  std::vector<Point3Df> centroids = {{0.5F, 0.333333F, 0.0F}, {3.0F, 0.333333, 0.0F}, {6.66667, 0.0F, 0.0F}, {9.0F, 0.0F, 0.0F}};

  const auto& calculatedCentroids = dataStructure.getDataRefAs<Float32Array>(triangleCentroidsDataPath);

  for(size_t t = 0; t < 4; t++)
  {
    for(size_t c = 0; c < 3; c++)
    {
      auto result1 = fabs(calculatedCentroids[t * 3 + c] - centroids[t][c]);
      REQUIRE(result1 < ::k_MaxDifference);
    }
  }
}
