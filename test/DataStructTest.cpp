#include "DataStructObserver.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
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
#include "simplnx/DataStructure/Messaging/DataAddedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <nod/nod.hpp>

#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
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
  UnitTest::LoadPlugins();

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
  UnitTest::LoadPlugins();

  DataStructure dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  auto outputPath = fs::path(fmt::format("{}/exportHierarchyAsGraphViz_test.dot", unit_test::k_BinaryTestOutputDir));
  std::cout << outputPath << std::endl;
  std::ofstream output(outputPath, std::ios_base::trunc);
  dataStructure.exportHierarchyAsGraphViz(output);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// This test will ensure we don't run into runtime exceptions trying to run the functions
TEST_CASE("SimplnxCore::exportHierarchyAsText")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  auto outputPath = fs::path(fmt::format("{}/exportHierarchyAsText_test.txt", unit_test::k_BinaryTestOutputDir));
  std::cout << outputPath << std::endl;
  std::ofstream output(outputPath, std::ios_base::trunc);
  dataStructure.exportHierarchyAsText(output);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    auto* neighborList = NeighborList<int16>::Create(dataStruct, Constants::k_Int16DataSet, ShapeType{3}, levelOneGroup->getId());
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
    StringArray* testStringArray = StringArray::CreateWithValues(dataStruct, k_StringArray, vertAttMatrix->getShape(), {"stringone", "stringtwo", "stringthree", "stringfour"}, vertAttMatrix->getId());
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
    AttributeMatrix* edgeAttMatrix = AttributeMatrix::Create(dataStruct, EdgeGeom::k_EdgeAttributeMatrixName, edgeTupleShape, edgeGeom->getId());
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
    AttributeMatrix* faceAttMatrix = AttributeMatrix::Create(dataStruct, TriangleGeom::k_FaceAttributeMatrixName, faceTupleShape, triangleGeom->getId());
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
    AttributeMatrix* faceAttMatrix = AttributeMatrix::Create(dataStruct, QuadGeom::k_FaceAttributeMatrixName, faceTupleShape, quadGeom->getId());
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
    for(auto value : store)
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
      for(auto value : *dataArr)
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataStructureAddingObjectToNonBaseGroup")
{
  DataStructure dataStructure;
  auto store = std::make_shared<DataStore<int32>>(std::vector<usize>{10}, std::vector<usize>{1}, 0);
  auto* dataArray = DataArray<int32>::Create(dataStructure, "foo", std::move(store));
  REQUIRE(dataArray != nullptr);

  DataGroup* group = DataGroup::Create(dataStructure, "bar", dataArray->getId());
  REQUIRE(group == nullptr);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(EdgeGeom::k_EdgeAttributeMatrixName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(EdgeGeom::k_EdgeAttributeMatrixName);
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

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(TriangleGeom::k_FaceAttributeMatrixName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(TriangleGeom::k_FaceAttributeMatrixName);
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

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(TriangleGeom::k_SharedEdgeListName);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(TriangleGeom::k_SharedEdgeListName);
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

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(TriangleGeom::k_UnsharedEdgesListName);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(TriangleGeom::k_UnsharedEdgesListName);
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

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName);
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

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(QuadGeom::k_SharedEdgeListName);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(QuadGeom::k_SharedEdgeListName);
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

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(QuadGeom::k_UnsharedEdgesListName);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(QuadGeom::k_UnsharedEdgesListName);
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

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_SharedEdgeListName);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_SharedEdgeListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcFaceArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_SharedFacesListName);
    const DataPath destFacesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_SharedFacesListName);
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

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_UnsharedEdgesListName);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_UnsharedEdgesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);

    const DataPath srcUnsharedFacesArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_UnsharedFacesListName);
    const DataPath destUnsharedFacesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_UnsharedFacesListName);
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

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_SharedEdgeListName);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_SharedEdgeListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcFaceArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_SharedFacesListName);
    const DataPath destFacesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_SharedFacesListName);
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

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_UnsharedEdgesListName);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_UnsharedEdgesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);

    const DataPath srcUnsharedFacesArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_UnsharedFacesListName);
    const DataPath destUnsharedFacesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_UnsharedFacesListName);
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

    Result<> status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(true);
    REQUIRE(status.valid());
  }

  SECTION("Rectilinear Grid Geometry")
  {
    const DataPath srcRectGeoPath({k_RectGridGeo});
    auto* geom = dataStruct.getDataAs<RectGridGeom>(srcRectGeoPath);

    Result<> status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(true);
    REQUIRE(status.valid());
  }

  SECTION("Vertex Geometry")
  {
    const DataPath srcGeoPath({Constants::k_VertexGeometry});
    auto* geom = dataStruct.getDataAs<VertexGeom>(srcGeoPath);

    Result<> status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(true);
    REQUIRE(status.valid());
  }

  SECTION("Edge Geometry")
  {
    const DataPath srcGeoPath({k_EdgeGeo});
    auto* geom = dataStruct.getDataAs<EdgeGeom>(srcGeoPath);

    Result<> status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(true);
    REQUIRE(status.valid());

    status = geom->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geom->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geom->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geom->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geom->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geom->findElementCentroids(true);
    REQUIRE(status.valid());
  }

  SECTION("Triangle Geometry")
  {
    const DataPath srcGeoPath({Constants::k_TriangleGeometryName});
    auto* geom = dataStruct.getDataAs<TriangleGeom>(srcGeoPath);

    Result<> status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(true);
    REQUIRE(status.valid());

    status = geom->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geom->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geom->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geom->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geom->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geom->findElementCentroids(true);
    REQUIRE(status.valid());

    status = geom->findEdges(false);
    REQUIRE(status.valid());
    status = geom->findEdges(true);
    REQUIRE(status.valid());

    status = geom->findUnsharedEdges(false);
    REQUIRE(status.valid());
    status = geom->findUnsharedEdges(true);
    REQUIRE(status.valid());
  }

  SECTION("Quad Geometry")
  {
    const DataPath srcGeoPath({k_QuadGeo});
    auto* geom = dataStruct.getDataAs<QuadGeom>(srcGeoPath);

    Result<> status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(true);
    REQUIRE(status.valid());

    status = geom->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geom->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geom->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geom->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geom->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geom->findElementCentroids(true);
    REQUIRE(status.valid());

    status = geom->findEdges(false);
    REQUIRE(status.valid());
    status = geom->findEdges(true);
    REQUIRE(status.valid());

    status = geom->findUnsharedEdges(false);
    REQUIRE(status.valid());
    status = geom->findUnsharedEdges(true);
    REQUIRE(status.valid());
  }

  SECTION("Tetrahedral Geometry")
  {
    const DataPath srcGeoPath({k_TetGeo});
    auto* geom = dataStruct.getDataAs<TetrahedralGeom>(srcGeoPath);

    Result<> status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(true);
    REQUIRE(status.valid());

    status = geom->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geom->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geom->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geom->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geom->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geom->findElementCentroids(true);
    REQUIRE(status.valid());

    status = geom->findEdges(false);
    REQUIRE(status.valid());
    status = geom->findEdges(true);
    REQUIRE(status.valid());

    status = geom->findUnsharedEdges(false);
    REQUIRE(status.valid());
    status = geom->findUnsharedEdges(true);
    REQUIRE(status.valid());

    status = geom->findFaces(false);
    REQUIRE(status.valid());
    status = geom->findFaces(true);
    REQUIRE(status.valid());

    status = geom->findUnsharedFaces(false);
    REQUIRE(status.valid());
    status = geom->findUnsharedFaces(true);
    REQUIRE(status.valid());
  }

  SECTION("Hexahedral Geometry")
  {
    const DataPath srcGeoPath({k_HexGeo});
    auto* geom = dataStruct.getDataAs<HexahedralGeom>(srcGeoPath);

    Result<> status = geom->findElementSizes(false);
    REQUIRE(status.valid());
    status = geom->findElementSizes(true);
    REQUIRE(status.valid());

    status = geom->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geom->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geom->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geom->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geom->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geom->findElementCentroids(true);
    REQUIRE(status.valid());

    status = geom->findEdges(false);
    REQUIRE(status.valid());
    status = geom->findEdges(true);
    REQUIRE(status.valid());

    status = geom->findUnsharedEdges(false);
    REQUIRE(status.valid());
    status = geom->findUnsharedEdges(true);
    REQUIRE(status.valid());

    status = geom->findFaces(false);
    REQUIRE(status.valid());
    status = geom->findFaces(true);
    REQUIRE(status.valid());

    status = geom->findUnsharedFaces(false);
    REQUIRE(status.valid());
    status = geom->findUnsharedFaces(true);
    REQUIRE(status.valid());
  }
}

