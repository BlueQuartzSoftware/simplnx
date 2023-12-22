#include "SimplnxCore/Filters/CropVertexGeometry.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
constexpr uint64 k_TupleCount = 8;
constexpr StringLiteral k_VertexDataName = "VertexData";
const DataPath k_VertexGeomPath{std::vector<std::string>{"VertexGeom"}};
const DataPath k_VertexDataPath = k_VertexGeomPath.createChildPath(k_VertexDataName);
const DataPath k_CroppedGeomPath{std::vector<std::string>{"Cropped VertexGeom"}};
const std::vector<DataPath> targetDataArrays{k_VertexDataPath.createChildPath("DataArray")};

DataStructure createTestData()
{
  DataStructure dataStructure;
  auto* vertexGeom = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*vertexArray);

  auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, k_VertexDataName, {k_TupleCount}, vertexGeom->getId());
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

TEST_CASE("SimplnxCore::CropVertexGeometry(Instantiate)", "[SimplnxCore][CropVertexGeometry]")
{
  static const std::vector<float32> k_MinPos{0, 0, 0};
  static const std::vector<float32> k_MaxPos{5, 6, 7};

  CropVertexGeometry filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  args.insert(CropVertexGeometry::k_VertexGeom_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insert(CropVertexGeometry::k_CroppedGeom_Key, std::make_any<DataPath>(k_CroppedGeomPath));
  args.insert(CropVertexGeometry::k_VertexDataName_Key, std::make_any<std::string>(k_VertexDataName));
  args.insert(CropVertexGeometry::k_MinPos_Key, std::make_any<std::vector<float32>>(k_MinPos));
  args.insert(CropVertexGeometry::k_MaxPos_Key, std::make_any<std::vector<float32>>(k_MaxPos));
  args.insert(CropVertexGeometry::k_TargetArrayPaths_Key, std::make_any<std::vector<DataPath>>(targetDataArrays));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("SimplnxCore::CropVertexGeometry(Data)", "[SimplnxCore][CropVertexGeometry]")
{
  static const std::vector<float32> k_MinPos{0, 0, 0};
  static const std::vector<float32> k_MaxPos{5, 6, 7};

  CropVertexGeometry filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  args.insert(CropVertexGeometry::k_VertexGeom_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insert(CropVertexGeometry::k_CroppedGeom_Key, std::make_any<DataPath>(k_CroppedGeomPath));
  args.insert(CropVertexGeometry::k_VertexDataName_Key, std::make_any<std::string>(k_VertexDataName));
  args.insert(CropVertexGeometry::k_MinPos_Key, std::make_any<std::vector<float32>>(k_MinPos));
  args.insert(CropVertexGeometry::k_MaxPos_Key, std::make_any<std::vector<float32>>(k_MaxPos));
  args.insert(CropVertexGeometry::k_TargetArrayPaths_Key, std::make_any<std::vector<DataPath>>(targetDataArrays));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto* croppedGeom = dataStructure.getDataAs<VertexGeom>(k_CroppedGeomPath);
  REQUIRE(croppedGeom != nullptr);

  auto* croppedVertices = croppedGeom->getVertices();
  REQUIRE(croppedVertices != nullptr);

  auto* croppedData = dataStructure.getDataAs<Int32Array>(k_CroppedGeomPath.createChildPath(k_VertexDataName).createChildPath("DataArray"));
  REQUIRE(croppedData != nullptr);

  REQUIRE(croppedData->getNumberOfTuples() == 6);
  REQUIRE(croppedVertices->getNumberOfTuples() == 6);

  auto& croppedDataStore = croppedData->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    REQUIRE(croppedDataStore[i] == i);
  }
}
