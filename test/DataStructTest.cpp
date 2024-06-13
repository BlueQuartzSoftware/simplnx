#include "DataStructObserver.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_RectGridGeo = "Rect Grid Geom";
constexpr StringLiteral k_XBounds = "X Bounds";
constexpr StringLiteral k_YBounds = "Y Bounds";
constexpr StringLiteral k_ZBounds = "Z Bounds";
constexpr StringLiteral k_StringArray = "String Array";
constexpr StringLiteral k_EdgeGeo = "Edge Geometry";
constexpr StringLiteral k_ScalarData = "scalar";
constexpr StringLiteral k_SharedEdges = "SharedEdges";
constexpr StringLiteral k_SharedFaces = "SharedFaces";
constexpr StringLiteral k_QuadGeo = "Quad Geometry";
constexpr StringLiteral k_TetGeo = "Tet Geometry";
constexpr StringLiteral k_SharedPolyhedrons = "SharedPolyhedronList";
constexpr StringLiteral k_HexGeo = "Hex Geometry";
} // namespace

TEST_CASE("SimplnxCore::DataObjectType Check")
{
  static_assert(DataObject::Type::IGeometry == static_cast<DataObject::Type>(8));
  static_assert(DataObject::Type::IGridGeometry == static_cast<DataObject::Type>(9));
  static_assert(DataObject::Type::RectGridGeom == static_cast<DataObject::Type>(10));
  static_assert(DataObject::Type::ImageGeom == static_cast<DataObject::Type>(11));
  static_assert(DataObject::Type::INodeGeometry0D == static_cast<DataObject::Type>(12));
  static_assert(DataObject::Type::VertexGeom == static_cast<DataObject::Type>(13));
  static_assert(DataObject::Type::INodeGeometry1D == static_cast<DataObject::Type>(14));
  static_assert(DataObject::Type::EdgeGeom == static_cast<DataObject::Type>(15));
  static_assert(DataObject::Type::INodeGeometry2D == static_cast<DataObject::Type>(16));
  static_assert(DataObject::Type::QuadGeom == static_cast<DataObject::Type>(17));
  static_assert(DataObject::Type::TriangleGeom == static_cast<DataObject::Type>(18));
  static_assert(DataObject::Type::INodeGeometry3D == static_cast<DataObject::Type>(19));
  static_assert(DataObject::Type::HexahedralGeom == static_cast<DataObject::Type>(20));
  static_assert(DataObject::Type::TetrahedralGeom == static_cast<DataObject::Type>(21));
}

// This test will ensure we don't run into runtime exceptions trying to run the functions
TEST_CASE("SimplnxCore::exportHierarchyAsGraphViz")
{
  DataStructure dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  auto outputPath = fs::path(fmt::format("{}/exportHierarchyAsGraphViz_test.dot", unit_test::k_BinaryTestOutputDir));
  std::cout << outputPath << std::endl;
  std::ofstream output(outputPath, std::ios_base::trunc);
  dataStructure.exportHierarchyAsGraphViz(output);
}

// This test will ensure we don't run into runtime exceptions trying to run the functions
TEST_CASE("SimplnxCore::exportHierarchyAsText")
{
  DataStructure dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  auto outputPath = fs::path(fmt::format("{}/exportHierarchyAsText_test.txt", unit_test::k_BinaryTestOutputDir));
  std::cout << outputPath << std::endl;
  std::ofstream output(outputPath, std::ios_base::trunc);
  dataStructure.exportHierarchyAsText(output);
}

