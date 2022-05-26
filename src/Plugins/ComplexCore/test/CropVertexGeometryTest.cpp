#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CropVertexGeometry.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

namespace
{
constexpr uint64 k_TupleCount = 8;
constexpr StringLiteral k_CroppedGroupName = "Cropped Geom";
const DataPath k_VertexGeomPath{std::vector<std::string>{"VertexGeom"}};
const DataPath k_CroppedGeomPath{std::vector<std::string>{"Cropped VertexGeom"}};
const std::vector<DataPath> targetDataArrays{k_VertexGeomPath.createChildPath("DataArray")};

DataStructure createTestData()
{
  DataStructure dataStructure;
  auto* vertexGeom = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*vertexArray);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "DataArray", {k_TupleCount}, {1}, vertexGeom->getId());
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

TEST_CASE("ComplexCore::CropVertexGeometry(Instantiate)", "[ComplexCore][CropVertexGeometry]")
{
  static const std::vector<float32> k_MinPos{0, 0, 0};
  static const std::vector<float32> k_MaxPos{5, 6, 7};

  CropVertexGeometry filter;
  DataStructure ds = createTestData();
  Arguments args;

  args.insert(CropVertexGeometry::k_VertexGeom_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insert(CropVertexGeometry::k_CroppedGeom_Key, std::make_any<DataPath>(k_CroppedGeomPath));
  args.insert(CropVertexGeometry::k_MinPos_Key, std::make_any<std::vector<float32>>(k_MinPos));
  args.insert(CropVertexGeometry::k_MaxPos_Key, std::make_any<std::vector<float32>>(k_MaxPos));
  args.insert(CropVertexGeometry::k_TargetArrayPaths_Key, std::make_any<std::vector<DataPath>>(targetDataArrays));
  args.insert(CropVertexGeometry::k_CroppedGroupName_Key, std::make_any<std::string>(k_CroppedGroupName));

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("ComplexCore::CropVertexGeometry(Data)", "[ComplexCore][CropVertexGeometry]")
{
  static const std::vector<float32> k_MinPos{0, 0, 0};
  static const std::vector<float32> k_MaxPos{5, 6, 7};

  CropVertexGeometry filter;
  DataStructure ds = createTestData();
  Arguments args;

  args.insert(CropVertexGeometry::k_VertexGeom_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insert(CropVertexGeometry::k_CroppedGeom_Key, std::make_any<DataPath>(k_CroppedGeomPath));
  args.insert(CropVertexGeometry::k_MinPos_Key, std::make_any<std::vector<float32>>(k_MinPos));
  args.insert(CropVertexGeometry::k_MaxPos_Key, std::make_any<std::vector<float32>>(k_MaxPos));
  args.insert(CropVertexGeometry::k_TargetArrayPaths_Key, std::make_any<std::vector<DataPath>>(targetDataArrays));
  args.insert(CropVertexGeometry::k_CroppedGroupName_Key, std::make_any<std::string>(k_CroppedGroupName));

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  auto* croppedGeom = ds.getDataAs<VertexGeom>(k_CroppedGeomPath);
  REQUIRE(croppedGeom != nullptr);

  auto* croppedVertices = croppedGeom->getVertices();
  REQUIRE(croppedVertices != nullptr);

  auto* croppedData = ds.getDataAs<Int32Array>(k_CroppedGeomPath.createChildPath(k_CroppedGroupName).createChildPath("DataArray"));
  REQUIRE(croppedData != nullptr);

  REQUIRE(croppedData->getNumberOfTuples() == 6);
  REQUIRE(croppedVertices->getNumberOfTuples() == 6);

  auto& croppedDataStore = croppedData->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    REQUIRE(croppedDataStore[i] == i);
  }
}
