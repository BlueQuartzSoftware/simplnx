#include "SimplnxCore/Filters/RemoveFlaggedTrianglesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
fs::path KBaseDataFilePath = fs::path(fmt::format("{}/remove_flagged_elements_data/remove_flagged_triangles_data.dream3d", nx::core::unit_test::k_TestFilesDir));

constexpr StringLiteralType k_RegionIdsName = "Region Ids";
constexpr StringLiteralType k_FaceNormalsName = "Face Normals";
constexpr StringLiteralType k_VertexListName = "SharedVertexList";
constexpr StringLiteralType k_TriangleListName = "SharedTriList";

const DataPath k_TriangleGeomPath({"TriangleGeometry"});
const DataPath k_MaskPath = k_TriangleGeomPath.createChildPath(Constants::k_FaceDataWithSpaces).createChildPath(Constants::k_Mask);
const DataPath k_RegionIdsPath = k_TriangleGeomPath.createChildPath(Constants::k_FaceDataWithSpaces).createChildPath(k_RegionIdsName);
const DataPath k_FaceNormalsPath = k_TriangleGeomPath.createChildPath(Constants::k_FaceDataWithSpaces).createChildPath(k_FaceNormalsName);
const DataPath k_ReducedGeomPath({"ReducedGeometry"});
const DataPath k_ExemplarReducedGeomPath({"ExemplarReducedGeometry"});
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedTrianglesFilter: Test Algorithm", "[SimplnxCore][RemoveFlaggedTrianglesFilter]")
{
  UnitTest::LoadPlugins();

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "remove_flagged_elements_data.tar.gz", "remove_flagged_elements_data");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  DataStructure dataStructure = UnitTest::LoadDataStructure(KBaseDataFilePath);

  {
    // Instantiate the filter and an Arguments Object
    RemoveFlaggedTrianglesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriangleGeomPath));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(::k_MaskPath));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(::k_ReducedGeomPath));

    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_TriangleDataHandling_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_TriangleDataSelectedArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{::k_FaceNormalsPath, ::k_RegionIdsPath}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(Constants::k_FaceDataWithSpaces).createChildPath(::k_RegionIdsName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(Constants::k_FaceDataWithSpaces).createChildPath(::k_RegionIdsName);

    UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(Constants::k_FaceDataWithSpaces).createChildPath(::k_FaceNormalsName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(Constants::k_FaceDataWithSpaces).createChildPath(::k_FaceNormalsName);

    UnitTest::CompareDataArrays<float64>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(::k_VertexListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_VertexListName);

    UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(::k_TriangleListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_TriangleListName);

    UnitTest::CompareDataArrays<uint64>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedTrianglesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RemoveFlaggedTrianglesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RemoveFlaggedTrianglesFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<RemoveFlaggedTrianglesFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<DataPath>(RemoveFlaggedTrianglesFilter::k_SelectedTriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RemoveFlaggedTrianglesFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "FaceData", "Mask"}));
      CHECK(args.value<DataPath>(RemoveFlaggedTrianglesFilter::k_CreatedTriangleGeometryPath_Key) == DataPath({"ReducedTriangleGeometry"}));
    }
  }
}

TEST_CASE("SimplnxCore::RemoveFlaggedTrianglesFilter: Reduced geometry preserves the face attribute matrix name", "[SimplnxCore][RemoveFlaggedTrianglesFilter]")
{
  const bool withEdges = GENERATE(false, true);
  CAPTURE(withEdges);
  DataStructure dataStructure;
  auto* geomPtr = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(geomPtr != nullptr);
  auto* verticesPtr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Vertices", {3}, {3}, geomPtr->getId());
  geomPtr->setVertices(*verticesPtr);
  geomPtr->setVertexCoordinate(0, {0, 0, 0});
  geomPtr->setVertexCoordinate(1, {2, 0, 0});
  geomPtr->setVertexCoordinate(2, {0, 3, 0});
  auto* facesPtr = DataArray<IGeometry::MeshIndexType>::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "Faces", {1}, {3}, geomPtr->getId());
  (*facesPtr)[0] = 0;
  (*facesPtr)[1] = 1;
  (*facesPtr)[2] = 2;
  geomPtr->setFaceList(*facesPtr);
  auto* faceAmPtr = AttributeMatrix::Create(dataStructure, "FacesCustom", {1}, geomPtr->getId());
  geomPtr->setFaceAttributeMatrix(*faceAmPtr);
  auto* valuesPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Value", {1}, {1}, faceAmPtr->getId());
  (*valuesPtr)[0] = 17;
  auto* maskPtr = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {1}, {1}, faceAmPtr->getId());
  (*maskPtr)[0] = false;
  if(withEdges)
  {
    auto* edgeAmPtr = AttributeMatrix::Create(dataStructure, "EdgesCustom", {0}, geomPtr->getId());
    geomPtr->setEdgeAttributeMatrix(*edgeAmPtr);
  }
  const DataPath inputPath({"Triangles"});
  const DataPath facePath = inputPath.createChildPath("FacesCustom");
  const DataPath outputPath({"Reduced"});
  RemoveFlaggedTrianglesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(facePath.createChildPath("Mask")));
  args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(outputPath));
  args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_TriangleDataHandling_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_TriangleDataSelectedArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{facePath.createChildPath("Value")}));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* reducedPtr = dataStructure.getDataAs<TriangleGeom>(outputPath);
  REQUIRE(reducedPtr != nullptr);
  CHECK(reducedPtr->getNumberOfFaces() == 1);
  CHECK(reducedPtr->getNumberOfVertices() == 3);
  const auto* outputAmPtr = dataStructure.getDataAs<AttributeMatrix>(outputPath.createChildPath("FacesCustom"));
  REQUIRE(outputAmPtr != nullptr);
  CHECK(reducedPtr->getFaceAttributeMatrix() == outputAmPtr);
  const auto* outputPtr = dataStructure.getDataAs<Int32Array>(outputPath.createChildPath("FacesCustom").createChildPath("Value"));
  REQUIRE(outputPtr != nullptr);
  // A false removal mask retains the single face and its scalar value 17.
  CHECK((*outputPtr)[0] == 17);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