DataStructure createTestDataStructure()
{
  DataStructure dataStruct;

  // Image Geometry
  {
    ImageGeom* imageGeom = ImageGeom::Create(dataStruct, Constants::k_ImageGeometry);
    imageGeom->setSpacing({0.25f, 0.55f, 1.86});
    imageGeom->setOrigin({0.0f, 20.0f, 66.0f});
    std::vector<usize> imageGeomDims = {40, 60, 80};
    imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

    const DataGroup* levelOneGroup = DataGroup::Create(dataStruct, Constants::k_LevelOne, imageGeom->getId());

    const DataGroup* levelTwoGroup = DataGroup::Create(dataStruct, Constants::k_LevelTwo, levelOneGroup->getId());
    auto* neighborList = NeighborList<int16>::Create(dataStruct, Constants::k_Int16DataSet, 3, levelOneGroup->getId());
    neighborList->resizeTotalElements(3);
    std::vector<int16> list1 = {117, 875, 1035, 3905, 4214};
    std::vector<int16> list2 = {750, 1905, 1912, 2015, 2586, 3180, 3592, 4041, 4772};
    std::vector<int16> list3 = {309, 775, 2625, 2818, 3061, 3751, 4235, 4817};
    neighborList->setList(0, std::make_shared<std::vector<int16>>(list1));
    neighborList->setList(1, std::make_shared<std::vector<int16>>(list2));
    neighborList->setList(2, std::make_shared<std::vector<int16>>(list3));

    Int32Array* testIntArray = UnitTest::CreateTestDataArray<int32>(dataStruct, Constants::k_Int32DataSet, {5}, {1}, levelTwoGroup->getId());
    (*testIntArray)[0] = 1;
    (*testIntArray)[1] = 5;
    (*testIntArray)[2] = 10;
    (*testIntArray)[3] = 15;
    (*testIntArray)[4] = 20;

    const std::vector<usize> reversedDims(imageGeomDims.rbegin(), imageGeomDims.rend());
    AttributeMatrix* levelOneAttMatrix = AttributeMatrix::Create(dataStruct, Constants::k_CellData, reversedDims, imageGeom->getId());
    imageGeom->setCellData(*levelOneAttMatrix);

    BoolArray* testBoolArray = UnitTest::CreateTestDataArray<bool>(dataStruct, Constants::k_ConditionalArray, reversedDims, {1}, levelOneAttMatrix->getId());
    testBoolArray->fill(true);
    (*testBoolArray)[0] = false;
    (*testBoolArray)[5] = false;
    (*testBoolArray)[10] = false;
    (*testBoolArray)[20] = false;
    (*testBoolArray)[50] = false;
  }

  // Rectilinear Grid Geometry
  {
    RectGridGeom* rectGeom = RectGridGeom::Create(dataStruct, k_RectGridGeo);
    std::vector<usize> dims = {10, 10, 5};
    rectGeom->setDimensions(dims);

    Float32Array* xBoundsArray = UnitTest::CreateTestDataArray<float32>(dataStruct, k_XBounds, {14}, {1});
    (*xBoundsArray)[0] = 0;
    (*xBoundsArray)[1] = 1;
    (*xBoundsArray)[2] = 2;
    (*xBoundsArray)[3] = 3;
    (*xBoundsArray)[4] = 4;
    (*xBoundsArray)[5] = 5;
    (*xBoundsArray)[6] = 6;
    (*xBoundsArray)[7] = 7;
    (*xBoundsArray)[8] = 8;
    (*xBoundsArray)[9] = 9;
    (*xBoundsArray)[10] = 10;
    (*xBoundsArray)[11] = 11;
    (*xBoundsArray)[12] = 12;
    (*xBoundsArray)[13] = 14;
    Float32Array* yBoundsArray = UnitTest::CreateTestDataArray<float32>(dataStruct, k_YBounds, {14}, {1});
    (*yBoundsArray)[0] = 0;
    (*yBoundsArray)[1] = 2;
    (*yBoundsArray)[2] = 4;
    (*yBoundsArray)[3] = 6;
    (*yBoundsArray)[4] = 8;
    (*yBoundsArray)[5] = 10;
    (*yBoundsArray)[6] = 12;
    (*yBoundsArray)[7] = 14;
    (*yBoundsArray)[8] = 16;
    (*yBoundsArray)[9] = 18;
    (*yBoundsArray)[10] = 20;
    (*yBoundsArray)[11] = 22;
    (*yBoundsArray)[12] = 24;
    (*yBoundsArray)[13] = 26;
    Float32Array* zBoundsArray = UnitTest::CreateTestDataArray<float32>(dataStruct, k_ZBounds, {14}, {1});
    (*zBoundsArray)[0] = 0;
    (*zBoundsArray)[1] = 5;
    (*zBoundsArray)[2] = 10;
    (*zBoundsArray)[3] = 15;
    (*zBoundsArray)[4] = 20;
    (*zBoundsArray)[5] = 25;
    (*zBoundsArray)[6] = 30;
    (*zBoundsArray)[7] = 35;
    (*zBoundsArray)[8] = 40;
    (*zBoundsArray)[9] = 45;
    (*zBoundsArray)[10] = 50;
    (*zBoundsArray)[11] = 55;
    (*zBoundsArray)[12] = 60;
    (*zBoundsArray)[13] = 65;

    rectGeom->setBounds(xBoundsArray, yBoundsArray, zBoundsArray);
    rectGeom->findElementSizes(false);
  }

  // Vertex Geometry
  {
    VertexGeom* vertexGeom = VertexGeom::Create(dataStruct, Constants::k_VertexGeometry);

    std::vector<usize> vertTupleShape = {4};
    AttributeMatrix* vertAttMatrix = AttributeMatrix::Create(dataStruct, Constants::k_VertexDataGroupName, vertTupleShape, vertexGeom->getId());
    vertexGeom->setVertexAttributeMatrix(*vertAttMatrix);
    StringArray* testStringArray = StringArray::CreateWithValues(dataStruct, k_StringArray, {"stringone", "stringtwo", "stringthree", "stringfour"}, vertAttMatrix->getId());
    Float32Array* vertListArray = UnitTest::CreateTestDataArray<float32>(dataStruct, Constants::k_Float32DataSet, vertTupleShape, {3}, vertexGeom->getId());
    (*vertListArray)[0] = 0;
    (*vertListArray)[1] = 0;
    (*vertListArray)[2] = 0;
    (*vertListArray)[3] = 1;
    (*vertListArray)[4] = 1;
    (*vertListArray)[5] = 1;
    (*vertListArray)[6] = 2;
    (*vertListArray)[7] = 2;
    (*vertListArray)[8] = 2;
    (*vertListArray)[9] = 3;
    (*vertListArray)[10] = 3;
    (*vertListArray)[11] = 3;
    vertexGeom->setVertices(*vertListArray);

    vertexGeom->findElementSizes(false);
  }

  // Edge Geometry
  {
    EdgeGeom* edgeGeom = EdgeGeom::Create(dataStruct, k_EdgeGeo);

    auto scalar = ScalarData<int64>::Create(dataStruct, k_ScalarData, 60, edgeGeom->getId());
    std::vector<usize> edgeTupleShape = {4};
    AttributeMatrix* edgeAttMatrix = AttributeMatrix::Create(dataStruct, INodeGeometry1D::k_EdgeDataName, edgeTupleShape, edgeGeom->getId());
    edgeGeom->setEdgeAttributeMatrix(*edgeAttMatrix);

    std::vector<usize> edgeVertTupleShape = {5};
    AttributeMatrix* edgeVertAttMatrix = AttributeMatrix::Create(dataStruct, Constants::k_VertexDataGroupName, edgeVertTupleShape, edgeGeom->getId());
    edgeGeom->setVertexAttributeMatrix(*edgeVertAttMatrix);
    Float32Array* edgeVertListArray = UnitTest::CreateTestDataArray<float32>(dataStruct, Constants::k_Float32DataSet, edgeVertTupleShape, {3}, edgeGeom->getId());
    (*edgeVertListArray)[0] = 0;
    (*edgeVertListArray)[1] = 0;
    (*edgeVertListArray)[2] = 0;
    (*edgeVertListArray)[3] = 1;
    (*edgeVertListArray)[4] = 1;
    (*edgeVertListArray)[5] = 1;
    (*edgeVertListArray)[6] = 2;
    (*edgeVertListArray)[7] = 2;
    (*edgeVertListArray)[8] = 2;
    (*edgeVertListArray)[9] = 3;
    (*edgeVertListArray)[10] = 3;
    (*edgeVertListArray)[11] = 3;
    (*edgeVertListArray)[12] = 4;
    (*edgeVertListArray)[13] = 4;
    (*edgeVertListArray)[14] = 4;
    edgeGeom->setVertices(*edgeVertListArray);
    IGeometry::SharedEdgeList* edgesListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedEdges, edgeTupleShape, {2}, edgeGeom->getId());
    (*edgesListArray)[0] = 0;
    (*edgesListArray)[1] = 1;
    (*edgesListArray)[2] = 1;
    (*edgesListArray)[3] = 2;
    (*edgesListArray)[4] = 2;
    (*edgesListArray)[5] = 3;
    (*edgesListArray)[6] = 3;
    (*edgesListArray)[7] = 4;
    edgeGeom->setEdgeList(*edgesListArray);

    edgeGeom->findElementSizes(false);
    edgeGeom->findElementsContainingVert(false);
    edgeGeom->findElementNeighbors(false);
    edgeGeom->findElementCentroids(false);
  }

  // Triangle Geometry
  {
    TriangleGeom* triangleGeom = TriangleGeom::Create(dataStruct, Constants::k_TriangleGeometryName);

    std::vector<usize> faceTupleShape = {5};
    AttributeMatrix* faceAttMatrix = AttributeMatrix::Create(dataStruct, INodeGeometry2D::k_FaceDataName, faceTupleShape, triangleGeom->getId());
    triangleGeom->setFaceAttributeMatrix(*faceAttMatrix);

    std::vector<usize> faceVertTupleShape = {6};
    AttributeMatrix* faceVertAttMatrix = AttributeMatrix::Create(dataStruct, Constants::k_VertexDataGroupName, faceVertTupleShape, triangleGeom->getId());
    triangleGeom->setVertexAttributeMatrix(*faceVertAttMatrix);
    Float32Array* faceVertListArray = UnitTest::CreateTestDataArray<float32>(dataStruct, Constants::k_Float32DataSet, faceVertTupleShape, {3}, triangleGeom->getId());
    (*faceVertListArray)[0] = 0;
    (*faceVertListArray)[1] = 0;
    (*faceVertListArray)[2] = 0;
    (*faceVertListArray)[3] = 0;
    (*faceVertListArray)[4] = -1;
    (*faceVertListArray)[5] = 1;
    (*faceVertListArray)[6] = 1;
    (*faceVertListArray)[7] = -0.5;
    (*faceVertListArray)[8] = 1;
    (*faceVertListArray)[9] = 0.5;
    (*faceVertListArray)[10] = 1;
    (*faceVertListArray)[11] = 0.5;
    (*faceVertListArray)[12] = -0.5;
    (*faceVertListArray)[13] = 1;
    (*faceVertListArray)[14] = 0;
    (*faceVertListArray)[15] = -1;
    (*faceVertListArray)[16] = -0.5;
    (*faceVertListArray)[17] = -1;
    triangleGeom->setVertices(*faceVertListArray);
    IGeometry::SharedEdgeList* facesListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedFaces, faceTupleShape, {3}, triangleGeom->getId());
    (*facesListArray)[0] = 0;
    (*facesListArray)[1] = 1;
    (*facesListArray)[2] = 2;
    (*facesListArray)[3] = 0;
    (*facesListArray)[4] = 2;
    (*facesListArray)[5] = 3;
    (*facesListArray)[6] = 0;
    (*facesListArray)[7] = 3;
    (*facesListArray)[8] = 4;
    (*facesListArray)[9] = 0;
    (*facesListArray)[10] = 4;
    (*facesListArray)[11] = 5;
    (*facesListArray)[12] = 0;
    (*facesListArray)[13] = 5;
    (*facesListArray)[14] = 1;
    triangleGeom->setFaceList(*facesListArray);

    triangleGeom->findEdges(false);
    // TODO: triangleGeom->findElementSizes(false);
    triangleGeom->findElementsContainingVert(false);
    triangleGeom->findElementNeighbors(false);
    triangleGeom->findElementCentroids(false);
    triangleGeom->findUnsharedEdges(false);
  }

  // Quad Geometry
  {
    QuadGeom* quadGeom = QuadGeom::Create(dataStruct, k_QuadGeo);

    std::vector<usize> faceTupleShape = {2};
    AttributeMatrix* faceAttMatrix = AttributeMatrix::Create(dataStruct, INodeGeometry2D::k_FaceDataName, faceTupleShape, quadGeom->getId());
    quadGeom->setFaceAttributeMatrix(*faceAttMatrix);
    std::vector<usize> faceVertTupleShape = {6};
    AttributeMatrix* faceVertAttMatrix = AttributeMatrix::Create(dataStruct, Constants::k_VertexDataGroupName, faceVertTupleShape, quadGeom->getId());
    quadGeom->setVertexAttributeMatrix(*faceVertAttMatrix);
    Float32Array* faceVertListArray = UnitTest::CreateTestDataArray<float32>(dataStruct, Constants::k_Float32DataSet, faceVertTupleShape, {3}, quadGeom->getId());
    (*faceVertListArray)[0] = -1;
    (*faceVertListArray)[1] = 1;
    (*faceVertListArray)[2] = -1;
    (*faceVertListArray)[3] = 0;
    (*faceVertListArray)[4] = 1;
    (*faceVertListArray)[5] = 0;
    (*faceVertListArray)[6] = 1;
    (*faceVertListArray)[7] = 1;
    (*faceVertListArray)[8] = 1;
    (*faceVertListArray)[9] = -1;
    (*faceVertListArray)[10] = -1;
    (*faceVertListArray)[11] = -1;
    (*faceVertListArray)[12] = 0;
    (*faceVertListArray)[13] = -1;
    (*faceVertListArray)[14] = 0;
    (*faceVertListArray)[15] = 1;
    (*faceVertListArray)[16] = -1;
    (*faceVertListArray)[17] = 1;
    quadGeom->setVertices(*faceVertListArray);
    IGeometry::SharedEdgeList* facesListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedFaces, faceTupleShape, {4}, quadGeom->getId());
    (*facesListArray)[0] = 0;
    (*facesListArray)[1] = 3;
    (*facesListArray)[2] = 4;
    (*facesListArray)[3] = 1;
    (*facesListArray)[4] = 1;
    (*facesListArray)[5] = 4;
    (*facesListArray)[6] = 5;
    (*facesListArray)[7] = 2;
    quadGeom->setFaceList(*facesListArray);

    quadGeom->findEdges(false);
    // TODO: quadGeom->findElementSizes();
    quadGeom->findElementsContainingVert(false);
    quadGeom->findElementNeighbors(false);
    quadGeom->findElementCentroids(false);
    quadGeom->findUnsharedEdges(false);
  }

  // Tetrahedral Geometry
  {
    TetrahedralGeom* tetGeom = TetrahedralGeom::Create(dataStruct, k_TetGeo);

    std::vector<usize> cellTupleShape = {2};
    AttributeMatrix* polyAttMatrix = AttributeMatrix::Create(dataStruct, INodeGeometry3D::k_PolyhedronDataName, cellTupleShape, tetGeom->getId());
    tetGeom->setPolyhedraAttributeMatrix(*polyAttMatrix);
    std::vector<usize> vertTupleShape = {5};
    AttributeMatrix* cellVertAttMatrix = AttributeMatrix::Create(dataStruct, Constants::k_VertexDataGroupName, vertTupleShape, tetGeom->getId());
    tetGeom->setVertexAttributeMatrix(*cellVertAttMatrix);
    Float32Array* vertListArray = UnitTest::CreateTestDataArray<float32>(dataStruct, Constants::k_Float32DataSet, vertTupleShape, {3}, tetGeom->getId());
    (*vertListArray)[0] = -1;
    (*vertListArray)[1] = 0.5;
    (*vertListArray)[2] = 0;
    (*vertListArray)[3] = 0;
    (*vertListArray)[4] = 0;
    (*vertListArray)[5] = 0;
    (*vertListArray)[6] = 0;
    (*vertListArray)[7] = 1;
    (*vertListArray)[8] = 0;
    (*vertListArray)[9] = -0.5;
    (*vertListArray)[10] = 0.5;
    (*vertListArray)[11] = 1;
    (*vertListArray)[12] = 1;
    (*vertListArray)[13] = 0.5;
    (*vertListArray)[14] = 0;
    tetGeom->setVertices(*vertListArray);
    IGeometry::SharedEdgeList* polyListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedPolyhedrons, cellTupleShape, {4}, tetGeom->getId());
    (*polyListArray)[0] = 0;
    (*polyListArray)[1] = 1;
    (*polyListArray)[2] = 2;
    (*polyListArray)[3] = 3;
    (*polyListArray)[4] = 1;
    (*polyListArray)[5] = 4;
    (*polyListArray)[6] = 2;
    (*polyListArray)[7] = 3;
    tetGeom->setPolyhedraList(*polyListArray);

    tetGeom->findEdges(false);
    tetGeom->findFaces(false);
    tetGeom->findElementSizes(false);
    tetGeom->findElementsContainingVert(false);
    tetGeom->findElementNeighbors(false);
    tetGeom->findElementCentroids(false);
    tetGeom->findUnsharedEdges(false);
    tetGeom->findUnsharedFaces(false);
  }

  // Hexahedral Geometry
  {
    HexahedralGeom* hexGeom = HexahedralGeom::Create(dataStruct, k_HexGeo);

    std::vector<usize> cellTupleShape = {2};
    AttributeMatrix* polyAttMatrix = AttributeMatrix::Create(dataStruct, INodeGeometry3D::k_PolyhedronDataName, cellTupleShape, hexGeom->getId());
    hexGeom->setPolyhedraAttributeMatrix(*polyAttMatrix);
    std::vector<usize> vertTupleShape = {12};
    AttributeMatrix* cellVertAttMatrix = AttributeMatrix::Create(dataStruct, Constants::k_VertexDataGroupName, vertTupleShape, hexGeom->getId());
    hexGeom->setVertexAttributeMatrix(*cellVertAttMatrix);
    Float32Array* vertListArray = UnitTest::CreateTestDataArray<float32>(dataStruct, Constants::k_Float32DataSet, vertTupleShape, {3}, hexGeom->getId());
    (*vertListArray)[0] = -1;
    (*vertListArray)[1] = 1;
    (*vertListArray)[2] = 1;
    (*vertListArray)[3] = -1;
    (*vertListArray)[4] = -1;
    (*vertListArray)[5] = 1;
    (*vertListArray)[6] = 0;
    (*vertListArray)[7] = -1;
    (*vertListArray)[8] = 1;
    (*vertListArray)[9] = 0;
    (*vertListArray)[10] = 1;
    (*vertListArray)[11] = 1;
    (*vertListArray)[12] = 0;
    (*vertListArray)[13] = 1;
    (*vertListArray)[14] = -1;
    (*vertListArray)[15] = -1;
    (*vertListArray)[16] = 1;
    (*vertListArray)[17] = -1;
    (*vertListArray)[18] = -1;
    (*vertListArray)[19] = -1;
    (*vertListArray)[20] = -1;
    (*vertListArray)[21] = 0;
    (*vertListArray)[22] = -1;
    (*vertListArray)[23] = -1;
    (*vertListArray)[24] = 1;
    (*vertListArray)[25] = -1;
    (*vertListArray)[26] = -1;
    (*vertListArray)[27] = 1;
    (*vertListArray)[28] = 1;
    (*vertListArray)[29] = -1;
    (*vertListArray)[30] = 1;
    (*vertListArray)[31] = 1;
    (*vertListArray)[32] = 1;
    (*vertListArray)[33] = 1;
    (*vertListArray)[34] = -1;
    (*vertListArray)[35] = 1;
    hexGeom->setVertices(*vertListArray);
    IGeometry::SharedEdgeList* polyListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedPolyhedrons, cellTupleShape, {8}, hexGeom->getId());
    (*polyListArray)[0] = 6;
    (*polyListArray)[1] = 7;
    (*polyListArray)[2] = 4;
    (*polyListArray)[3] = 5;
    (*polyListArray)[4] = 1;
    (*polyListArray)[5] = 2;
    (*polyListArray)[6] = 3;
    (*polyListArray)[7] = 0;
    (*polyListArray)[8] = 7;
    (*polyListArray)[9] = 8;
    (*polyListArray)[10] = 9;
    (*polyListArray)[11] = 4;
    (*polyListArray)[12] = 2;
    (*polyListArray)[13] = 11;
    (*polyListArray)[14] = 10;
    (*polyListArray)[15] = 3;
    hexGeom->setPolyhedraList(*polyListArray);

    hexGeom->findEdges(false);
    hexGeom->findFaces(false);
    hexGeom->findElementSizes(false);
    hexGeom->findElementsContainingVert(false);
    hexGeom->findElementNeighbors(false);
    hexGeom->findElementCentroids(false);
    hexGeom->findUnsharedEdges(false);
    hexGeom->findUnsharedFaces(false);
  }

  return dataStruct;
}

