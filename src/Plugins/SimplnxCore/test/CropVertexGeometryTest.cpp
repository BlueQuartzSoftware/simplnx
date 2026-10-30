#include "SimplnxCore/Filters/CropVertexGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
constexpr uint64 k_TupleCount = 8;
constexpr StringLiteral k_VertexAttributeMatrixName = "VertexData";
const DataPath k_VertexGeomPath{std::vector<std::string>{"VertexGeom"}};
const DataPath k_VertexDataPath = k_VertexGeomPath.createChildPath(k_VertexAttributeMatrixName);
const DataPath k_CroppedGeomPath{std::vector<std::string>{"Cropped VertexGeom"}};
const std::vector<DataPath> targetDataArrays{k_VertexDataPath.createChildPath("DataArray")};

DataStructure createTestData()
{
  DataStructure dataStructure;
  auto* vertexGeom = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*vertexArray);

  auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, k_VertexAttributeMatrixName, {k_TupleCount}, vertexGeom->getId());
  vertexGeom->setVertexAttributeMatrix(*vertexAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "DataArray", {k_TupleCount}, {1}, vertexAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    dataStore[i] = i;
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }

  return dataStructure;
}
} // namespace

TEST_CASE("SimplnxCore::CropVertexGeometryFilter(Instantiate)", "[SimplnxCore][CropVertexGeometryFilter]")
{
  UnitTest::LoadPlugins();

  static const std::vector<float32> k_MinPos{0, 0, 0};
  static const std::vector<float32> k_MaxPos{5, 6, 7};

  CropVertexGeometryFilter filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  args.insert(CropVertexGeometryFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insert(CropVertexGeometryFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(k_CroppedGeomPath));
  args.insert(CropVertexGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexAttributeMatrixName));
  args.insert(CropVertexGeometryFilter::k_MinPos_Key, std::make_any<std::vector<float32>>(k_MinPos));
  args.insert(CropVertexGeometryFilter::k_MaxPos_Key, std::make_any<std::vector<float32>>(k_MaxPos));
  args.insert(CropVertexGeometryFilter::k_TargetArrayPaths_Key, std::make_any<std::vector<DataPath>>(targetDataArrays));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropVertexGeometryFilter(Data)", "[SimplnxCore][CropVertexGeometryFilter]")
{
  UnitTest::LoadPlugins();

  static const std::vector<float32> k_MinPos{0, 0, 0};
  static const std::vector<float32> k_MaxPos{5, 6, 7};

  CropVertexGeometryFilter filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  args.insert(CropVertexGeometryFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insert(CropVertexGeometryFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(k_CroppedGeomPath));
  args.insert(CropVertexGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexAttributeMatrixName));
  args.insert(CropVertexGeometryFilter::k_MinPos_Key, std::make_any<std::vector<float32>>(k_MinPos));
  args.insert(CropVertexGeometryFilter::k_MaxPos_Key, std::make_any<std::vector<float32>>(k_MaxPos));
  args.insert(CropVertexGeometryFilter::k_TargetArrayPaths_Key, std::make_any<std::vector<DataPath>>(targetDataArrays));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto* croppedGeom = dataStructure.getDataAs<VertexGeom>(k_CroppedGeomPath);
  REQUIRE(croppedGeom != nullptr);

  auto* croppedVertices = croppedGeom->getVertices();
  REQUIRE(croppedVertices != nullptr);

  auto* croppedData = dataStructure.getDataAs<Int32Array>(k_CroppedGeomPath.createChildPath(k_VertexAttributeMatrixName).createChildPath("DataArray"));
  REQUIRE(croppedData != nullptr);

  REQUIRE(croppedData->getNumberOfTuples() == 6);
  REQUIRE(croppedVertices->getNumberOfTuples() == 6);

  auto& croppedDataStore = croppedData->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    REQUIRE(croppedDataStore[i] == i);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropVertexGeometryFilter(PreserveComponentShapes)", "[SimplnxCore][CropVertexGeometryFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* vertexGeom = VertexGeom::Create(dataStructure, "VertexGeom");
  REQUIRE(vertexGeom != nullptr);
  auto* vertices = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {3}, {3}, vertexGeom->getId());
  REQUIRE(vertices != nullptr);
  vertexGeom->setVertices(*vertices);
  auto* vertexData = AttributeMatrix::Create(dataStructure, k_VertexAttributeMatrixName, {3}, vertexGeom->getId());
  REQUIRE(vertexData != nullptr);
  vertexGeom->setVertexAttributeMatrix(*vertexData);

  // In the box [0, 1]^3, vertex 0 is outside; vertices 1 and 2 are kept in that order.
  const std::array<float32, 9> coordinates = {-1, 0, 0, 0, 0, 0, 1, 1, 1};
  auto coordinatesWriteResult = vertices->getDataStoreRef().copyFromBuffer(0, coordinates);
  SIMPLNX_RESULT_REQUIRE_VALID(coordinatesWriteResult);
  auto* matrices = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Matrices", {3}, {3, 3}, vertexData->getId());
  auto* vectors = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vectors", {3}, {2}, vertexData->getId());
  REQUIRE(matrices != nullptr);
  REQUIRE(vectors != nullptr);
  const std::array<int32, 27> matrixValues = {-9, -8, -7, -6, -5, -4, -3, -2, -1, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 24, 25, 26, 27, 28, 29};
  const std::array<float32, 6> vectorValues = {-2, -1, 10, 11, 20, 21};
  auto matricesWriteResult = matrices->getDataStoreRef().copyFromBuffer(0, matrixValues);
  SIMPLNX_RESULT_REQUIRE_VALID(matricesWriteResult);
  auto vectorsWriteResult = vectors->getDataStoreRef().copyFromBuffer(0, vectorValues);
  SIMPLNX_RESULT_REQUIRE_VALID(vectorsWriteResult);

  CropVertexGeometryFilter filter;
  Arguments args;
  args.insert(CropVertexGeometryFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insert(CropVertexGeometryFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(k_CroppedGeomPath));
  args.insert(CropVertexGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexAttributeMatrixName));
  args.insert(CropVertexGeometryFilter::k_MinPos_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0, 0, 0}));
  args.insert(CropVertexGeometryFilter::k_MaxPos_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1, 1, 1}));
  args.insert(CropVertexGeometryFilter::k_TargetArrayPaths_Key,
              std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_VertexDataPath.createChildPath("Matrices"), k_VertexDataPath.createChildPath("Vectors")}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  const auto croppedDataPath = k_CroppedGeomPath.createChildPath(k_VertexAttributeMatrixName);
  const auto* croppedMatrices = dataStructure.getDataAs<Int32Array>(croppedDataPath.createChildPath("Matrices"));
  const auto* croppedVectors = dataStructure.getDataAs<Float32Array>(croppedDataPath.createChildPath("Vectors"));
  const auto* croppedGeom = dataStructure.getDataAs<VertexGeom>(k_CroppedGeomPath);
  REQUIRE(croppedMatrices != nullptr);
  REQUIRE(croppedVectors != nullptr);
  REQUIRE(croppedGeom != nullptr);
  REQUIRE(croppedGeom->getVertices() != nullptr);
  REQUIRE(croppedMatrices->getNumberOfTuples() == 2);
  REQUIRE(croppedVectors->getNumberOfTuples() == 2);
  REQUIRE(croppedGeom->getNumberOfVertices() == 2);
  CHECK(croppedMatrices->getComponentShape() == ShapeType{3, 3});
  CHECK(croppedVectors->getComponentShape() == ShapeType{2});

  // Literal oracle: discard the first tuple and retain the two boundary vertices.
  const std::array<int32, 18> expectedMatrices = {11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 24, 25, 26, 27, 28, 29};
  const std::array<float32, 4> expectedVectors = {10, 11, 20, 21};
  const std::array<float32, 6> expectedCoordinates = {0, 0, 0, 1, 1, 1};
  std::array<int32, 18> actualMatrices{};
  std::array<float32, 4> actualVectors{};
  std::array<float32, 6> actualCoordinates{};
  auto matricesReadResult = croppedMatrices->getDataStoreRef().copyIntoBuffer(0, actualMatrices);
  SIMPLNX_RESULT_REQUIRE_VALID(matricesReadResult);
  auto vectorsReadResult = croppedVectors->getDataStoreRef().copyIntoBuffer(0, actualVectors);
  SIMPLNX_RESULT_REQUIRE_VALID(vectorsReadResult);
  auto coordinatesReadResult = croppedGeom->getVertices()->getDataStoreRef().copyIntoBuffer(0, actualCoordinates);
  SIMPLNX_RESULT_REQUIRE_VALID(coordinatesReadResult);
  CHECK(actualMatrices == expectedMatrices);
  CHECK(actualVectors == expectedVectors);
  CHECK(actualCoordinates == expectedCoordinates);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropVertexGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CropVertexGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CropVertexGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CropVertexGeometryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CropVertexGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (FloatToVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatToVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(CropVertexGeometryFilter::k_SelectedVertexGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(CropVertexGeometryFilter::k_CreatedVertexGeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