TEST_CASE("HexahedralGeom::findElementSizes rectangular boxes", "[DataStruct][Geometry]")
{
  DataStructure dataStructure;
  auto* hexGeomPtr = HexahedralGeom::Create(dataStructure, "Boxes");
  REQUIRE(hexGeomPtr != nullptr);
  auto* verticesPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Vertices", {16}, {3}, hexGeomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  const std::vector<float32> coordinates = {0,  0,  0,  2,  0,  0,  2,  3,  0,  0,  3,  0,  0,  0,  5,  2,  0,  5,  2,  3,  5,  0,  3,  5,
                                            10, 20, 30, 11, 20, 30, 11, 21, 30, 10, 21, 30, 10, 20, 31, 11, 20, 31, 11, 21, 31, 10, 21, 31};
  for(usize index = 0; index < coordinates.size(); index++)
  {
    (*verticesPtr)[index] = coordinates[index];
  }
  hexGeomPtr->setVertices(*verticesPtr);
  auto* cellsPtr = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, "Cells", {2}, {8}, hexGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  for(usize index = 0; index < 16; index++)
  {
    (*cellsPtr)[index] = index;
  }
  hexGeomPtr->setPolyhedraList(*cellsPtr);

  const auto result = hexGeomPtr->findElementSizes(false);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* volumesPtr = hexGeomPtr->getElementSizes();
  REQUIRE(volumesPtr != nullptr);
  REQUIRE(volumesPtr->getNumberOfTuples() == 2);
  // For this vertex order, det(v1-v0, v2-v0, v3-v0)/6 gives positive volumes.
  // The five tetrahedra contribute 5 + 5 + 10 + 5 + 5 = 30; translation preserves the unit cube's volume.
  REQUIRE((*volumesPtr)[0] == Approx(30.0f).margin(1.0e-5f));
  REQUIRE((*volumesPtr)[1] == Approx(1.0f).margin(1.0e-5f));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CompareDataStructures")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure0 = createTestDataStructure();
  DataStructure dataStructure1 = createTestDataStructure();

  UnitTest::CompareDataStructures(dataStructure0, dataStructure1);
}

TEST_CASE("CompareDataStructures-fail", "[SimplnxReview][!mayfail]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure0 = createTestDataStructure();
  DataStructure dataStructure1 = createTestDataStructure();
  dataStructure1.removeData(DataPath({Constants::k_ImageGeometry}));

  UnitTest::CompareDataStructures(dataStructure0, dataStructure1);
}

namespace
{
/**
 * @brief Creates a small resident fixture with explicit IDs and literal values.
 * @param structure Owns the array.
 * @param name Identifies the root array.
 * @param objectId Supplies the fixture ID.
 * @param values Supplies the independent expected values.
 * @return The exact inserted array owner.
 */
std::shared_ptr<Int32Array> createImportPublicationArray(DataStructure& structure, const std::string& name, DataObject::IdType objectId, const std::array<int32, 2>& values)
{
  auto store = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0);
  (*store)[0] = values[0];
  (*store)[1] = values[1];
  REQUIRE(Int32Array::Import(structure, name, objectId, store) != nullptr);
  auto owner = std::dynamic_pointer_cast<Int32Array>(structure.getSharedData(objectId));
  REQUIRE(owner != nullptr);
  return owner;
}

/**
 * @class ImportCopyFailureGroup
 * @brief Throws during detached-copy construction to exercise association restoration.
 */
class ImportCopyFailureGroup : public DataGroup
{
public:
  /**
   * @brief Creates a group whose shallow copy fails.
   * @param structure Supplies the source association.
   */
  explicit ImportCopyFailureGroup(DataStructure& structure)
  : DataGroup(structure, "CopyFailure")
  {
  }

  ~ImportCopyFailureGroup() noexcept override = default;
  ImportCopyFailureGroup(const ImportCopyFailureGroup&) = default;
  ImportCopyFailureGroup(ImportCopyFailureGroup&&) = delete;
  ImportCopyFailureGroup& operator=(const ImportCopyFailureGroup&) = delete;
  ImportCopyFailureGroup& operator=(ImportCopyFailureGroup&&) = delete;

  /**
   * @brief Throws before a new owner is constructed.
   * @return Does not return.
   * @throws std::runtime_error Always, as the fixture's copy failure.
   */
  DataObject* shallowCopy() override
  {
    throw std::runtime_error("import detached-copy failure witness");
  }
};
} // namespace