/**
 * @brief Test creation and removal of items in a tree-style structure. No node has more than one parent.
 */
TEST_CASE("DataGroupTree")
{
  // Create structuure
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child = DataGroup::Create(dataStr, "bar", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "bazz", child->getId());

  auto groupId = group->getId();
  auto childId = child->getId();
  auto grandchildId = grandchild->getId();

  SECTION("find data")
  {
    REQUIRE(dataStr.getData(groupId) != nullptr);
    REQUIRE(dataStr.getData(childId) != nullptr);
    REQUIRE(dataStr.getData(grandchildId) != nullptr);
  }
  SECTION("remove mid-level group")
  {
    group->remove(child->getName());
    REQUIRE(dataStr.getData(childId) == nullptr);
    REQUIRE(dataStr.getData(grandchildId) == nullptr);
  }
  SECTION("remove top-level group")
  {
    dataStr.removeData(group->getId());
    REQUIRE(dataStr.getData(groupId) == nullptr);
  }
}

/**
 * @brief Test creation and removal of DataObjects in a graph-style DataStructure where multiple parents are allowed.
 */
TEST_CASE("DataGroupGraph")
{
  // Create DataStructure
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  // Get IDs
  auto groupId = group->getId();
  auto child1Id = child1->getId();
  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();

  // Multi-parent connections
  dataStr.setAdditionalParent(grandchildId, child2Id);

  // Test findAllObjectsOfType
  {
    auto findAll = group->findAllChildrenOfType<BaseGroup>();
    REQUIRE(findAll.size() == 2);

    auto findAllRecur = group->findAllChildrenOfType<BaseGroup>(true);
    REQUIRE(findAllRecur.size() == 3);
  }

  SECTION("find data")
  {
    REQUIRE(dataStr.getData(groupId) != nullptr);
    REQUIRE(dataStr.getData(child1Id) != nullptr);
    REQUIRE(dataStr.getData(child2Id) != nullptr);
    REQUIRE(dataStr.getData(grandchildId) != nullptr);
  }
  SECTION("remove mid-level group")
  {
    group->remove(child1->getName());
    REQUIRE(dataStr.getData(child1Id) == nullptr);
    REQUIRE(dataStr.getData(grandchildId) != nullptr);
  }
  SECTION("remove top-level group")
  {
    dataStr.removeData(group->getId());
    REQUIRE(dataStr.getData(groupId) == nullptr);
    REQUIRE(dataStr.getData(grandchildId) == nullptr);
  }
}

