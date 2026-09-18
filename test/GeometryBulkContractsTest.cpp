#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace nx::core;

namespace
{
constexpr int32 k_InjectedStoreError = -98761;

/**
 * @class FailingResidentStore
 * @brief Keeps real resident values while selected operations return a known error.
 * @tparam T Stored scalar type.
 */
template <typename T>
class FailingResidentStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  bool rejectResize = false;
  usize failingReadCall = 0;
  usize failingWriteCall = 0;
  mutable usize readCalls = 0;
  usize writeCalls = 0;

  /**
   * @brief Returns a selected resize failure without changing resident values.
   * @param shape Requested tuple dimensions.
   * @return Injected error or the real resident resize result.
   */
  Result<> resizeTuples(const ShapeType& shape) override
  {
    return rejectResize ? MakeErrorResult(k_InjectedStoreError, "Injected resident resize failure") : DataStore<T>::resizeTuples(shape);
  }

  /**
   * @brief Fails one selected bulk read and performs all other reads normally.
   * @param startIndex First flat value index.
   * @param buffer Receives the resident values.
   * @return Injected error or the real resident read result.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++readCalls;
    return readCalls == failingReadCall ? MakeErrorResult(k_InjectedStoreError, "Injected resident read failure") : DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  /**
   * @brief Fails one selected bulk write and performs all other writes normally.
   * @param startIndex First flat value index.
   * @param buffer Values to write.
   * @return Injected error or the real resident write result.
   */
  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++writeCalls;
    return writeCalls == failingWriteCall ? MakeErrorResult(k_InjectedStoreError, "Injected resident write failure") : DataStore<T>::copyFromBuffer(startIndex, buffer);
  }
};

// The second cell shares a complete boundary with the first. The third shares
// only vertex zero, except for edges where one shared vertex is a full boundary.
struct MeshCase
{
  IGeometry::Type type;
  usize vertexCount;
  std::vector<std::vector<uint64>> cells;
  std::vector<std::vector<uint64>> incidentCells;
  std::set<std::vector<uint64>> edges;
  std::set<std::vector<uint64>> sharedEdges;
  std::set<std::vector<uint64>> faces;
  std::set<std::vector<uint64>> sharedFaces;
};

/**
 * @brief Supplies hand-derived incidence and topology for small connected cells.
 * @param type Selects the cell topology.
 * @return Three-cell fixture with an isolated vertex; extraction expectations use the first two cells.
 */
MeshCase MakeMeshCase(IGeometry::Type type)
{
  switch(type)
  {
  case IGeometry::Type::Edge:
    return {type, 6, {{0, 1}, {1, 2}, {3, 4}}, {{0}, {0, 1}, {1}, {2}, {2}, {}}, {}, {}, {}, {}};
  case IGeometry::Type::Triangle:
    return {type, 7, {{0, 1, 2}, {1, 3, 2}, {0, 4, 5}}, {{0, 2}, {0, 1}, {0, 1}, {1}, {2}, {2}, {}}, {{0, 1}, {0, 2}, {1, 2}, {1, 3}, {2, 3}}, {{1, 2}}, {}, {}};
  case IGeometry::Type::Quad:
    return {type, 10, {{0, 1, 2, 3}, {1, 4, 5, 2}, {0, 6, 7, 8}}, {{0, 2}, {0, 1}, {0, 1}, {0}, {1}, {1}, {2}, {2}, {2}, {}}, {{0, 1}, {0, 3}, {1, 2}, {2, 3}, {1, 4}, {4, 5}, {2, 5}}, {{1, 2}},
            {},   {}};
  case IGeometry::Type::Tetrahedral:
    return {type,
            9,
            {{0, 1, 2, 3}, {0, 2, 1, 4}, {0, 5, 6, 7}},
            {{0, 1, 2}, {0, 1}, {0, 1}, {0}, {1}, {2}, {2}, {2}, {}},
            {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}, {0, 4}, {1, 4}, {2, 4}},
            {{0, 1}, {0, 2}, {1, 2}},
            {{0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}, {0, 1, 4}, {0, 2, 4}, {1, 2, 4}},
            {{0, 1, 2}}};
  case IGeometry::Type::Hexahedral:
    return {type,
            20,
            {{0, 1, 2, 3, 4, 5, 6, 7}, {1, 8, 9, 2, 5, 10, 11, 6}, {0, 12, 13, 14, 15, 16, 17, 18}},
            {{0, 2}, {0, 1}, {0, 1}, {0}, {0}, {0, 1}, {0, 1}, {0}, {1}, {1}, {1}, {1}, {2}, {2}, {2}, {2}, {2}, {2}, {2}, {}},
            {{0, 1}, {1, 2}, {2, 3}, {0, 3}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {4, 7}, {1, 8}, {8, 9}, {2, 9}, {8, 10}, {9, 11}, {5, 10}, {10, 11}, {6, 11}},
            {{1, 2}, {1, 5}, {2, 6}, {5, 6}},
            {{0, 1, 4, 5}, {1, 2, 5, 6}, {2, 3, 6, 7}, {0, 3, 4, 7}, {0, 1, 2, 3}, {4, 5, 6, 7}, {1, 5, 8, 10}, {8, 9, 10, 11}, {2, 6, 9, 11}, {1, 2, 8, 9}, {5, 6, 10, 11}},
            {{1, 2, 5, 6}}};
  default:
    FAIL("Unsupported analytical mesh fixture");
    return {};
  }
}