TEST_CASE("Import publication: group acceptance queries preserve state and virtual rules", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  DataStructure destination;
  auto* parentPtr = DataGroup::Create(destination, "Parent");
  REQUIRE(parentPtr != nullptr);
  auto* childPtr = DataGroup::Create(destination, "Child", parentPtr->getId());
  REQUIRE(childPtr != nullptr);
  auto* matrixPtr = AttributeMatrix::Create(destination, "Matrix", ShapeType{2});
  REQUIRE(matrixPtr != nullptr);
  DataStructure candidates;
  auto matching = createImportPublicationArray(candidates, "Matching", 10, std::array<int32, 2>{31, -7});
  auto mismatchStore = std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{1}, 0);
  auto* mismatchPtr = Int32Array::Create(candidates, "Mismatch", mismatchStore);
  REQUIRE(mismatchPtr != nullptr);
  auto* duplicateNamePtr = DataGroup::Create(candidates, "Child");
  REQUIRE(duplicateNamePtr != nullptr);
  const auto paths = destination.getAllDataPaths();
  const auto ids = destination.getAllDataObjectIds();
  const auto nextId = destination.getNextId();
  const auto parents = childPtr->getParentIds();
  const auto candidatePaths = candidates.getAllDataPaths();
  const auto candidateNextId = candidates.getNextId();
  usize notifications = 0;
  const nod::scoped_connection connection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const BaseGroup& parentView = *parentPtr;
  const BaseGroup& matrixView = *matrixPtr;
  CHECK_FALSE(parentView.acceptsChild(nullptr));
  CHECK_FALSE(parentView.acceptsChild(childPtr));
  CHECK_FALSE(parentView.acceptsChild(duplicateNamePtr));
  CHECK_FALSE(childPtr->acceptsChild(parentPtr));
  CHECK(parentView.acceptsChild(matching.get()));
  CHECK_FALSE(matrixView.acceptsChild(parentPtr));
  CHECK_FALSE(matrixView.acceptsChild(mismatchPtr));
  CHECK(matrixView.acceptsChild(matching.get()));
  CHECK(destination.getAllDataPaths() == paths);
  CHECK(destination.getAllDataObjectIds() == ids);
  CHECK(destination.getNextId() == nextId);
  CHECK(childPtr->getParentIds() == parents);
  CHECK(candidates.getAllDataPaths() == candidatePaths);
  CHECK(candidates.getNextId() == candidateNextId);
  CHECK(matching->getParentIds().empty());
  CHECK(notifications == 0);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(candidates);
}