/**
 * @brief Test DataPath usage
 */
TEST_CASE("DataPathTest")
{
  // Create DataStructure
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();
  SECTION("add additional parent")
  {
    REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));
  }

  auto shouldNotExist = dataStr.getId(DataPath({"Does", "Not", "Exist"}));
  REQUIRE(false == shouldNotExist.has_value());

  const DataPath gcPath1({"Foo", "Bar1", "Bazz"});
  const DataPath gcPath2({"Foo", "Bar2", "Bazz"});

  REQUIRE(dataStr.getData(gcPath1) != nullptr);
  REQUIRE(dataStr.getData(gcPath2) != nullptr);

  const DataPath c1Path({"Foo", "Bar1"});
  REQUIRE(dataStr.removeData(c1Path));
  REQUIRE(dataStr.getData(gcPath1) == nullptr);
  REQUIRE(dataStr.getData(DataPath({"Foo", "Bar2"})) != nullptr);
  auto gc2 = dataStr.getData(gcPath2);
  REQUIRE(dataStr.getData(gcPath2) != nullptr);

  dataStr.removeData(child2Id);
  REQUIRE(dataStr.getData(gcPath2) == nullptr);

  DataPath fromStringTest = DataPath::FromString("/Group/Data").value();
  REQUIRE(fromStringTest.getLength() == 2);
  fromStringTest = DataPath::FromString("/Group/").value();
  REQUIRE(fromStringTest.getLength() == 1);

  auto empty1 = DataPath::FromString("/");
  REQUIRE(empty1.has_value());
  REQUIRE(empty1.value().empty());
  auto empty2 = DataPath::FromString("");
  REQUIRE(empty2.has_value());
  REQUIRE(empty2.value().empty());
}

TEST_CASE("LinkedPathTest")
{
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();

  REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));

  // DataPath
  const DataPath grandPath({"Foo", "Bar1", "Bazz"});
  const LinkedPath linkedPath = dataStr.getLinkedPath(grandPath);

  REQUIRE(linkedPath.isValid());
  REQUIRE(grandchild == linkedPath.getData());

  REQUIRE(child1->rename("Bar1.3"));
  REQUIRE(dataStr.getData(grandPath) == nullptr);
  REQUIRE(linkedPath.getData() != nullptr);
  REQUIRE(linkedPath.isValid());

  REQUIRE(dataStr.removeData(linkedPath.getIdAt(1)));
  REQUIRE(!linkedPath.isValid());
}

/**
 * @brief Tests IDataStructureListener usage
 */
TEST_CASE("DataStructureListenerTest")
{
  DataStructure dataStr;
  DataStructObserver dsListener(dataStr);

  // Adds items to the DataStructure. Each addition should trigger dsListener.
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  auto groupId = group->getId();
  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();

  REQUIRE(dsListener.getDataAddedCount() == 4);

  REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));
  REQUIRE(dsListener.getDataReparentedCount() == 1);

  dataStr.removeData(child2Id);
  REQUIRE(dsListener.getDataRemovedCount() == 1);
  dataStr.removeData(groupId);
  REQUIRE(dsListener.getDataRemovedCount() == 4);
}