/**
 * @brief Creates a resident mesh whose coordinates are unused by connectivity operations.
 * @param dataStructure Owns the geometry and arrays.
 * @param fixture Supplies explicit connectivity.
 * @param cellCount Number of leading fixture cells to include.
 * @return Created geometry.
 */
INodeGeometry1D* CreateMesh(DataStructure& dataStructure, const MeshCase& fixture, usize cellCount)
{
  INodeGeometry1D* geometryPtr = nullptr;
  switch(fixture.type)
  {
  case IGeometry::Type::Edge:
    geometryPtr = EdgeGeom::Create(dataStructure, "Mesh");
    break;
  case IGeometry::Type::Triangle:
    geometryPtr = TriangleGeom::Create(dataStructure, "Mesh");
    break;
  case IGeometry::Type::Quad:
    geometryPtr = QuadGeom::Create(dataStructure, "Mesh");
    break;
  case IGeometry::Type::Tetrahedral:
    geometryPtr = TetrahedralGeom::Create(dataStructure, "Mesh");
    break;
  case IGeometry::Type::Hexahedral:
    geometryPtr = HexahedralGeom::Create(dataStructure, "Mesh");
    break;
  default:
    FAIL("Unsupported analytical mesh fixture");
  }
  REQUIRE(geometryPtr != nullptr);
  auto* verticesPtr = Float32Array::Create(dataStructure, "Vertices", std::make_shared<DataStore<float32>>(ShapeType{fixture.vertexCount}, ShapeType{3}, 0.0F), geometryPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  geometryPtr->setVertices(*verticesPtr);

  const usize components = fixture.cells.front().size();
  auto connectivityStore = std::make_shared<FailingResidentStore<uint64>>(ShapeType{cellCount}, ShapeType{components}, uint64{0});
  for(usize cellIndex = 0; cellIndex < cellCount; ++cellIndex)
  {
    for(usize componentIndex = 0; componentIndex < components; ++componentIndex)
    {
      connectivityStore->setValue(cellIndex * components + componentIndex, fixture.cells[cellIndex][componentIndex]);
    }
  }
  auto* connectivityPtr = UInt64Array::Create(dataStructure, "Connectivity", connectivityStore, geometryPtr->getId());
  REQUIRE(connectivityPtr != nullptr);
  if(auto* volumePtr = dynamic_cast<INodeGeometry3D*>(geometryPtr); volumePtr != nullptr)
  {
    volumePtr->setPolyhedraList(*connectivityPtr);
  }
  else if(auto* surfacePtr = dynamic_cast<INodeGeometry2D*>(geometryPtr); surfacePtr != nullptr)
  {
    surfacePtr->setFaceList(*connectivityPtr);
  }
  else
  {
    geometryPtr->setEdgeList(*connectivityPtr);
  }
  return geometryPtr;
}

/**
 * @brief Checks every dynamic list against explicit element IDs, including empty lists.
 * @param listsPtr Actual incidence or neighbor lists.
 * @param expected Hand-derived element IDs in ascending order.
 */
void RequireLists(const INodeGeometry1D::ElementDynamicList* listsPtr, const std::vector<std::vector<uint64>>& expected)
{
  REQUIRE(listsPtr != nullptr);
  REQUIRE(listsPtr->size() == expected.size());
  for(usize listIndex = 0; listIndex < expected.size(); ++listIndex)
  {
    CAPTURE(listIndex);
    const usize count = listsPtr->getNumberOfElements(listIndex);
    REQUIRE(count == expected[listIndex].size());
    std::vector<uint64> actual;
    if(count > 0)
    {
      const auto* values = listsPtr->getElementListPointer(listIndex);
      REQUIRE(values != nullptr);
      actual.assign(values, values + count);
    }
    std::sort(actual.begin(), actual.end());
    CHECK(actual == expected[listIndex]);
  }
}

/**
 * @brief Compares topology independently of tuple ordering or orientation.
 * @param arrayPtr Extracted edge or face array.
 * @param expected Hand-derived canonical records.
 */
void RequireTopology(const UInt64Array* arrayPtr, const std::set<std::vector<uint64>>& expected)
{
  REQUIRE(arrayPtr != nullptr);
  REQUIRE(arrayPtr->getNumberOfTuples() == expected.size());
  std::set<std::vector<uint64>> actual;
  for(usize tupleIndex = 0; tupleIndex < arrayPtr->getNumberOfTuples(); ++tupleIndex)
  {
    std::vector<uint64> record(arrayPtr->getNumberOfComponents());
    for(usize componentIndex = 0; componentIndex < record.size(); ++componentIndex)
    {
      record[componentIndex] = arrayPtr->getValue(tupleIndex * record.size() + componentIndex);
    }
    std::sort(record.begin(), record.end());
    actual.insert(std::move(record));
  }
  CHECK(actual == expected);
}

void RequireInjectedError(const Result<>& result, const std::string& operation)
{
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == k_InjectedStoreError);
  CHECK(result.errors().front().message.find(operation) != std::string::npos);
}

