#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"
#include "ComplexCore/Filters/TriangleDihedralAngleFilter.hpp"
#include "complex/Utilities/DataGroupUtilities.cpp"

#include <filesystem>
#include <iostream>
#include <string>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

namespace
{
constexpr float64 k_max_difference = 0.0001;
constexpr usize k_VertexTupleCount = 11;
constexpr usize k_VertexCompCount = 3;
constexpr usize k_FaceTupleCount = 4;
constexpr usize k_FaceCompCount = 3;

static const IGeometry::SharedVertexList* createVertexList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto ds = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{k_VertexTupleCount}, std::vector<usize>{k_VertexCompCount}, 0.0f);
  auto* dataArr = IGeometry::SharedVertexList::Create(*ds, "Vertices", std::move(dataStore), parentId);
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<IGeometry::SharedVertexList*>(dataArr);
}

static const IGeometry::SharedFaceList* createFaceList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto ds = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{k_FaceTupleCount}, std::vector<usize>{k_FaceCompCount}, 0);
  auto* dataArr = IGeometry::SharedFaceList::Create(*ds, "Faces", std::move(dataStore), parentId);
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<IGeometry::SharedFaceList*>(dataArr);
}

} // namespace

TEST_CASE("ComplexCore::TriangleDihedralAngleFilter[valid results]", "[ComplexCore][TriangleDihedralAngleFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string dihedralAnglesDataArrayName = "DihedralAngles";
  DataStructure dataStructure;
  TriangleGeom& acuteTriangle = *TriangleGeom::Create(dataStructure, triangleGeometryName);
  AttributeMatrix* faceData = AttributeMatrix::Create(dataStructure, INodeGeometry2D::k_FaceDataName, acuteTriangle.getId());
  faceData->setShape({k_FaceTupleCount});
  acuteTriangle.setFaceData(*faceData);
  AttributeMatrix* vertData = AttributeMatrix::Create(dataStructure, INodeGeometry0D::k_VertexDataName, acuteTriangle.getId());
  vertData->setShape({k_VertexTupleCount});
  acuteTriangle.setVertexData(*vertData);
  auto vertexList = createVertexList(acuteTriangle, vertData->getId());
  auto facesList = createFaceList(acuteTriangle, faceData->getId());
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
  acuteTriangle.setFaces(*facesList);
  acuteTriangle.setVertices(*vertexList);

  TriangleDihedralAngleFilter filter;
  Arguments args;
  std::string triangleDihedralAnglesName = "Triangle Dihedral Angles";

  DataPath geometryPath = dataStructure.getDataPathsForId(acuteTriangle.getId())[0];
  DataPath triangleDihedralAnglesDataPath = geometryPath.createChildPath(INodeGeometry2D::k_FaceDataName).createChildPath(triangleDihedralAnglesName);

  // Create default Parameters for the filter.
  args.insertOrAssign(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, std::make_any<std::string>(triangleDihedralAnglesName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  std::vector<float64> officialDihedralAngleMinimums = {53.1301002502441, 18.4349555969238, 0.0, 0.0};
  Float64Array& calculatedDihedralAngleMinimums = dataStructure.getDataRefAs<Float64Array>(triangleDihedralAnglesDataPath);

  auto debug1 = calculatedDihedralAngleMinimums[0];
  auto debug2 = calculatedDihedralAngleMinimums[1];
  auto debug3 = calculatedDihedralAngleMinimums[2];
  auto debug4 = calculatedDihedralAngleMinimums[3];

  auto result1 = fabs(officialDihedralAngleMinimums[0] - calculatedDihedralAngleMinimums[0]);
  REQUIRE(result1 < ::k_max_difference);
  auto result2 = fabs(officialDihedralAngleMinimums[1] - calculatedDihedralAngleMinimums[1]);
  REQUIRE(result2 < ::k_max_difference);
  REQUIRE(std::isnan(calculatedDihedralAngleMinimums[2]));
  auto result4 = fabs(officialDihedralAngleMinimums[3] - calculatedDihedralAngleMinimums[3]);
  REQUIRE(result4 < ::k_max_difference);
}
