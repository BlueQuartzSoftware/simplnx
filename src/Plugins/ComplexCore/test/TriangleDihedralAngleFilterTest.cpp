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

static const AbstractGeometry::SharedVertexList* createVertexList(AbstractGeometry& geom)
{
  auto ds = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{30}, std::vector<usize>{3}, 0.0f);
  auto dataArr = AbstractGeometry::SharedVertexList::Create(*ds, "Vertices", std::move(dataStore), geom.getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const AbstractGeometry::SharedVertexList*>(dataArr);
}

static const AbstractGeometry::SharedFaceList* createFaceList(AbstractGeometry& geom)
{
  auto ds = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<AbstractGeometry::MeshIndexType>>(std::vector<usize>{16}, std::vector<usize>{4}, 0);
  auto dataArr = AbstractGeometry::SharedFaceList::Create(*ds, "Faces", std::move(dataStore), geom.getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const AbstractGeometry::SharedFaceList*>(dataArr);
}

} // namespace

TEST_CASE("ComplexCore::TriangleDihedralAngleFilter[valid results]", "[ComplexCore][TriangleDihedralAngleFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string dihedralAnglesDataArrayName = "DihedralAngles";
  DataStructure dataStructure;
  TriangleGeom& acuteTriangle = *TriangleGeom::Create(dataStructure, triangleGeometryName);
  auto underlyingStoreV = createVertexList(acuteTriangle)->getDataStorePtr().lock();
  auto underlyingStoreF = createFaceList(acuteTriangle)->getDataStorePtr().lock();
  // load Vertex list
  std::vector<float32> vertexes = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 3.0f, 0.0f,  0.0f, 4.0f, 1.0f,
                                   0.0f, 6.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 9.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f};
  auto count = 0;
  for(auto element : vertexes)
  {
    underlyingStoreV->setValue(count, element);
    count++;
  }
  // load face list
  std::vector<AbstractGeometry::MeshIndexType> faces = {0, 0, 1, 2, 1, 3, 4, 5, 2, 6, 7, 7, 3, 8, 9, 10};
  count = 0;
  for(auto element : faces)
  {
    underlyingStoreF->setValue(count, element);
    count++;
  }

  TriangleDihedralAngleFilter filter;
  Arguments args;
  std::string triangleDihedralAnglesName = "Triangle Dihedral Angles";

  DataPath geometryPath = dataStructure.getDataPathsForId(acuteTriangle.getId())[0];

  // Create default Parameters for the filter.
  DataPath triangleDihedralAnglesDataPath = geometryPath.createChildPath(triangleDihedralAnglesName);

  args.insertOrAssign(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayPath_Key, std::make_any<DataPath>(triangleDihedralAnglesDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  std::vector<float64> officialDihedralAngleMinimums = {53.1301002502441, 18.4349555969238, 0.0, 0.0};
  Float64Array& calculatedDihedralAngleMinimums = dataStructure.getDataRefAs<Float64Array>(triangleDihedralAnglesDataPath);
  for(int64 i = 0; i < calculatedDihedralAngleMinimums.getSize(); i++)
  {
    auto result = fabs(officialDihedralAngleMinimums[i] - calculatedDihedralAngleMinimums[i]);
    REQUIRE(result < ::k_max_difference);
  }
}