// A completed first calculation establishes a real cached ID before each failure.
RectGridGeom* CreateRectGrid(DataStructure& dataStructure)
{
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* geometryPtr = RectGridGeom::Create(dataStructure, "Grid");
  REQUIRE(geometryPtr != nullptr);
  geometryPtr->setDimensions({2, 2, 2});
  std::array<Float32Array*, 3> bounds{};
  const std::array<std::string, 3> names = {"X", "Y", "Z"};
  for(usize axisIndex = 0; axisIndex < bounds.size(); ++axisIndex)
  {
    auto store = std::make_shared<FailingResidentStore<float32>>(ShapeType{3}, ShapeType{1}, 0.0F);
    store->setValue(1, 1.0F);
    store->setValue(2, 3.0F);
    bounds[axisIndex] = Float32Array::Create(dataStructure, names[axisIndex], store, geometryPtr->getId());
    REQUIRE(bounds[axisIndex] != nullptr);
  }
  geometryPtr->setBounds(bounds[0], bounds[1], bounds[2]);
  REQUIRE(geometryPtr->findElementSizes(false).valid());
  REQUIRE(geometryPtr->getElementSizesId().has_value());
  return geometryPtr;
}
} // namespace

TEST_CASE("Geometry list resize preserves resident failures and context", "[simplnx][GeometryBulkContracts]")
{
  DataStructure dataStructure;
  auto* geometryPtr = HexahedralGeom::Create(dataStructure, "Resize Geometry");
  REQUIRE(geometryPtr != nullptr);
  const usize listKind = GENERATE(0, 1, 2, 3);
  CAPTURE(listKind);
  Result<> result;
  const std::array<std::string, 4> listNames = {"vertex", "edge", "face", "polyhedra"};
  constexpr usize k_TargetTuples = 19;
  if(listKind == 0)
  {
    auto store = std::make_shared<FailingResidentStore<float32>>(ShapeType{2}, ShapeType{3}, 13.0F);
    store->rejectResize = true;
    auto* arrayPtr = Float32Array::Create(dataStructure, "Vertices", store, geometryPtr->getId());
    REQUIRE(arrayPtr != nullptr);
    geometryPtr->setVertices(*arrayPtr);
    result = geometryPtr->resizeVertexList(k_TargetTuples);
    CHECK(store->getTupleShape() == ShapeType{2});
    REQUIRE(store->getSize() == 6);
    for(usize valueIndex = 0; valueIndex < store->getSize(); ++valueIndex)
    {
      CHECK(store->getValue(valueIndex) == 13.0F);
    }
  }
  else
  {
    const usize components = listKind == 1 ? 2 : (listKind == 2 ? 4 : 8);
    auto store = std::make_shared<FailingResidentStore<uint64>>(ShapeType{2}, ShapeType{components}, uint64{13});
    store->rejectResize = true;
    auto* arrayPtr = UInt64Array::Create(dataStructure, "Indices", store, geometryPtr->getId());
    REQUIRE(arrayPtr != nullptr);
    if(listKind == 1)
    {
      geometryPtr->setEdgeList(*arrayPtr);
      result = geometryPtr->resizeEdgeList(k_TargetTuples);
    }
    else if(listKind == 2)
    {
      geometryPtr->setFaceList(*arrayPtr);
      result = geometryPtr->resizeFaceList(k_TargetTuples);
    }
    else
    {
      geometryPtr->setPolyhedraList(*arrayPtr);
      result = geometryPtr->resizePolyhedraList(k_TargetTuples);
    }
    CHECK(store->getTupleShape() == ShapeType{2});
    REQUIRE(store->getSize() == 2 * components);
    for(usize valueIndex = 0; valueIndex < store->getSize(); ++valueIndex)
    {
      CHECK(store->getValue(valueIndex) == 13);
    }
  }
  RequireInjectedError(result, "resize");
  CHECK(result.errors().front().message.find("Resize Geometry") != std::string::npos);
  CHECK(result.errors().front().message.find(listNames[listKind]) != std::string::npos);
  CHECK(result.errors().front().message.find("19") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("Geometry connectivity computes exact incidence and complete-boundary neighbors", "[simplnx][GeometryBulkContracts]")
{
  const auto geometryType = GENERATE(IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad, IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral);
  const bool empty = GENERATE(false, true);
  CAPTURE(static_cast<int32>(geometryType), empty);
  const MeshCase fixture = MakeMeshCase(geometryType);
  DataStructure dataStructure;
  auto* geometryPtr = CreateMesh(dataStructure, fixture, empty ? 0 : fixture.cells.size());
  REQUIRE(geometryPtr->findElementsContainingVert(false).valid());
  RequireLists(geometryPtr->getElementsContainingVert(), empty ? std::vector<std::vector<uint64>>(fixture.vertexCount) : fixture.incidentCells);
  REQUIRE(geometryPtr->findElementNeighbors(false).valid());
  RequireLists(geometryPtr->getElementNeighbors(), empty ? std::vector<std::vector<uint64>>{} : std::vector<std::vector<uint64>>{{1}, {0}, {}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("Geometry extraction computes exact unique and unshared topology", "[simplnx][GeometryBulkContracts]")
{
  const auto geometryType = GENERATE(IGeometry::Type::Triangle, IGeometry::Type::Quad, IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral);
  const bool empty = GENERATE(false, true);
  CAPTURE(static_cast<int32>(geometryType), empty);
  MeshCase fixture = MakeMeshCase(geometryType);
  DataStructure dataStructure;
  auto* geometryPtr = dynamic_cast<INodeGeometry2D*>(CreateMesh(dataStructure, fixture, empty ? 0 : 2));
  REQUIRE(geometryPtr != nullptr);
  if(empty)
  {
    fixture.edges.clear();
    fixture.faces.clear();
  }
  REQUIRE(geometryPtr->findEdges(false).valid());
  RequireTopology(geometryPtr->getEdges(), fixture.edges);
  REQUIRE(geometryPtr->findUnsharedEdges(false).valid());
  for(const auto& edge : fixture.sharedEdges)
  {
    fixture.edges.erase(edge);
  }
  RequireTopology(geometryPtr->getUnsharedEdges(), fixture.edges);

  if(auto* volumePtr = dynamic_cast<INodeGeometry3D*>(geometryPtr); volumePtr != nullptr)
  {
    REQUIRE(volumePtr->findFaces(false).valid());
    RequireTopology(volumePtr->getFaces(), fixture.faces);
    REQUIRE(volumePtr->findUnsharedFaces(false).valid());
    for(const auto& face : fixture.sharedFaces)
    {
      fixture.faces.erase(face);
    }
    RequireTopology(volumePtr->getUnsharedFaces(), fixture.faces);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("Geometry incidence and neighbors clear cached IDs after bulk-read failures", "[simplnx][GeometryBulkContracts]")
{
  const auto geometryType = GENERATE(IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad, IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral);
  const usize failingRead = GENERATE(1, 2, 3);
  CAPTURE(static_cast<int32>(geometryType), failingRead);
  DataStructure dataStructure;
  auto* geometryPtr = CreateMesh(dataStructure, MakeMeshCase(geometryType), 3);
  REQUIRE(geometryPtr->findElementNeighbors(false).valid());
  REQUIRE(geometryPtr->getElementContainingVertId().has_value());
  REQUIRE(geometryPtr->getElementNeighborsId().has_value());
  UInt64Array* connectivityPtr = nullptr;
  REQUIRE_NOTHROW(connectivityPtr = &dataStructure.getDataRefAs<UInt64Array>(DataPath({"Mesh", "Connectivity"})));
  auto* storePtr = dynamic_cast<FailingResidentStore<uint64>*>(connectivityPtr->getDataStore());
  REQUIRE(storePtr != nullptr);
  storePtr->readCalls = 0;
  storePtr->failingReadCall = failingRead;
  RequireInjectedError(geometryPtr->findElementNeighbors(true), "read");
  CHECK_FALSE(geometryPtr->getElementNeighborsId().has_value());
  if(failingRead < 3)
  {
    CHECK_FALSE(geometryPtr->getElementContainingVertId().has_value());
  }
  else
  {
    CHECK(geometryPtr->getElementContainingVertId().has_value());
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("Geometry extraction clears cached IDs after output-resize failures", "[simplnx][GeometryBulkContracts]")
{
  const auto geometryType = GENERATE(IGeometry::Type::Triangle, IGeometry::Type::Quad, IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral);
  const bool unshared = GENERATE(false, true);
  CAPTURE(static_cast<int32>(geometryType), unshared);
  DataStructure dataStructure;
  auto* geometryPtr = dynamic_cast<INodeGeometry2D*>(CreateMesh(dataStructure, MakeMeshCase(geometryType), 2));
  REQUIRE(geometryPtr != nullptr);

  SECTION("Edges")
  {
    REQUIRE((unshared ? geometryPtr->findUnsharedEdges(false) : geometryPtr->findEdges(false)).valid());
    const auto* outputPtr = unshared ? geometryPtr->getUnsharedEdges() : geometryPtr->getEdges();
    REQUIRE(outputPtr != nullptr);
    auto* mutableOutputPtr = dataStructure.getDataAs<UInt64Array>(outputPtr->getId());
    REQUIRE(mutableOutputPtr != nullptr);
    auto store = std::make_shared<FailingResidentStore<uint64>>(outputPtr->getTupleShape(), outputPtr->getComponentShape(), uint64{77});
    store->rejectResize = true;
    auto setDataStoreResult3 = mutableOutputPtr->setDataStore(store);
    SIMPLNX_RESULT_REQUIRE_VALID(setDataStoreResult3);
    RequireInjectedError(unshared ? geometryPtr->findUnsharedEdges(true) : geometryPtr->findEdges(true), "resize");
    CHECK_FALSE((unshared ? geometryPtr->getUnsharedEdgesId() : geometryPtr->getEdgeListId()).has_value());
    for(usize valueIndex = 0; valueIndex < store->getSize(); ++valueIndex)
    {
      CHECK(store->getValue(valueIndex) == 77);
    }
  }

  SECTION("Faces")
  {
    if(auto* volumePtr = dynamic_cast<INodeGeometry3D*>(geometryPtr); volumePtr != nullptr)
    {
      REQUIRE((unshared ? volumePtr->findUnsharedFaces(false) : volumePtr->findFaces(false)).valid());
      const auto* outputPtr = unshared ? volumePtr->getUnsharedFaces() : volumePtr->getFaces();
      REQUIRE(outputPtr != nullptr);
      auto* mutableOutputPtr = dataStructure.getDataAs<UInt64Array>(outputPtr->getId());
      REQUIRE(mutableOutputPtr != nullptr);
      auto store = std::make_shared<FailingResidentStore<uint64>>(outputPtr->getTupleShape(), outputPtr->getComponentShape(), uint64{77});
      store->rejectResize = true;
      auto setDataStoreResult2 = mutableOutputPtr->setDataStore(store);
      SIMPLNX_RESULT_REQUIRE_VALID(setDataStoreResult2);
      RequireInjectedError(unshared ? volumePtr->findUnsharedFaces(true) : volumePtr->findFaces(true), "resize");
      CHECK_FALSE((unshared ? volumePtr->getUnsharedFacesId() : volumePtr->getFaceListId()).has_value());
      for(usize valueIndex = 0; valueIndex < store->getSize(); ++valueIndex)
      {
        CHECK(store->getValue(valueIndex) == 77);
      }
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("RectGrid element sizes clear cached IDs after invalid bounds", "[simplnx][GeometryBulkContracts]")
{
  const usize axisIndex = GENERATE(0, 1, 2);
  CAPTURE(axisIndex);
  DataStructure dataStructure;
  auto* geometryPtr = CreateRectGrid(dataStructure);
  const std::array<Float32Array*, 3> bounds = {geometryPtr->getXBounds(), geometryPtr->getYBounds(), geometryPtr->getZBounds()};
  auto* boundsPtr = bounds[axisIndex];
  REQUIRE(boundsPtr != nullptr);
  int32 expectedError = -1833;

  SECTION("Too few bound values")
  {
    REQUIRE(boundsPtr->getDataStoreRef().resizeTuples({2}).valid());
    expectedError = -6030;
  }
  SECTION("Zero spacing")
  {
    boundsPtr->setValue(1, 0.0F);
  }
  SECTION("Negative spacing")
  {
    boundsPtr->setValue(1, -1.0F);
  }

  const auto result = geometryPtr->findElementSizes(true);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == expectedError);
  CHECK_FALSE(geometryPtr->getElementSizesId().has_value());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("RectGrid element sizes return bulk failures and clear cached IDs", "[simplnx][GeometryBulkContracts]")
{
  DataStructure dataStructure;
  auto* geometryPtr = CreateRectGrid(dataStructure);
  SECTION("Each bounds read")
  {
    const usize axisIndex = GENERATE(0, 1, 2);
    CAPTURE(axisIndex);
    const std::array<Float32Array*, 3> bounds = {geometryPtr->getXBounds(), geometryPtr->getYBounds(), geometryPtr->getZBounds()};
    auto* storePtr = dynamic_cast<FailingResidentStore<float32>*>(bounds[axisIndex]->getDataStore());
    REQUIRE(storePtr != nullptr);
    storePtr->readCalls = 0;
    storePtr->failingReadCall = 1;
    RequireInjectedError(geometryPtr->findElementSizes(true), "read");
  }
  SECTION("Sizes write")
  {
    const usize failingWrite = GENERATE(1, 2);
    CAPTURE(failingWrite);
    auto* sizesPtr = dataStructure.getDataAs<Float32Array>(*geometryPtr->getElementSizesId());
    REQUIRE(sizesPtr != nullptr);
    auto store = std::make_shared<FailingResidentStore<float32>>(sizesPtr->getTupleShape(), sizesPtr->getComponentShape(), -77.0F);
    store->failingWriteCall = failingWrite;
    auto setDataStoreResult = sizesPtr->setDataStore(store);
    SIMPLNX_RESULT_REQUIRE_VALID(setDataStoreResult);
    RequireInjectedError(geometryPtr->findElementSizes(true), "write");
    // Completed slices may change, but the rejected slice and later values retain their sentinel.
    for(usize valueIndex = (failingWrite - 1) * 4; valueIndex < store->getSize(); ++valueIndex)
    {
      CHECK(store->getValue(valueIndex) == -77.0F);
    }
  }
  CHECK_FALSE(geometryPtr->getElementSizesId().has_value());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