TEST_CASE("DataStructureCopyTest")
{
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  auto groupId = group->getId();
  auto child1Id = child1->getId();
  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();

  REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));

  // Copy DataStructure
  DataStructure dataStrCopy(dataStr);

  REQUIRE(dataStrCopy.getData(groupId));
  REQUIRE(dataStrCopy.getData(child1Id));
  REQUIRE(dataStrCopy.getData(child2Id));
  REQUIRE(dataStrCopy.getData(grandchildId));

  DataObject* data = dataStr.getData(groupId);
  DataObject* dataCopy = dataStrCopy.getData(groupId);
  REQUIRE(dataStrCopy.getData(groupId) != dataStr.getData(groupId));
  REQUIRE(dataStrCopy.getData(child1Id) != dataStr.getData(child1Id));
  REQUIRE(dataStrCopy.getData(child2Id) != dataStr.getData(child2Id));
  REQUIRE(dataStrCopy.getData(grandchildId) != dataStr.getData(grandchildId));

  // Create new group
  auto newGroup = DataGroup::Create(dataStr, "New Group", child2Id);
  auto newId = newGroup->getId();
  REQUIRE(dataStr.getData(newId));
  REQUIRE(!dataStrCopy.getData(newId));

  auto newGroup2 = DataGroup::Create(dataStrCopy, "New Group (2)", child2Id);
  auto newId2 = newGroup2->getId();
  REQUIRE(dataStr.getData(newId2) != dataStrCopy.getData(newId2));
  REQUIRE(dataStrCopy.getData(newId2));
}

TEST_CASE("DataStoreTest")
{
  const size_t numComponents = 3;
  const size_t numTuples = 10;
  DataStore<int32_t> store({numTuples}, {numComponents}, 0);

  REQUIRE(store.getNumberOfTuples() == numTuples);
  REQUIRE(store.getNumberOfComponents() == numComponents);
  REQUIRE(store.getSize() == (numComponents * numTuples));

  // subscript operator
  {
    for(usize i = 0; i < store.getSize(); i++)
    {
      store[i] = i + 1;
    }
    int32 x = 1;
    for(usize i = 0; i < store.getSize(); i++)
    {
      REQUIRE(store[i] == x++);
    }
  }
  // get / set values
  {
    const usize index = 5;
    const usize value = 25;
    store.setValue(index, value);
    REQUIRE(store.getValue(index) == value);
  }
  // iterators
  {
    const int32 initVal = -30;
    int32 x = initVal;
    for(auto& value : store)
    {
      value = x++;
    }
    x = initVal;
    for(const auto& value : store)
    {
      REQUIRE(value == x++);
    }
  }
}

TEST_CASE("DataArrayTest")
{
  DataStructure dataStr;

  auto store = std::make_shared<DataStore<int32>>(std::vector<usize>{2}, std::vector<usize>{2}, 0);
  REQUIRE(store != nullptr);
  auto dataArr = DataArray<int32>::Create(dataStr, "array", store);
  REQUIRE(dataArr != nullptr);

  SECTION("test size")
  {
    REQUIRE(dataArr->getSize() == store->getSize());
    REQUIRE(dataArr->getNumberOfComponents() == store->getNumberOfComponents());
    REQUIRE(dataArr->getNumberOfTuples() == store->getNumberOfTuples());
  }

  SECTION("test reading / writing to memory")
  {
    SECTION("test subscript operators")
    {
      for(usize i = 0; i < dataArr->getSize(); i++)
      {
        (*dataArr)[i] = i + 1;
      }
      int32 x = 1;
      for(usize i = 0; i < dataArr->getSize(); i++)
      {
        REQUIRE((*dataArr)[i] == x++);
      }
    }
    SECTION("test iterators")
    {
      const int32 initVal = -30;
      int32 x = initVal;
      for(auto& value : *dataArr)
      {
        value = x++;
      }
      x = initVal;
      for(const auto& value : *dataArr)
      {
        REQUIRE(value == x++);
      }
    }
  }
}

TEST_CASE("ScalarDataTest")
{
  DataStructure dataStr;
  const int32 value = 6;
  auto scalar = ScalarData<int32>::Create(dataStr, "scalar", value);

  // get value
  REQUIRE(value == scalar->getValue());
  REQUIRE((*scalar) == value);
  REQUIRE((*scalar) != (value + 1));

  // edit values
  const int32 newValue = 11;
  scalar->setValue(newValue);
  REQUIRE(newValue == scalar->getValue());

  const int32 newValue2 = 14;
  (*scalar) = newValue2;
  REQUIRE(scalar->getValue() == newValue2);
}

TEST_CASE("DataStructureDuplicateNames")
{
  static constexpr StringLiteral name = "foo";

  DataStructure dataStructure;

  // Top level test

  DataGroup* group1 = DataGroup::Create(dataStructure, name);
  REQUIRE(group1 != nullptr);

  DataGroup* group2 = DataGroup::Create(dataStructure, name);
  REQUIRE(group2 == nullptr);

  // Nested test

  DataGroup* childGroup1 = DataGroup::Create(dataStructure, name, group1->getId());
  REQUIRE(group1 != nullptr);

  DataGroup* childGroup2 = DataGroup::Create(dataStructure, name, group1->getId());
  REQUIRE(group2 == nullptr);
}

TEST_CASE("DataStructureAddingObjectToNonBaseGroup")
{
  DataStructure dataStructure;
  auto store = std::make_shared<DataStore<int32>>(std::vector<usize>{10}, std::vector<usize>{1}, 0);
  auto* dataArray = DataArray<int32>::Create(dataStructure, "foo", std::move(store));
  REQUIRE(dataArray != nullptr);

  DataGroup* group = DataGroup::Create(dataStructure, "bar", dataArray->getId());
  REQUIRE(group == nullptr);
}

