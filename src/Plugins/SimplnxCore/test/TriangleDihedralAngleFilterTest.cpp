#include "SimplnxCore/Filters/TriangleDihedralAngleFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
constexpr float64 k_max_difference = 0.0001;
constexpr usize k_VertexTupleCount = 11;
constexpr usize k_VertexCompCount = 3;
constexpr usize k_FaceTupleCount = 4;
constexpr usize k_FaceCompCount = 3;

static const IGeometry::SharedVertexList* createVertexList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto dataStructure = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{k_VertexTupleCount}, std::vector<usize>{k_VertexCompCount}, 0.0f);
  auto* dataArr = IGeometry::SharedVertexList::Create(*dataStructure, "Vertices", std::move(dataStore), parentId);
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<IGeometry::SharedVertexList*>(dataArr);
}

static const IGeometry::SharedFaceList* createFaceList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto dataStructure = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{k_FaceTupleCount}, std::vector<usize>{k_FaceCompCount}, 0);
  auto* dataArr = IGeometry::SharedFaceList::Create(*dataStructure, "Faces", std::move(dataStore), parentId);
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<IGeometry::SharedFaceList*>(dataArr);
}

} // namespace

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter[valid results]", "[SimplnxCore][TriangleDihedralAngleFilter]")
{
  UnitTest::LoadPlugins();

  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string dihedralAnglesDataArrayName = "DihedralAngles";
  DataStructure dataStructure;
  TriangleGeom& acuteTriangle = *TriangleGeom::Create(dataStructure, triangleGeometryName);
  AttributeMatrix* faceData = AttributeMatrix::Create(dataStructure, INodeGeometry2D::k_FaceAttributeMatrixName, {k_FaceTupleCount}, acuteTriangle.getId());
  acuteTriangle.setFaceAttributeMatrix(*faceData);
  AttributeMatrix* vertData = AttributeMatrix::Create(dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, {k_VertexTupleCount}, acuteTriangle.getId());
  acuteTriangle.setVertexAttributeMatrix(*vertData);
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
  acuteTriangle.setFaceList(*facesList);
  acuteTriangle.setVertices(*vertexList);

  TriangleDihedralAngleFilter filter;
  Arguments args;
  std::string triangleDihedralAnglesName = "Triangle Dihedral Angles";

  DataPath geometryPath = dataStructure.getDataPathsForId(acuteTriangle.getId())[0];
  DataPath triangleDihedralAnglesDataPath = geometryPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName).createChildPath(triangleDihedralAnglesName);

  // Create default Parameters for the filter.
  args.insertOrAssign(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, std::make_any<std::string>(triangleDihedralAnglesName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter: SIMPL Backwards Compatibility", "[SimplnxCore][TriangleDihedralAngleFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "TriangleDihedralAngleFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "TriangleDihedralAngleFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<TriangleDihedralAngleFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayName_Key) == "TestArray");
      CHECK(args.value<DataPath>(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key) == DataPath({"DataContainer"}));
    }
  }
}

