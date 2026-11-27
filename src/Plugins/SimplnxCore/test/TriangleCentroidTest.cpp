#include "SimplnxCore/Filters/TriangleCentroidFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::constants;

namespace
{
constexpr float64 k_MaxDifference = 0.00001;
constexpr usize k_VertexTupleCount = 11;
constexpr usize k_VertexCompCount = 3;
constexpr usize k_FaceTupleCount = 4;
constexpr usize k_FaceCompCount = 3;
constexpr StringLiteralType k_CentroidsName = "Centroids";

const IGeometry::SharedVertexListType* CreateVertexList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto* dataStructurePtr = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{k_VertexTupleCount}, std::vector<usize>{k_VertexCompCount}, 0.0f);
  auto* dataArrPtr = IGeometry::SharedVertexListType::Create(*dataStructurePtr, "Vertices", std::move(dataStore), parentId);
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<IGeometry::SharedVertexListType*>(dataArrPtr);
}

const IGeometry::SharedFaceListType* CreateFaceList(IGeometry& geom, const DataObject::IdType parentId)
{
  auto* dataStructurePtr = geom.getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{k_FaceTupleCount}, std::vector<usize>{k_FaceCompCount}, 0);
  auto* dataArrPtr = IGeometry::SharedFaceListType::Create(*dataStructurePtr, "Faces", std::move(dataStore), parentId);
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<IGeometry::SharedFaceListType*>(dataArrPtr);
}

} // namespace

TEST_CASE("SimplnxCore::TriangleCentroidFilter", "[SimplnxCore][TriangleCentroidFilter]")
{
  unit_test::LoadPlugins();

  DataStructure dataStructure;
  TriangleGeom& acuteTriangle = *TriangleGeom::Create(dataStructure, k_TriangleGeometryName);
  AttributeMatrix* faceDataPtr = AttributeMatrix::Create(dataStructure, INodeGeometry2D::k_FaceAttributeMatrixName, {k_FaceTupleCount}, acuteTriangle.getId());
  acuteTriangle.setFaceAttributeMatrix(*faceDataPtr);
  AttributeMatrix* vertDataPtr = AttributeMatrix::Create(dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, {k_VertexTupleCount}, acuteTriangle.getId());
  acuteTriangle.setVertexAttributeMatrix(*vertDataPtr);
  const auto* vertexListPtr = CreateVertexList(acuteTriangle, vertDataPtr->getId());
  const auto* facesListPtr = CreateFaceList(acuteTriangle, faceDataPtr->getId());
  auto underlyingStoreV = vertexListPtr->getDataStorePtr().lock();
  auto underlyingStoreF = facesListPtr->getDataStorePtr().lock();

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
  acuteTriangle.setFaceList(*facesListPtr);
  acuteTriangle.setVertices(*vertexListPtr);

  const TriangleCentroidFilter filter;
  Arguments args;

  DataPath geometryPath = dataStructure.getDataPathsForId(acuteTriangle.getId())[0];
  DataPath triangleCentroidsDataPath = geometryPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName).createChildPath(k_CentroidsName);

  // Create default Parameters for the filter.
  args.insertOrAssign(TriangleCentroidFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(TriangleCentroidFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  std::vector<FloatVec3Type> centroids = {{0.5F, 0.333333F, 0.0F}, {3.0F, 0.333333, 0.0F}, {6.66667, 0.0F, 0.0F}, {9.0F, 0.0F, 0.0F}};

  const auto& calculatedCentroids = dataStructure.getDataRefAs<Float64ArrayType>(triangleCentroidsDataPath);

  for(size_t t = 0; t < 4; t++)
  {
    for(size_t c = 0; c < 3; c++)
    {
      auto result1 = fabs(calculatedCentroids[t * 3 + c] - centroids[t][c]);
      REQUIRE(result1 < ::k_MaxDifference);
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::TriangleCentroidFilter: SIMPL Backwards Compatibility", "[SimplnxCore][TriangleCentroidFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "TriangleCentroidFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "TriangleCentroidFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<TriangleCentroidFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<std::string>(TriangleCentroidFilter::k_CentroidsArrayName_Key) == "TestArray");
      CHECK(args.value<DataPath>(TriangleCentroidFilter::k_TriGeometryDataPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