TEST_CASE("DataObjectsDeepCopyTest")
{
  DataStructure dataStruct = createTestDataStructure();

  SECTION("Image Geometry")
  {
    const DataPath srcImageGeoPath({Constants::k_ImageGeometry});
    const DataPath destImageGeoPath({Constants::k_ImageGeometry.str() + "_COPY"});
    auto* data = dataStruct.getData(srcImageGeoPath);
    std::shared_ptr<DataObject> imageGeoCopy = data->deepCopy(destImageGeoPath);

    auto allSrcImageGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcImageGeoPath).value();
    auto destImageGeoResults = GetAllChildDataPathsRecursive(dataStruct, destImageGeoPath);
    REQUIRE(destImageGeoResults.has_value());
    auto allDestImageGeoDataPaths = destImageGeoResults.value();
    REQUIRE(allSrcImageGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const auto srcImageGeo = dataStruct.getDataAs<ImageGeom>(srcImageGeoPath);
    const auto destImageGeo = dataStruct.getDataAs<ImageGeom>(destImageGeoPath);
    REQUIRE(srcImageGeo != destImageGeo);
    REQUIRE(srcImageGeo->getOrigin() == destImageGeo->getOrigin());
    REQUIRE(srcImageGeo->getSpacing() == destImageGeo->getSpacing());
    REQUIRE(srcImageGeo->getDimensions() == destImageGeo->getDimensions());

    const DataPath srcLevelOnePath = srcImageGeoPath.createChildPath(Constants::k_LevelOne);
    const DataPath destLevelOnePath = destImageGeoPath.createChildPath(Constants::k_LevelOne);
    REQUIRE(dataStruct.getDataAs<DataGroup>(destLevelOnePath) != nullptr);
    REQUIRE(dataStruct.getDataAs<DataGroup>(destLevelOnePath) != dataStruct.getDataAs<DataGroup>(srcLevelOnePath));

    const DataPath srcLevelTwoPath = srcLevelOnePath.createChildPath(Constants::k_LevelTwo);
    const DataPath destLevelTwoPath = destLevelOnePath.createChildPath(Constants::k_LevelTwo);
    REQUIRE(dataStruct.getDataAs<DataGroup>(destLevelTwoPath) != nullptr);
    REQUIRE(dataStruct.getDataAs<DataGroup>(destLevelTwoPath) != dataStruct.getDataAs<DataGroup>(srcLevelTwoPath));

    const DataPath srcNeighborListPath = srcLevelOnePath.createChildPath(Constants::k_Int16DataSet);
    const DataPath destNeighborListPath = destLevelOnePath.createChildPath(Constants::k_Int16DataSet);
    REQUIRE(dataStruct.getDataAs<NeighborList<int16>>(srcNeighborListPath) != dataStruct.getDataAs<NeighborList<int16>>(destNeighborListPath));
    UnitTest::CompareNeighborLists<int16>(dataStruct, srcNeighborListPath, destNeighborListPath);

    const DataPath srcDataArrayPath = srcLevelTwoPath.createChildPath(Constants::k_Int32DataSet);
    const DataPath destDataArrayPath = destLevelTwoPath.createChildPath(Constants::k_Int32DataSet);
    REQUIRE(dataStruct.getDataAs<Int32Array>(srcDataArrayPath) != dataStruct.getDataAs<Int32Array>(destDataArrayPath));
    UnitTest::CompareArrays<int32>(dataStruct, srcDataArrayPath, destDataArrayPath);

    const DataPath srcAttMatrixPath = srcImageGeoPath.createChildPath(Constants::k_CellData);
    const DataPath destAttMatrixPath = destImageGeoPath.createChildPath(Constants::k_CellData);
    const auto srcAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcAttMatrixPath);
    const auto destAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destAttMatrixPath);
    REQUIRE(srcAttMatrix != destAttMatrix);
    REQUIRE(srcAttMatrix->getShape() == destAttMatrix->getShape());

    const DataPath srcBoolArrayPath = srcAttMatrixPath.createChildPath(Constants::k_ConditionalArray);
    const DataPath destBoolArrayPath = destAttMatrixPath.createChildPath(Constants::k_ConditionalArray);
    REQUIRE(dataStruct.getDataAs<BoolArray>(srcBoolArrayPath) != dataStruct.getDataAs<BoolArray>(destBoolArrayPath));
    UnitTest::CompareArrays<bool>(dataStruct, srcBoolArrayPath, destBoolArrayPath);
  }

  SECTION("Rectilinear Grid Geometry")
  {
    const DataPath srcRectGeoPath({k_RectGridGeo});
    const DataPath destRectGeoPath({k_RectGridGeo.str() + "_COPY"});
    auto* data = dataStruct.getData(srcRectGeoPath);
    std::shared_ptr<DataObject> rectGeoCopy = data->deepCopy(destRectGeoPath);

    auto allSrcRectGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcRectGeoPath).value();
    auto destRectGeoResults = GetAllChildDataPathsRecursive(dataStruct, destRectGeoPath);
    REQUIRE(destRectGeoResults.has_value());
    auto allDestRectGeoDataPaths = destRectGeoResults.value();
    REQUIRE(allSrcRectGeoDataPaths.size() == allDestRectGeoDataPaths.size() - 3);

    const auto srcRectGeo = dataStruct.getDataAs<RectGridGeom>(srcRectGeoPath);
    const auto destRectGeo = dataStruct.getDataAs<RectGridGeom>(destRectGeoPath);
    REQUIRE(srcRectGeo != destRectGeo);
    REQUIRE(srcRectGeo->getDimensions() == destRectGeo->getDimensions());

    const DataPath srcXBoundsArrayPath({k_XBounds});
    const DataPath destXBoundsArrayPath = destRectGeoPath.createChildPath(k_XBounds);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcXBoundsArrayPath) != dataStruct.getDataAs<Float32Array>(destXBoundsArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcXBoundsArrayPath, destXBoundsArrayPath);

    const DataPath srcYBoundsArrayPath({k_YBounds});
    const DataPath destYBoundsArrayPath = destRectGeoPath.createChildPath(k_YBounds);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcYBoundsArrayPath) != dataStruct.getDataAs<Float32Array>(destYBoundsArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcYBoundsArrayPath, destYBoundsArrayPath);

    const DataPath srcZBoundsArrayPath({k_ZBounds});
    const DataPath destZBoundsArrayPath = destRectGeoPath.createChildPath(k_ZBounds);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcZBoundsArrayPath) != dataStruct.getDataAs<Float32Array>(destZBoundsArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcZBoundsArrayPath, destZBoundsArrayPath);

    const DataPath srcVoxelSizesArrayPath = srcRectGeoPath.createChildPath(RectGridGeom::k_VoxelSizes);
    const DataPath destVoxelSizesArrayPath = destRectGeoPath.createChildPath(RectGridGeom::k_VoxelSizes);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcVoxelSizesArrayPath) != dataStruct.getDataAs<Float32Array>(destVoxelSizesArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcVoxelSizesArrayPath, destVoxelSizesArrayPath);
  }

  SECTION("Vertex Geometry")
  {
    const DataPath srcGeoPath({Constants::k_VertexGeometry});
    const DataPath destGeoPath({Constants::k_VertexGeometry.str() + "_COPY"});
    auto* data = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = data->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    auto allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcAttMatrixPath = srcGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const DataPath destAttMatrixPath = destGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const auto srcAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcAttMatrixPath);
    const auto destAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destAttMatrixPath);
    REQUIRE(srcAttMatrix != destAttMatrix);
    REQUIRE(srcAttMatrix->getShape() == destAttMatrix->getShape());

    const DataPath srcStringArrayPath = srcAttMatrixPath.createChildPath(k_StringArray);
    const DataPath destStringArrayPath = destAttMatrixPath.createChildPath(k_StringArray);
    REQUIRE(dataStruct.getDataAs<StringArray>(srcStringArrayPath) != dataStruct.getDataAs<StringArray>(destStringArrayPath));
    UnitTest::CompareStringArrays(dataStruct, srcStringArrayPath, destStringArrayPath);

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(Constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(Constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcVertArrayPath) != dataStruct.getDataAs<Float32Array>(destVertArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcVoxelSizesArrayPath = srcGeoPath.createChildPath(VertexGeom::k_VoxelSizes);
    const DataPath destVoxelSizesArrayPath = destGeoPath.createChildPath(VertexGeom::k_VoxelSizes);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcVoxelSizesArrayPath) != dataStruct.getDataAs<Float32Array>(destVoxelSizesArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcVoxelSizesArrayPath, destVoxelSizesArrayPath);
  }

  SECTION("Edge Geometry")
  {
    const DataPath srcGeoPath({k_EdgeGeo});
    const DataPath destGeoPath({k_EdgeGeo.str() + "_COPY"});
    auto* data = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = data->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    auto allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const auto srcVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    const auto destVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrix != destVertAttMatrix);
    REQUIRE(srcVertAttMatrix->getShape() == destVertAttMatrix->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(INodeGeometry1D::k_EdgeDataName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(INodeGeometry1D::k_EdgeDataName);
    const auto srcCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    const auto destCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrix != destCellAttMatrix);
    REQUIRE(srcCellAttMatrix->getShape() == destCellAttMatrix->getShape());

    const DataPath srcScalarPath = srcGeoPath.createChildPath(k_ScalarData);
    const DataPath destScalarPath = destGeoPath.createChildPath(k_ScalarData);
    const auto srcScalarData = dataStruct.getDataAs<ScalarData<int64>>(srcScalarPath);
    const auto destScalarData = dataStruct.getDataAs<ScalarData<int64>>(destScalarPath);
    REQUIRE(srcScalarData != destScalarData);
    REQUIRE(srcScalarData->getValue() == destScalarData->getValue());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(Constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(Constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcVertArrayPath) != dataStruct.getDataAs<Float32Array>(destVertArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedEdges);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedEdges);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcVoxelSizesArrayPath = srcGeoPath.createChildPath(EdgeGeom::k_VoxelSizes);
    const DataPath destVoxelSizesArrayPath = destGeoPath.createChildPath(EdgeGeom::k_VoxelSizes);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcVoxelSizesArrayPath) != dataStruct.getDataAs<Float32Array>(destVoxelSizesArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcVoxelSizesArrayPath, destVoxelSizesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(EdgeGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(EdgeGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(EdgeGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(EdgeGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(EdgeGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(EdgeGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32Array>(destEltCentroidsPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);
  }

  SECTION("Triangle Geometry")
  {
    const DataPath srcGeoPath({Constants::k_TriangleGeometryName});
    const DataPath destGeoPath({Constants::k_TriangleGeometryName.str() + "_COPY"});
    auto* data = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = data->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    auto allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const auto srcVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    const auto destVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrix != destVertAttMatrix);
    REQUIRE(srcVertAttMatrix->getShape() == destVertAttMatrix->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(INodeGeometry2D::k_FaceDataName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(INodeGeometry2D::k_FaceDataName);
    const auto srcCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    const auto destCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrix != destCellAttMatrix);
    REQUIRE(srcCellAttMatrix->getShape() == destCellAttMatrix->getShape());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(Constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(Constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcVertArrayPath) != dataStruct.getDataAs<Float32Array>(destVertArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedFaces);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedFaces);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(TriangleGeom::k_Edges);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(TriangleGeom::k_Edges);
    REQUIRE(dataStruct.getDataAs<UInt64Array>(srcEdgesArrayPath) != dataStruct.getDataAs<UInt64Array>(destEdgesArrayPath));
    UnitTest::CompareArrays<uint64>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(TriangleGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(TriangleGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(TriangleGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(TriangleGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(TriangleGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(TriangleGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32Array>(destEltCentroidsPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(TriangleGeom::k_UnsharedEdges);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(TriangleGeom::k_UnsharedEdges);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);
  }

  SECTION("Quad Geometry")
  {
    const DataPath srcGeoPath({k_QuadGeo});
    const DataPath destGeoPath({k_QuadGeo.str() + "_COPY"});
    auto* data = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = data->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    auto allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const auto srcVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    const auto destVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrix != destVertAttMatrix);
    REQUIRE(srcVertAttMatrix->getShape() == destVertAttMatrix->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(INodeGeometry2D::k_FaceDataName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(INodeGeometry2D::k_FaceDataName);
    const auto srcCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    const auto destCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrix != destCellAttMatrix);
    REQUIRE(srcCellAttMatrix->getShape() == destCellAttMatrix->getShape());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(Constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(Constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcVertArrayPath) != dataStruct.getDataAs<Float32Array>(destVertArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedFaces);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedFaces);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(QuadGeom::k_Edges);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(QuadGeom::k_Edges);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(QuadGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(QuadGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(QuadGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(QuadGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(QuadGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(QuadGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32Array>(destEltCentroidsPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(QuadGeom::k_UnsharedEdges);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(QuadGeom::k_UnsharedEdges);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);
  }

  SECTION("Tetrahedral Geometry")
  {
    const DataPath srcGeoPath({k_TetGeo});
    const DataPath destGeoPath({k_TetGeo.str() + "_COPY"});
    auto* data = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = data->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    auto allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const auto srcVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    const auto destVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrix != destVertAttMatrix);
    REQUIRE(srcVertAttMatrix->getShape() == destVertAttMatrix->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName);
    const auto srcCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    const auto destCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrix != destCellAttMatrix);
    REQUIRE(srcCellAttMatrix->getShape() == destCellAttMatrix->getShape());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(Constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(Constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcVertArrayPath) != dataStruct.getDataAs<Float32Array>(destVertArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedPolyhedrons);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedPolyhedrons);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_Edges);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_Edges);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcFaceArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_TriangleFaceList);
    const DataPath destFacesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_TriangleFaceList);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcFaceArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destFacesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcFaceArrayPath, destFacesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(TetrahedralGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(TetrahedralGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(TetrahedralGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(TetrahedralGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(TetrahedralGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(TetrahedralGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32Array>(destEltCentroidsPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_UnsharedEdges);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_UnsharedEdges);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);

    const DataPath srcUnsharedFacesArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_UnsharedFaces);
    const DataPath destUnsharedFacesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_UnsharedFaces);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedFacesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedFacesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedFacesArrayPath, destUnsharedFacesArrayPath);
  }

  SECTION("Hexahedral Geometry")
  {
    const DataPath srcGeoPath({k_HexGeo});
    const DataPath destGeoPath({k_HexGeo.str() + "_COPY"});
    auto* data = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = data->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    auto allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(Constants::k_VertexDataGroupName);
    const auto srcVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    const auto destVertAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrix != destVertAttMatrix);
    REQUIRE(srcVertAttMatrix->getShape() == destVertAttMatrix->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName);
    const auto srcCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    const auto destCellAttMatrix = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrix != destCellAttMatrix);
    REQUIRE(srcCellAttMatrix->getShape() == destCellAttMatrix->getShape());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(Constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(Constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcVertArrayPath) != dataStruct.getDataAs<Float32Array>(destVertArrayPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedPolyhedrons);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedPolyhedrons);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_Edges);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_Edges);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcFaceArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_QuadFaceList);
    const DataPath destFacesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_QuadFaceList);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcFaceArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destFacesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcFaceArrayPath, destFacesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(HexahedralGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(HexahedralGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(HexahedralGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(HexahedralGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    UnitTest::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(HexahedralGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(HexahedralGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32Array>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32Array>(destEltCentroidsPath));
    UnitTest::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_UnsharedEdges);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_UnsharedEdges);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);

    const DataPath srcUnsharedFacesArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_UnsharedFaces);
    const DataPath destUnsharedFacesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_UnsharedFaces);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedFacesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedFacesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedFacesArrayPath, destUnsharedFacesArrayPath);
  }

  SECTION("Invalid copies")
  {
    DataStructure badCopyDataStruct = createTestDataStructure();
    const DataPath srcGeoPath({Constants::k_ImageGeometry});
    const DataPath srcGroupPath = srcGeoPath.createChildPath(Constants::k_LevelOne);
    const DataPath srcArrayPath = srcGroupPath.createChildPath(Constants::k_Int16DataSet);
    auto* imageGeo = dataStruct.getData(srcGeoPath);
    auto* dataGroup = dataStruct.getData(srcGroupPath);
    auto* nlArray = dataStruct.getData(srcArrayPath);

    std::shared_ptr<DataObject> copyToChildGeo = imageGeo->deepCopy(srcGroupPath);
    REQUIRE(copyToChildGeo == nullptr);
    REQUIRE(badCopyDataStruct.getDataAs<VertexGeom>(srcGroupPath) == nullptr);

    std::shared_ptr<DataObject> copyGroupToSelfGeo = dataGroup->deepCopy(srcGroupPath);
    REQUIRE(copyGroupToSelfGeo == nullptr);

    std::shared_ptr<DataObject> copyArrayToSelfGeo = nlArray->deepCopy(srcArrayPath);
    REQUIRE(copyArrayToSelfGeo == nullptr);
  }
}

TEST_CASE("NodeBasedGeometryFindElementsTest")
{
  DataStructure dataStruct = createTestDataStructure();

  SECTION("Image Geometry")
  {
    const DataPath srcImageGeoPath({Constants::k_ImageGeometry});
    auto* geom = dataStruct.getDataAs<ImageGeom>(srcImageGeoPath);

    IGeometry::StatusCode status = geom->findElementSizes(false);
    REQUIRE(status == 1);
    status = geom->findElementSizes(false);
    REQUIRE(status == 0);
    status = geom->findElementSizes(true);
    REQUIRE(status == 1);
  }

  SECTION("Rectilinear Grid Geometry")
  {
    const DataPath srcRectGeoPath({k_RectGridGeo});
    auto* geom = dataStruct.getDataAs<RectGridGeom>(srcRectGeoPath);

    IGeometry::StatusCode status = geom->findElementSizes(false);
    REQUIRE(status == 0);
    status = geom->findElementSizes(true);
    REQUIRE(status == 1);
  }

  SECTION("Vertex Geometry")
  {
    const DataPath srcGeoPath({Constants::k_VertexGeometry});
    auto* geom = dataStruct.getDataAs<VertexGeom>(srcGeoPath);

    IGeometry::StatusCode status = geom->findElementSizes(false);
    REQUIRE(status == 0);
    status = geom->findElementSizes(true);
    REQUIRE(status == 1);
  }

  SECTION("Edge Geometry")
  {
    const DataPath srcGeoPath({k_EdgeGeo});
    auto* geom = dataStruct.getDataAs<EdgeGeom>(srcGeoPath);

    IGeometry::StatusCode status = geom->findElementSizes(false);
    REQUIRE(status == 0);
    status = geom->findElementSizes(true);
    REQUIRE(status == 1);

    status = geom->findElementsContainingVert(false);
    REQUIRE(status == 0);
    status = geom->findElementsContainingVert(true);
    REQUIRE(status == 1);

    status = geom->findElementNeighbors(false);
    REQUIRE(status == 0);
    status = geom->findElementNeighbors(true);
    REQUIRE(status == 1);

    status = geom->findElementCentroids(false);
    REQUIRE(status == 0);
    status = geom->findElementCentroids(true);
    REQUIRE(status == 1);
  }

  SECTION("Triangle Geometry")
  {
    const DataPath srcGeoPath({Constants::k_TriangleGeometryName});
    auto* geom = dataStruct.getDataAs<TriangleGeom>(srcGeoPath);

    IGeometry::StatusCode status = geom->findElementSizes(false);
    REQUIRE(status == 1);
    status = geom->findElementSizes(false);
    REQUIRE(status == 0);
    status = geom->findElementSizes(true);
    REQUIRE(status == 1);

    status = geom->findElementsContainingVert(false);
    REQUIRE(status == 0);
    status = geom->findElementsContainingVert(true);
    REQUIRE(status == 1);

    status = geom->findElementNeighbors(false);
    REQUIRE(status == 0);
    status = geom->findElementNeighbors(true);
    REQUIRE(status == 1);

    status = geom->findElementCentroids(false);
    REQUIRE(status == 0);
    status = geom->findElementCentroids(true);
    REQUIRE(status == 1);

    status = geom->findEdges(false);
    REQUIRE(status == 0);
    status = geom->findEdges(true);
    REQUIRE(status == 1);

    status = geom->findUnsharedEdges(false);
    REQUIRE(status == 0);
    status = geom->findUnsharedEdges(true);
    REQUIRE(status == 1);
  }

  SECTION("Quad Geometry")
  {
    const DataPath srcGeoPath({k_QuadGeo});
    auto* geom = dataStruct.getDataAs<QuadGeom>(srcGeoPath);

    IGeometry::StatusCode status = geom->findElementSizes(false);
    REQUIRE(status == 1);
    status = geom->findElementSizes(false);
    REQUIRE(status == 0);
    status = geom->findElementSizes(true);
    REQUIRE(status == 1);

    status = geom->findElementsContainingVert(false);
    REQUIRE(status == 0);
    status = geom->findElementsContainingVert(true);
    REQUIRE(status == 1);

    status = geom->findElementNeighbors(false);
    REQUIRE(status == 0);
    status = geom->findElementNeighbors(true);
    REQUIRE(status == 1);

    status = geom->findElementCentroids(false);
    REQUIRE(status == 0);
    status = geom->findElementCentroids(true);
    REQUIRE(status == 1);

    status = geom->findEdges(false);
    REQUIRE(status == 0);
    status = geom->findEdges(true);
    REQUIRE(status == 1);

    status = geom->findUnsharedEdges(false);
    REQUIRE(status == 0);
    status = geom->findUnsharedEdges(true);
    REQUIRE(status == 1);
  }

  SECTION("Tetrahedral Geometry")
  {
    const DataPath srcGeoPath({k_TetGeo});
    auto* geom = dataStruct.getDataAs<TetrahedralGeom>(srcGeoPath);

    IGeometry::StatusCode status = geom->findElementSizes(false);
    REQUIRE(status == 0);
    status = geom->findElementSizes(true);
    REQUIRE(status == 1);

    status = geom->findElementsContainingVert(false);
    REQUIRE(status == 0);
    status = geom->findElementsContainingVert(true);
    REQUIRE(status == 1);

    status = geom->findElementNeighbors(false);
    REQUIRE(status == 0);
    status = geom->findElementNeighbors(true);
    REQUIRE(status == 1);

    status = geom->findElementCentroids(false);
    REQUIRE(status == 0);
    status = geom->findElementCentroids(true);
    REQUIRE(status == 1);

    status = geom->findEdges(false);
    REQUIRE(status == 0);
    status = geom->findEdges(true);
    REQUIRE(status == 1);

    status = geom->findUnsharedEdges(false);
    REQUIRE(status == 0);
    status = geom->findUnsharedEdges(true);
    REQUIRE(status == 1);

    status = geom->findFaces(false);
    REQUIRE(status == 0);
    status = geom->findFaces(true);
    REQUIRE(status == 1);

    status = geom->findUnsharedFaces(false);
    REQUIRE(status == 0);
    status = geom->findUnsharedFaces(true);
    REQUIRE(status == 1);
  }

  SECTION("Hexahedral Geometry")
  {
    const DataPath srcGeoPath({k_HexGeo});
    auto* geom = dataStruct.getDataAs<HexahedralGeom>(srcGeoPath);

    IGeometry::StatusCode status = geom->findElementSizes(false);
    REQUIRE(status == 0);
    status = geom->findElementSizes(true);
    REQUIRE(status == 1);

    status = geom->findElementsContainingVert(false);
    REQUIRE(status == 0);
    status = geom->findElementsContainingVert(true);
    REQUIRE(status == 1);

    status = geom->findElementNeighbors(false);
    REQUIRE(status == 0);
    status = geom->findElementNeighbors(true);
    REQUIRE(status == 1);

    status = geom->findElementCentroids(false);
    REQUIRE(status == 0);
    status = geom->findElementCentroids(true);
    REQUIRE(status == 1);

    status = geom->findEdges(false);
    REQUIRE(status == 0);
    status = geom->findEdges(true);
    REQUIRE(status == 1);

    status = geom->findUnsharedEdges(false);
    REQUIRE(status == 0);
    status = geom->findUnsharedEdges(true);
    REQUIRE(status == 1);

    status = geom->findFaces(false);
    REQUIRE(status == 0);
    status = geom->findFaces(true);
    REQUIRE(status == 1);

    status = geom->findUnsharedFaces(false);
    REQUIRE(status == 0);
    status = geom->findUnsharedFaces(true);
    REQUIRE(status == 1);
  }
}