TEST_CASE("SimplnxCore::TriangleDihedralAngleFilter: Permutation invariant minimum", "[SimplnxCore][TriangleDihedralAngleFilter]")
{
  UnitTest::LoadPlugins();

  // Issue #1790 (70-30-80 degrees); a right triangle with atan(1/10) at B; an obtuse triangle (135 degrees at A);
  // a collinear face (C = -2B exactly in float32) whose normalized dot products round past +/-1 without the clamp.
  const std::array<std::array<float32, 9>, 4> fixtures = {
      {{{0, 0, 0, 1, 0, 0, 0.1736481785774231F, 0.477094441652298F, 0}}, {{0, 0, 0, 10, 0, 0, 0, 1, 0}}, {{0, 0, 0, 1, 0, 0, -0.5F, 0.5F, 0}}, {{0, 0, 0, 0.7F, 3.0F, 0.1F, -1.4F, -6.0F, -0.2F}}}};
  // Closed-form minimum interior angle of each fixture, in degrees.
  const float64 radToDeg = 180.0 / std::acos(-1.0);
  const std::array<float64, 4> closedFormMinimums = {{30.0, std::atan(0.1) * radToDeg, std::atan(1.0 / 3.0) * radToDeg, 0.0}};
  // ABC, BCA, CAB, followed by their reversed windings.
  const std::array<usize, 18> orders = {{0, 1, 2, 1, 2, 0, 2, 0, 1, 0, 2, 1, 2, 1, 0, 1, 0, 2}};
  for(usize fixture = 0; fixture < fixtures.size(); ++fixture)
  {
    CAPTURE(fixture);
    DataStructure dataStructure;
    auto* geom = TriangleGeom::Create(dataStructure, "Triangles");
    REQUIRE(geom != nullptr);
    auto* faceData = AttributeMatrix::Create(dataStructure, "Face Data", std::vector<usize>{6}, geom->getId());
    auto* vertexData = AttributeMatrix::Create(dataStructure, "Vertex Data", std::vector<usize>{3}, geom->getId());
    REQUIRE(faceData != nullptr);
    REQUIRE(vertexData != nullptr);
    geom->setFaceAttributeMatrix(*faceData);
    geom->setVertexAttributeMatrix(*vertexData);
    auto* vertices = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Vertices", std::vector<usize>{3}, std::vector<usize>{3}, geom->getId());
    auto* faces = DataArray<IGeometry::MeshIndexType>::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "Faces", std::vector<usize>{6}, std::vector<usize>{3}, geom->getId());
    REQUIRE(vertices != nullptr);
    REQUIRE(faces != nullptr);
    for(usize i = 0; i < 9; ++i)
    {
      (*vertices)[i] = fixtures[fixture][i];
    }
    for(usize i = 0; i < orders.size(); ++i)
    {
      (*faces)[i] = orders[i];
    }
    geom->setVertices(*vertices);
    geom->setFaceList(*faces);

    // Independent reference: atan2 of each corner's cross magnitude and dot.
    float64 expected = std::numeric_limits<float64>::infinity();
    for(usize corner = 0; corner < 3; ++corner)
    {
      std::array<float64, 3> toNext{};
      std::array<float64, 3> toPrev{};
      for(usize axis = 0; axis < 3; ++axis)
      {
        toNext[axis] = static_cast<float64>((*vertices)[3 * ((corner + 1) % 3) + axis]) - (*vertices)[3 * corner + axis];
        toPrev[axis] = static_cast<float64>((*vertices)[3 * ((corner + 2) % 3) + axis]) - (*vertices)[3 * corner + axis];
      }
      const std::array<float64, 3> cross = {{toNext[1] * toPrev[2] - toNext[2] * toPrev[1], toNext[2] * toPrev[0] - toNext[0] * toPrev[2], toNext[0] * toPrev[1] - toNext[1] * toPrev[0]}};
      const float64 dot = toNext[0] * toPrev[0] + toNext[1] * toPrev[1] + toNext[2] * toPrev[2];
      const float64 magnitude = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
      expected = std::min(expected, std::atan2(magnitude, dot) * radToDeg);
    }
    // The computed oracle must agree with the hand-derived closed form.
    REQUIRE(std::abs(expected - closedFormMinimums[fixture]) < 1.0e-5);
    TriangleDihedralAngleFilter filter;
    Arguments args;
    args.insertOrAssign(TriangleDihedralAngleFilter::k_TGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"Triangles"})));
    args.insertOrAssign(TriangleDihedralAngleFilter::k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, std::make_any<std::string>("Minimum"));
    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(DataPath({"Triangles", "Face Data", "Minimum"})));
    const auto& output = dataStructure.getDataRefAs<Float64Array>(DataPath({"Triangles", "Face Data", "Minimum"}));
    for(usize face = 0; face < 6; ++face)
    {
      CAPTURE(face, expected, output[face]);
      CHECK(std::isfinite(output[face]));
      // Allow float32 edge-subtraction rounding; the oracle subtracts in float64.
      CHECK(std::abs(output[face] - expected) < 1.0e-5);
      CHECK(output[face] <= 60.0 + 1.0e-5);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