TEST_CASE("Import publication: occupied IDs include expired entries and hierarchy-only aliases", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  DataStructure destination;
  auto* markerPtr = DataGroup::Create(destination, "Marker");
  REQUIRE(markerPtr != nullptr);
  auto* expiredPtr = DataGroup::Create(destination, "Expired");
  REQUIRE(expiredPtr != nullptr);
  REQUIRE(markerPtr->getId() == 1);
  REQUIRE(expiredPtr->getId() == 2);
  REQUIRE(destination.removeData(DataObject::IdType{2}));
  REQUIRE(destination.getData(DataObject::IdType{2}) == nullptr);
  DataStructure prepared;
  auto* sourcePtr = DataGroup::Import(prepared, "HierarchyOnly", 7);
  REQUIRE(sourcePtr != nullptr);
  auto shell = DataStructure::makeImportPublicationCopy(*sourcePtr);
  REQUIRE(shell != nullptr);
  // Raw hierarchy placement models the same unregistered stage as a partial import.
  REQUIRE(destination.getRootGroup().insert(shell));
  REQUIRE(markerPtr->getDataMap().insert(shell));
  CHECK(destination.getData(DataObject::IdType{7}) == nullptr);
  const auto nextId = destination.getNextId();
  CHECK(destination.getLiveObjectCount() == 1);
  CHECK((destination.getOccupiedObjectIds() == std::set<DataObject::IdType>{1, 2, 7}));
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2}));
  CHECK(destination.getNextId() == nextId);
  const auto rootPlacement = destination.getRootGroup().find(7);
  REQUIRE(rootPlacement != destination.getRootGroup().end());
  CHECK(rootPlacement->second == shell);
  const auto childPlacement = markerPtr->getDataMap().find(7);
  REQUIRE(childPlacement != markerPtr->getDataMap().end());
  CHECK(childPlacement->second == shell);
  REQUIRE(markerPtr->getDataMap().remove(DataObject::IdType{7}));
  REQUIRE(destination.getRootGroup().remove(DataObject::IdType{7}));
  CHECK(shell->getDataStructure() == nullptr);
  UnitTest::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("Import publication: detached copies preserve the source and share only value storage", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  DataStructure source;
  auto* parentPtr = DataGroup::Create(source, "Parent");
  REQUIRE(parentPtr != nullptr);
  auto store = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0);
  (*store)[0] = 31;
  (*store)[1] = -7;
  auto* arrayPtr = Int32Array::Create(source, "Values", store, parentPtr->getId());
  REQUIRE(arrayPtr != nullptr);
  const auto arrayId = arrayPtr->getId();
  const auto parentIds = arrayPtr->getParentIds();
  const auto paths = source.getAllDataPaths();
  const auto nextId = source.getNextId();
  usize notifications = 0;
  const nod::scoped_connection connection(source.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  {
    const auto groupCopy = std::dynamic_pointer_cast<BaseGroup>(DataStructure::makeImportPublicationCopy(*parentPtr));
    REQUIRE(groupCopy != nullptr);
    CHECK(groupCopy.get() != parentPtr);
    CHECK(groupCopy->getId() == parentPtr->getId());
    CHECK(groupCopy->empty());
    CHECK(groupCopy->getDataStructure() == nullptr);
    CHECK(groupCopy->getParentIds().empty());
    const auto arrayCopy = std::dynamic_pointer_cast<Int32Array>(DataStructure::makeImportPublicationCopy(*arrayPtr));
    REQUIRE(arrayCopy != nullptr);
    CHECK(arrayCopy.get() != arrayPtr);
    CHECK(arrayCopy->getId() == arrayId);
    CHECK(arrayCopy->getDataStructure() == nullptr);
    CHECK(arrayCopy->getParentIds().empty());
    CHECK(arrayCopy->getIDataStore() == store.get());
    CHECK((*arrayCopy)[0] == 31);
    CHECK((*arrayCopy)[1] == -7);
  }
  CHECK(parentPtr->getDataStructure() == &source);
  CHECK(arrayPtr->getDataStructure() == &source);
  CHECK(arrayPtr->getParentIds() == parentIds);
  CHECK(parentPtr->getSize() == 1);
  CHECK(source.getData(arrayId) == arrayPtr);
  CHECK(source.getAllDataPaths() == paths);
  CHECK(source.getNextId() == nextId);
  CHECK(notifications == 0);
  auto failing = std::make_shared<ImportCopyFailureGroup>(source);
  REQUIRE(source.insert(failing, DataPath{}));
  const auto failureId = failing->getId();
  const auto failurePaths = source.getAllDataPaths();
  const auto failureNextId = source.getNextId();
  const auto beforeFailure = notifications;
  REQUIRE_THROWS_AS(DataStructure::makeImportPublicationCopy(*failing), std::runtime_error);
  CHECK(failing->getDataStructure() == &source);
  CHECK(source.getData(failureId) == failing.get());
  CHECK(source.getAllDataPaths() == failurePaths);
  CHECK(source.getNextId() == failureNextId);
  CHECK(notifications == beforeFailure);
  UnitTest::CheckArraysInheritTupleDims(source);
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS

TEST_CASE("Import publication: existing root names reject the complete selection before publication", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto marker = createImportPublicationArray(destination, "Marker", 1, std::array<int32, 2>{51, 52});
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto candidate = createImportPublicationArray(prepared, "Candidate", 2, std::array<int32, 2>{31, -7});
  auto conflict = createImportPublicationArray(prepared, "Marker", 3, std::array<int32, 2>{81, 82});
  const auto* markerStorePtr = marker->getIDataStore();
  const auto* candidateStorePtr = candidate->getIDataStore();
  const auto* conflictStorePtr = conflict->getIDataStore();
  const std::vector<DataPath> selected{DataPath({"Candidate"}), DataPath({"Marker"})};
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::BeforePublication, false);
  usize notifications = 0;
  const nod::scoped_connection destinationConnection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const nod::scoped_connection preparedConnection(prepared.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const auto result = instrumented ? control.publish(destination, prepared, selected, fs::path("root-name-conflict.dream3d"), checkpoint) :
                                     destination.publishImportedObjects(prepared, selected, fs::path("root-name-conflict.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == -6203);
  CHECK(result.errors().front().message.find("'Marker'") != std::string::npos);
  CHECK(result.errors().front().message.find("root-name-conflict.dream3d") != std::string::npos);
  CHECK(result.errors().front().message.find("destination path already exists") != std::string::npos);
  CHECK(result.warnings().empty());
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1}));
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Marker"})}));
  CHECK(destination.getData(DataObject::IdType{1}) == marker.get());
  CHECK(destination.getData(DataPath({"Marker"})) == marker.get());
  CHECK(destination.getData(DataObject::IdType{2}) == nullptr);
  CHECK(destination.getData(DataObject::IdType{3}) == nullptr);
  CHECK(destination.getNextId() == 2);
  CHECK(marker->getIDataStore() == markerStorePtr);
  CHECK((*marker)[0] == 51);
  CHECK((*marker)[1] == 52);
  CHECK((prepared.getAllDataObjectIds() == std::vector<DataObject::IdType>{2, 3}));
  CHECK((prepared.getAllDataPaths() == std::vector<DataPath>{DataPath({"Candidate"}), DataPath({"Marker"})}));
  CHECK(prepared.getData(DataObject::IdType{2}) == candidate.get());
  CHECK(prepared.getData(DataObject::IdType{3}) == conflict.get());
  CHECK(candidate->getDataStructure() == &prepared);
  CHECK(conflict->getDataStructure() == &prepared);
  CHECK(candidate->getParentIds().empty());
  CHECK(conflict->getParentIds().empty());
  CHECK(candidate->getIDataStore() == candidateStorePtr);
  CHECK(conflict->getIDataStore() == conflictStorePtr);
  CHECK((*candidate)[0] == 31);
  CHECK((*candidate)[1] == -7);
  CHECK((*conflict)[0] == 81);
  CHECK((*conflict)[1] == 82);
  CHECK(prepared.getNextId() == 4);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);

  if(instrumented)
  {
    const auto fault = control.publish(destination, prepared, {DataPath({"Candidate"})}, fs::path("root-name-conflict.dream3d"), checkpoint);
    SIMPLNX_RESULT_REQUIRE_INVALID(fault);
    REQUIRE(fault.errors().size() == 1);
    CHECK(fault.errors().front().code == -6214);
    REQUIRE(control.publicationFaultConsumed());
    CHECK(destination.getNextId() == 2);
    CHECK(destination.getData(DataObject::IdType{2}) == nullptr);
    CHECK(notifications == 0);
  }
  const auto retry = instrumented ? control.publish(destination, prepared, {DataPath({"Candidate"})}, fs::path("root-name-conflict.dream3d"), checkpoint) :
                                    destination.publishImportedObjects(prepared, {DataPath({"Candidate"})}, fs::path("root-name-conflict.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_VALID(retry);
  auto published = std::dynamic_pointer_cast<Int32Array>(destination.getSharedData(DataObject::IdType{2}));
  REQUIRE(published != nullptr);
  CHECK(published.get() != candidate.get());
  CHECK(published->getIDataStore() == candidateStorePtr);
  CHECK((*published)[0] == 31);
  CHECK((*published)[1] == -7);
  CHECK(destination.getData(DataPath({"Marker"})) == marker.get());
  CHECK(destination.getData(DataPath({"Candidate"})) == published.get());
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2}));
  CHECK(destination.getNextId() == 3);
  CHECK(prepared.getData(DataObject::IdType{2}) == candidate.get());
  CHECK(prepared.getData(DataObject::IdType{3}) == conflict.get());
  CHECK(prepared.getNextId() == 4);
  CHECK(notifications == 0);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: hierarchy-only IDs in another parent reject before publication", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto* parentPtr = DataGroup::Import(destination, "Parent", 1);
  REQUIRE(parentPtr != nullptr);
  DataStructure source;
  auto* sourcePtr = DataGroup::Import(source, "HierarchyOnly", 7);
  REQUIRE(sourcePtr != nullptr);
  auto occupied = DataStructure::makeImportPublicationCopy(*sourcePtr);
  REQUIRE(occupied != nullptr);
  // The child map owns ID 7, but the weak registry and the destination root do not.
  REQUIRE(parentPtr->getDataMap().insert(occupied));
  REQUIRE(destination.getData(DataObject::IdType{7}) == nullptr);
  REQUIRE(destination.getRootGroup().find(7) == destination.getRootGroup().end());
  REQUIRE(destination.getData(DataPath({"Parent", "HierarchyOnly"})) == occupied.get());
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto candidate = createImportPublicationArray(prepared, "Candidate", 7, std::array<int32, 2>{31, -7});
  const auto* candidateStorePtr = candidate->getIDataStore();
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::BeforePublication, false);
  usize notifications = 0;
  const nod::scoped_connection destinationConnection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const nod::scoped_connection preparedConnection(prepared.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const auto result = instrumented ? control.publish(destination, prepared, {DataPath({"Candidate"})}, fs::path("hierarchy-id-conflict.dream3d"), checkpoint) :
                                     destination.publishImportedObjects(prepared, {DataPath({"Candidate"})}, fs::path("hierarchy-id-conflict.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == -6213);
  CHECK(result.errors().front().message.find("'Candidate'") != std::string::npos);
  CHECK(result.errors().front().message.find("identifier 7") != std::string::npos);
  CHECK(result.errors().front().message.find("hierarchy-id-conflict.dream3d") != std::string::npos);
  CHECK(result.warnings().empty());
  CHECK(destination.getData(DataObject::IdType{1}) == parentPtr);
  CHECK(destination.getData(DataObject::IdType{7}) == nullptr);
  CHECK(destination.getData(DataPath({"Parent"})) == parentPtr);
  CHECK(destination.getData(DataPath({"Parent", "HierarchyOnly"})) == occupied.get());
  CHECK(destination.getData(DataPath({"Candidate"})) == nullptr);
  CHECK(destination.getRootGroup().getSize() == 1);
  CHECK(parentPtr->getSize() == 1);
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1}));
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Parent"})}));
  CHECK((destination.getOccupiedObjectIds() == std::set<DataObject::IdType>{1, 7}));
  CHECK(destination.getNextId() == 2);
  CHECK(occupied->getId() == 7);
  CHECK(occupied->getDataStructure() == nullptr);
  CHECK(occupied->getParentIds().empty());
  CHECK((prepared.getAllDataObjectIds() == std::vector<DataObject::IdType>{7}));
  CHECK((prepared.getAllDataPaths() == std::vector<DataPath>{DataPath({"Candidate"})}));
  CHECK(prepared.getData(DataObject::IdType{7}) == candidate.get());
  CHECK(candidate->getDataStructure() == &prepared);
  CHECK(candidate->getParentIds().empty());
  CHECK(candidate->getIDataStore() == candidateStorePtr);
  CHECK((*candidate)[0] == 31);
  CHECK((*candidate)[1] == -7);
  CHECK(prepared.getNextId() == 8);
  CHECK(source.getData(DataObject::IdType{7}) == sourcePtr);
  CHECK(sourcePtr->getDataStructure() == &source);
  CHECK(source.getNextId() == 8);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);
  REQUIRE(parentPtr->getDataMap().remove(DataObject::IdType{7}));
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: duplicate selected paths reject before publication", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto* markerPtr = DataGroup::Import(destination, "Marker", 1);
  REQUIRE(markerPtr != nullptr);
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto candidate = createImportPublicationArray(prepared, "Candidate", 2, std::array<int32, 2>{31, -7});
  const auto* storePtr = candidate->getIDataStore();
  const std::vector<DataPath> selected{DataPath({"Candidate"}), DataPath({"Candidate"})};
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::BeforePublication, false);
  usize notifications = 0;
  const nod::scoped_connection destinationConnection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const nod::scoped_connection preparedConnection(prepared.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const auto result = instrumented ? control.publish(destination, prepared, selected, fs::path("duplicate-path.dream3d"), checkpoint) :
                                     destination.publishImportedObjects(prepared, selected, fs::path("duplicate-path.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == -6203);
  CHECK(result.errors().front().message.find("'Candidate'") != std::string::npos);
  CHECK(result.errors().front().message.find("duplicate-path.dream3d") != std::string::npos);
  CHECK(result.errors().front().message.find("same path is selected more than once") != std::string::npos);
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1}));
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Marker"})}));
  CHECK(destination.getData(DataObject::IdType{1}) == markerPtr);
  CHECK(destination.getData(DataObject::IdType{2}) == nullptr);
  CHECK(destination.getNextId() == 2);
  CHECK((prepared.getAllDataObjectIds() == std::vector<DataObject::IdType>{2}));
  CHECK((prepared.getAllDataPaths() == std::vector<DataPath>{DataPath({"Candidate"})}));
  CHECK(prepared.getData(DataObject::IdType{2}) == candidate.get());
  CHECK(candidate->getDataStructure() == &prepared);
  CHECK(candidate->getParentIds().empty());
  CHECK(candidate->getIDataStore() == storePtr);
  CHECK((*candidate)[0] == 31);
  CHECK((*candidate)[1] == -7);
  CHECK(prepared.getNextId() == 3);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: distinct selected alias paths cannot publish the same ID twice", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto* leftPtr = DataGroup::Import(destination, "Left", 1);
  auto* rightPtr = DataGroup::Import(destination, "Right", 2);
  REQUIRE(leftPtr != nullptr);
  REQUIRE(rightPtr != nullptr);
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto* preparedLeftPtr = DataGroup::Import(prepared, "Left", 1);
  auto* preparedRightPtr = DataGroup::Import(prepared, "Right", 2);
  REQUIRE(preparedLeftPtr != nullptr);
  REQUIRE(preparedRightPtr != nullptr);
  auto store = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0);
  (*store)[0] = 31;
  (*store)[1] = -7;
  REQUIRE(Int32Array::Import(prepared, "Candidate", 3, store, DataObject::IdType{1}) != nullptr);
  auto candidate = std::dynamic_pointer_cast<Int32Array>(prepared.getSharedData(DataObject::IdType{3}));
  REQUIRE(candidate != nullptr);
  REQUIRE(prepared.setAdditionalParent(3, 2));
  const std::vector<DataPath> selected{DataPath({"Left", "Candidate"}), DataPath({"Right", "Candidate"})};
  REQUIRE(prepared.getData(selected[0]) == candidate.get());
  REQUIRE(prepared.getData(selected[1]) == candidate.get());
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::BeforePublication, false);
  usize notifications = 0;
  const nod::scoped_connection destinationConnection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const nod::scoped_connection preparedConnection(prepared.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const auto result = instrumented ? control.publish(destination, prepared, selected, fs::path("duplicate-id.dream3d"), checkpoint) :
                                     destination.publishImportedObjects(prepared, selected, fs::path("duplicate-id.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == -6213);
  CHECK(result.errors().front().message.find("Right/Candidate") != std::string::npos);
  CHECK(result.errors().front().message.find("identifier 3") != std::string::npos);
  CHECK(result.errors().front().message.find("duplicate-id.dream3d") != std::string::npos);
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2}));
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Left"}), DataPath({"Right"})}));
  CHECK(destination.getData(DataObject::IdType{1}) == leftPtr);
  CHECK(destination.getData(DataObject::IdType{2}) == rightPtr);
  CHECK(destination.getData(DataObject::IdType{3}) == nullptr);
  CHECK(leftPtr->empty());
  CHECK(rightPtr->empty());
  CHECK(destination.getNextId() == 3);
  CHECK((prepared.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2, 3}));
  CHECK((prepared.getAllDataPaths() == std::vector<DataPath>{DataPath({"Left"}), DataPath({"Right"}), DataPath({"Left", "Candidate"}), DataPath({"Right", "Candidate"})}));
  CHECK(prepared.getData(DataObject::IdType{1}) == preparedLeftPtr);
  CHECK(prepared.getData(DataObject::IdType{2}) == preparedRightPtr);
  CHECK(prepared.getData(DataObject::IdType{3}) == candidate.get());
  CHECK(prepared.getData(selected[0]) == candidate.get());
  CHECK(prepared.getData(selected[1]) == candidate.get());
  CHECK(candidate->getDataStructure() == &prepared);
  CHECK((candidate->getParentIds() == DataObject::ParentCollectionType{1, 2}));
  CHECK(candidate->getIDataStore() == store.get());
  CHECK((*candidate)[0] == 31);
  CHECK((*candidate)[1] == -7);
  CHECK(prepared.getNextId() == 4);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: distinct parent aliases reject duplicate destination placements", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto* leftPtr = DataGroup::Import(destination, "Left", 1);
  auto* rightPtr = DataGroup::Import(destination, "Right", 2);
  REQUIRE(leftPtr != nullptr);
  REQUIRE(rightPtr != nullptr);
  auto* sharedParentPtr = DataGroup::Import(destination, "Shared", 3, DataObject::IdType{1});
  REQUIRE(sharedParentPtr != nullptr);
  REQUIRE(destination.setAdditionalParent(3, 2));
  REQUIRE(destination.getData(DataPath({"Left", "Shared"})) == sharedParentPtr);
  REQUIRE(destination.getData(DataPath({"Right", "Shared"})) == sharedParentPtr);
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto* preparedLeftPtr = DataGroup::Import(prepared, "Left", 1);
  auto* preparedRightPtr = DataGroup::Import(prepared, "Right", 2);
  REQUIRE(preparedLeftPtr != nullptr);
  REQUIRE(preparedRightPtr != nullptr);
  auto* firstParentPtr = DataGroup::Import(prepared, "Shared", 3, DataObject::IdType{1});
  auto* secondParentPtr = DataGroup::Import(prepared, "Shared", 4, DataObject::IdType{2});
  REQUIRE(firstParentPtr != nullptr);
  REQUIRE(secondParentPtr != nullptr);
  REQUIRE(firstParentPtr != secondParentPtr);
  auto firstStore = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0);
  (*firstStore)[0] = 31;
  (*firstStore)[1] = -7;
  auto secondStore = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0);
  (*secondStore)[0] = 81;
  (*secondStore)[1] = 82;
  auto* firstPtr = Int32Array::Import(prepared, "Candidate", 5, firstStore, DataObject::IdType{3});
  auto* secondPtr = Int32Array::Import(prepared, "Candidate", 6, secondStore, DataObject::IdType{4});
  REQUIRE(firstPtr != nullptr);
  REQUIRE(secondPtr != nullptr);
  const std::vector<DataPath> selected{DataPath({"Left", "Shared", "Candidate"}), DataPath({"Right", "Shared", "Candidate"})};
  REQUIRE(prepared.getData(selected[0]) == firstPtr);
  REQUIRE(prepared.getData(selected[1]) == secondPtr);
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::BeforePublication, false);
  usize notifications = 0;
  const nod::scoped_connection destinationConnection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const nod::scoped_connection preparedConnection(prepared.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const auto result = instrumented ? control.publish(destination, prepared, selected, fs::path("aliased-parent.dream3d"), checkpoint) :
                                     destination.publishImportedObjects(prepared, selected, fs::path("aliased-parent.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == -6203);
  CHECK(result.errors().front().message.find("Right/Shared/Candidate") != std::string::npos);
  CHECK(result.errors().front().message.find("aliased-parent.dream3d") != std::string::npos);
  CHECK(result.errors().front().message.find("destination placement is selected more than once") != std::string::npos);
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2, 3}));
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Left"}), DataPath({"Right"}), DataPath({"Left", "Shared"}), DataPath({"Right", "Shared"})}));
  CHECK(destination.getData(DataObject::IdType{1}) == leftPtr);
  CHECK(destination.getData(DataObject::IdType{2}) == rightPtr);
  CHECK(destination.getData(DataObject::IdType{3}) == sharedParentPtr);
  CHECK(destination.getData(DataObject::IdType{5}) == nullptr);
  CHECK(destination.getData(DataObject::IdType{6}) == nullptr);
  CHECK(destination.getData(selected[0]) == nullptr);
  CHECK(destination.getData(selected[1]) == nullptr);
  CHECK(sharedParentPtr->empty());
  CHECK((sharedParentPtr->getParentIds() == DataObject::ParentCollectionType{1, 2}));
  CHECK(destination.getNextId() == 4);
  CHECK((prepared.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2, 3, 4, 5, 6}));
  CHECK((prepared.getAllDataPaths() == std::vector<DataPath>{DataPath({"Left"}), DataPath({"Right"}), DataPath({"Left", "Shared"}), DataPath({"Right", "Shared"}),
                                                             DataPath({"Left", "Shared", "Candidate"}), DataPath({"Right", "Shared", "Candidate"})}));
  CHECK(prepared.getData(DataObject::IdType{1}) == preparedLeftPtr);
  CHECK(prepared.getData(DataObject::IdType{2}) == preparedRightPtr);
  CHECK(prepared.getData(DataObject::IdType{3}) == firstParentPtr);
  CHECK(prepared.getData(DataObject::IdType{4}) == secondParentPtr);
  CHECK(prepared.getData(DataObject::IdType{5}) == firstPtr);
  CHECK(prepared.getData(DataObject::IdType{6}) == secondPtr);
  CHECK(prepared.getData(selected[0]) == firstPtr);
  CHECK(prepared.getData(selected[1]) == secondPtr);
  CHECK(firstPtr->getDataStructure() == &prepared);
  CHECK(secondPtr->getDataStructure() == &prepared);
  CHECK((firstPtr->getParentIds() == DataObject::ParentCollectionType{3}));
  CHECK((secondPtr->getParentIds() == DataObject::ParentCollectionType{4}));
  CHECK(firstPtr->getIDataStore() == firstStore.get());
  CHECK(secondPtr->getIDataStore() == secondStore.get());
  CHECK((*firstPtr)[0] == 31);
  CHECK((*firstPtr)[1] == -7);
  CHECK((*secondPtr)[0] == 81);
  CHECK((*secondPtr)[1] == 82);
  CHECK(prepared.getNextId() == 7);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: checkpoint errors precede conflicting selection admission", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool foreign = GENERATE(false, true);
  const bool instrumented = GENERATE(false, true);
  CAPTURE(foreign, instrumented);
  const DataStructure original;
  DataStructure destination;
  auto* markerPtr = DataGroup::Import(destination, "Marker", 1);
  REQUIRE(markerPtr != nullptr);
  const DataStructure::ImportPublicationCheckpoint checkpoint(foreign ? original : destination);
  auto* laterPtr = DataGroup::Import(destination, "Later", 2);
  REQUIRE(laterPtr != nullptr);
  DataStructure prepared;
  auto* conflictPtr = DataGroup::Import(prepared, "Marker", 1);
  REQUIRE(conflictPtr != nullptr);
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::BeforePublication, false);
  usize notifications = 0;
  const nod::scoped_connection connection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const auto result = instrumented ? control.publish(destination, prepared, {DataPath({"Marker"}), DataPath({"Marker"})}, fs::path("checkpoint-priority.dream3d"), checkpoint) :
                                     destination.publishImportedObjects(prepared, {DataPath({"Marker"}), DataPath({"Marker"})}, fs::path("checkpoint-priority.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == (foreign ? -6217 : -6218));
  CHECK(destination.getData(DataObject::IdType{1}) == markerPtr);
  CHECK(destination.getData(DataObject::IdType{2}) == laterPtr);
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Marker"}), DataPath({"Later"})}));
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2}));
  CHECK(destination.getNextId() == 3);
  CHECK(prepared.getData(DataObject::IdType{1}) == conflictPtr);
  CHECK(prepared.getNextId() == 2);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: empty and fully recovered selections preserve pending faults", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool empty = GENERATE(false, true);
  const bool instrumented = GENERATE(false, true);
  CAPTURE(empty, instrumented);
  DataStructure destination;
  auto* markerPtr = DataGroup::Import(destination, "Marker", 1);
  REQUIRE(markerPtr != nullptr);
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  const std::vector<DataPath> selected = empty ? std::vector<DataPath>{} : std::vector<DataPath>{DataPath({"Missing"}), DataPath({"Marker"}), DataPath({"Missing"})};
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::BeforePublication, false);
  usize notifications = 0;
  const nod::scoped_connection destinationConnection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const nod::scoped_connection preparedConnection(prepared.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const auto result = instrumented ? control.publish(destination, prepared, selected, fs::path("empty-recovery.dream3d"), checkpoint) :
                                     destination.publishImportedObjects(prepared, selected, fs::path("empty-recovery.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.warnings().empty());
  CHECK(destination.getData(DataObject::IdType{1}) == markerPtr);
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1}));
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Marker"})}));
  CHECK(destination.getNextId() == 2);
  CHECK(prepared.getAllDataObjectIds().empty());
  CHECK(prepared.getAllDataPaths().empty());
  CHECK(prepared.getNextId() == 1);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: missing recovered paths are skipped before admission", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto* markerPtr = DataGroup::Import(destination, "Marker", 1);
  REQUIRE(markerPtr != nullptr);
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto candidate = createImportPublicationArray(prepared, "Candidate", 2, std::array<int32, 2>{31, -7});
  const std::vector<DataPath> selected{DataPath({"Missing"}), DataPath({"Marker"}), DataPath({"Missing"}), DataPath({"Candidate"})};
  DataStructure::ImportPublicationTestControl control;
  usize notifications = 0;
  const nod::scoped_connection destinationConnection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const nod::scoped_connection preparedConnection(prepared.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const auto result = instrumented ? control.publish(destination, prepared, selected, fs::path("recovered-paths.dream3d"), checkpoint) :
                                     destination.publishImportedObjects(prepared, selected, fs::path("recovered-paths.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.warnings().empty());
  auto published = std::dynamic_pointer_cast<Int32Array>(destination.getSharedData(DataObject::IdType{2}));
  REQUIRE(published != nullptr);
  CHECK(published.get() != candidate.get());
  CHECK(published->getIDataStore() == candidate->getIDataStore());
  CHECK((*published)[0] == 31);
  CHECK((*published)[1] == -7);
  CHECK(destination.getData(DataObject::IdType{1}) == markerPtr);
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2}));
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Marker"}), DataPath({"Candidate"})}));
  CHECK(destination.getNextId() == 3);
  CHECK(prepared.getData(DataObject::IdType{2}) == candidate.get());
  CHECK(candidate->getDataStructure() == &prepared);
  CHECK(candidate->getParentIds().empty());
  CHECK((prepared.getAllDataObjectIds() == std::vector<DataObject::IdType>{2}));
  CHECK((prepared.getAllDataPaths() == std::vector<DataPath>{DataPath({"Candidate"})}));
  CHECK(prepared.getNextId() == 3);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: foreign checkpoints reject before fault consumption", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  DataStructure originalDestination;
  auto originalMarker = createImportPublicationArray(originalDestination, "OriginalMarker", 1, std::array<int32, 2>{41, 42});
  const DataStructure::ImportPublicationCheckpoint checkpoint(originalDestination);
  DataStructure destination;
  auto marker = createImportPublicationArray(destination, "Marker", 1, std::array<int32, 2>{51, 52});
  const auto* markerStorePtr = marker->getIDataStore();
  DataStructure prepared;
  auto candidate = createImportPublicationArray(prepared, "Candidate", 2, std::array<int32, 2>{31, -7});
  const auto* candidateStorePtr = candidate->getIDataStore();
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::BeforePublication, false);
  usize notifications = 0;
  const nod::scoped_connection originalConnection(originalDestination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const nod::scoped_connection destinationConnection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  Result<> result;
  if(instrumented)
  {
    result = control.publish(destination, prepared, {DataPath({"Candidate"})}, fs::path("foreign-checkpoint.dream3d"), checkpoint);
  }
  else
  {
    result = destination.publishImportedObjects(prepared, {DataPath({"Candidate"})}, fs::path("foreign-checkpoint.dream3d"), checkpoint);
  }
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == -6217);
  CHECK(result.errors().front().message.find("foreign-checkpoint.dream3d") != std::string::npos);
  CHECK((originalDestination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1}));
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1}));
  CHECK((originalDestination.getAllDataPaths() == std::vector<DataPath>{DataPath({"OriginalMarker"})}));
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Marker"})}));
  CHECK(originalDestination.getNextId() == 2);
  CHECK(destination.getNextId() == 2);
  CHECK(originalDestination.getData(DataObject::IdType{1}) == originalMarker.get());
  CHECK(destination.getData(DataObject::IdType{1}) == marker.get());
  CHECK(marker->getIDataStore() == markerStorePtr);
  CHECK((*marker)[0] == 51);
  CHECK((*marker)[1] == 52);
  CHECK(prepared.getData(DataObject::IdType{2}) == candidate.get());
  CHECK(candidate->getDataStructure() == &prepared);
  CHECK(candidate->getIDataStore() == candidateStorePtr);
  CHECK(prepared.getNextId() == 3);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);
  UnitTest::CheckArraysInheritTupleDims(originalDestination);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: stale checkpoints preserve successful imports and ordinary insertions", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  const bool previousPublication = GENERATE(false, true);
  const bool instrumented = GENERATE(false, true);
  CAPTURE(previousPublication, instrumented);
  DataStructure destination;
  auto marker = createImportPublicationArray(destination, "Marker", 1, std::array<int32, 2>{51, 52});
  const auto* markerStorePtr = marker->getIDataStore();
  const DataStructure::ImportPublicationCheckpoint originalCheckpoint(destination);
  const auto checkpoint = originalCheckpoint;
  const std::string retainedName = previousPublication ? "ImportedA" : "Unrelated";
  if(previousPublication)
  {
    DataStructure firstPrepared;
    createImportPublicationArray(firstPrepared, retainedName, 2, std::array<int32, 2>{61, 62});
    const auto first = destination.publishImportedObjects(firstPrepared, {DataPath({retainedName})}, fs::path("first-publication.dream3d"), originalCheckpoint);
    SIMPLNX_RESULT_REQUIRE_VALID(first);
  }
  else
  {
    auto store = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0);
    (*store)[0] = 61;
    (*store)[1] = 62;
    REQUIRE(Int32Array::Create(destination, retainedName, store) != nullptr);
  }
  auto retained = std::dynamic_pointer_cast<Int32Array>(destination.getSharedData(DataObject::IdType{2}));
  REQUIRE(retained != nullptr);
  const auto* retainedStorePtr = retained->getIDataStore();
  REQUIRE(destination.getNextId() == 3);
  DataStructure prepared;
  auto candidate = createImportPublicationArray(prepared, "ImportedB", 3, std::array<int32, 2>{31, -7});
  const auto* candidateStorePtr = candidate->getIDataStore();
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::BeforePublication, false);
  usize notifications = 0;
  const nod::scoped_connection connection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  Result<> result;
  if(instrumented)
  {
    result = control.publish(destination, prepared, {DataPath({"ImportedB"})}, fs::path("stale-checkpoint.dream3d"), checkpoint);
  }
  else
  {
    result = destination.publishImportedObjects(prepared, {DataPath({"ImportedB"})}, fs::path("stale-checkpoint.dream3d"), checkpoint);
  }
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == -6218);
  CHECK(result.errors().front().message.find("next identifier is 3") != std::string::npos);
  CHECK(result.errors().front().message.find("checkpoint recorded 2") != std::string::npos);
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2}));
  CHECK((destination.getAllDataPaths() == std::vector<DataPath>{DataPath({"Marker"}), DataPath({retainedName})}));
  CHECK(destination.getNextId() == 3);
  CHECK(destination.getData(DataObject::IdType{1}) == marker.get());
  CHECK(destination.getData(DataObject::IdType{2}) == retained.get());
  CHECK(destination.getData(DataPath({retainedName})) == retained.get());
  CHECK_FALSE(destination.containsData(DataPath({"ImportedB"})));
  CHECK(marker->getIDataStore() == markerStorePtr);
  CHECK(retained->getIDataStore() == retainedStorePtr);
  CHECK((*marker)[0] == 51);
  CHECK((*marker)[1] == 52);
  CHECK((*retained)[0] == 61);
  CHECK((*retained)[1] == 62);
  CHECK(prepared.getData(DataObject::IdType{3}) == candidate.get());
  CHECK(candidate->getDataStructure() == &prepared);
  CHECK(candidate->getIDataStore() == candidateStorePtr);
  CHECK(prepared.getNextId() == 4);
  CHECK(notifications == 0);
  CHECK_FALSE(control.publicationFaultConsumed());
  CHECK_FALSE(control.cleanupFaultConsumed());
  CHECK(control.retainedOwner() == nullptr);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: complete rollback permits checkpoint reuse without stale-owner deletion", "[ImportPublication][DataStructure]")
{
  UnitTest::LoadPlugins();
  DataStructure destination;
  auto marker = createImportPublicationArray(destination, "Marker", 1, std::array<int32, 2>{51, 52});
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto firstSource = createImportPublicationArray(prepared, "First", 2, std::array<int32, 2>{31, -7});
  auto secondSource = createImportPublicationArray(prepared, "Second", 3, std::array<int32, 2>{81, 82});
  const std::vector<DataPath> selected{DataPath({"First"}), DataPath({"Second"})};
  std::vector<DataObject::IdType> removals;
  usize additions = 0;
  const nod::scoped_connection connection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>& message) {
    if(message->getMsgType() == DataRemovedMessage::MsgType)
    {
      removals.push_back(std::static_pointer_cast<DataRemovedMessage>(message)->getId());
    }
    else if(message->getMsgType() == DataAddedMessage::MsgType)
    {
      ++additions;
    }
  }));
  DataStructure::ImportPublicationTestControl control;
  control.setFault(DataStructure::ImportPublicationTestControl::Fault::AfterSecondInsert, false);
  const auto failed = control.publish(destination, prepared, selected, fs::path("complete-rollback.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_INVALID(failed);
  REQUIRE(failed.errors().size() == 1);
  CHECK(failed.errors().front().code == -6214);
  REQUIRE(control.publicationFaultConsumed());
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1}));
  CHECK(destination.getData(DataObject::IdType{1}) == marker.get());
  CHECK(destination.getNextId() == 2);
  CHECK((removals == std::vector<DataObject::IdType>{3, 2}));
  auto detachedOwner = control.retainedOwner();
  REQUIRE(detachedOwner != nullptr);
  CHECK(detachedOwner->getId() == 3);
  CHECK(detachedOwner->getDataStructure() == nullptr);
  CHECK(detachedOwner->getParentIds().empty());
  const auto retry = control.publish(destination, prepared, selected, fs::path("complete-rollback.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_VALID(retry);
  CHECK((destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{1, 2, 3}));
  CHECK(destination.getNextId() == 4);
  auto first = std::dynamic_pointer_cast<Int32Array>(destination.getSharedData(DataObject::IdType{2}));
  auto second = std::dynamic_pointer_cast<Int32Array>(destination.getSharedData(DataObject::IdType{3}));
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  CHECK(first.get() != firstSource.get());
  CHECK(second.get() != secondSource.get());
  CHECK(second.get() != detachedOwner.get());
  CHECK(first->getIDataStore() == firstSource->getIDataStore());
  CHECK(second->getIDataStore() == secondSource->getIDataStore());
  CHECK((*first)[0] == 31);
  CHECK((*first)[1] == -7);
  CHECK((*second)[0] == 81);
  CHECK((*second)[1] == 82);
  detachedOwner.reset();
  CHECK(destination.getData(DataObject::IdType{3}) == second.get());
  CHECK((removals == std::vector<DataObject::IdType>{3, 2}));
  CHECK(additions == 0);
  UnitTest::CheckArraysInheritTupleDims(destination);
  UnitTest::CheckArraysInheritTupleDims(prepared);
}
#endif
