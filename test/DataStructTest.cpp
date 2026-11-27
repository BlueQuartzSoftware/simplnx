#include "DataStructObserver.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/Messaging/DataAddedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectExcludingAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <nod/nod.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
constexpr StringLiteralType k_RectGridGeo = "Rect Grid Geom";
constexpr StringLiteralType k_XBounds = "X Bounds";
constexpr StringLiteralType k_YBounds = "Y Bounds";
constexpr StringLiteralType k_ZBounds = "Z Bounds";
constexpr StringLiteralType k_StringArray = "String Array";
constexpr StringLiteralType k_EdgeGeo = "Edge Geometry";
constexpr StringLiteralType k_ScalarData = "scalar";
constexpr StringLiteralType k_SharedEdges = "SharedEdges";
constexpr StringLiteralType k_SharedFaces = "SharedFaces";
constexpr StringLiteralType k_QuadGeo = "Quad Geometry";
constexpr StringLiteralType k_TetGeo = "Tet Geometry";
constexpr StringLiteralType k_SharedPolyhedrons = "SharedPolyhedronList";
constexpr StringLiteralType k_HexGeo = "Hex Geometry";

enum class DynamicListMode : uint8
{
  Exclude,
  Include
};
} // namespace

TEST_CASE("SimplnxCore::DataObjectType Check")
{
  unit_test::LoadPlugins();

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

// Verify that GraphViz export completes without a runtime exception.
TEST_CASE("SimplnxCore::exportHierarchyAsGraphViz")
{
  unit_test::LoadPlugins();

  const DataStructure dataStructure = unit_test::CreateComplexMultiLevelDataGraph();
  auto outputPath = fs::path(fmt::format("{}/exportHierarchyAsGraphViz_test.dot", unit_test::k_BinaryTestOutputDir));
  std::cout << outputPath << '\n';
  std::ofstream output(outputPath, std::ios_base::trunc);
  dataStructure.exportHierarchyAsGraphViz(output);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataStructure::exportHierarchyAsGraphViz::UniqueEscapedNodes")
{
  DataStructure dataStructure;
  auto* firstParentPtr = DataGroup::Create(dataStructure, "Parent \"A\"\\Line\nNext {Record|<Port>}");
  auto* secondParentPtr = DataGroup::Create(dataStructure, "Parent B");
  REQUIRE(firstParentPtr != nullptr);
  REQUIRE(secondParentPtr != nullptr);

  auto* firstChildPtr = DataGroup::Create(dataStructure, "Shared Name", firstParentPtr->getId());
  auto* secondChildPtr = DataGroup::Create(dataStructure, "Shared Name", secondParentPtr->getId());
  REQUIRE(firstChildPtr != nullptr);
  REQUIRE(secondChildPtr != nullptr);

  std::ostringstream output;
  dataStructure.exportHierarchyAsGraphViz(output);
  const std::string graph = output.str();

  const std::string firstParentId = fmt::format("node_{}", firstParentPtr->getId());
  const std::string secondParentId = fmt::format("node_{}", secondParentPtr->getId());
  const std::string firstChildId = fmt::format("node_{}", firstChildPtr->getId());
  const std::string secondChildId = fmt::format("node_{}", secondChildPtr->getId());

  REQUIRE(graph.find(fmt::format("\"{}\" [label=\"Parent \\\"A\\\"\\\\Line\\nNext \\{{Record\\|\\<Port\\>\\}}\"]", firstParentId)) != std::string::npos);
  REQUIRE(graph.find(fmt::format("\"{}\" [label=\"Shared Name\"]", firstChildId)) != std::string::npos);
  REQUIRE(graph.find(fmt::format("\"{}\" [label=\"Shared Name\"]", secondChildId)) != std::string::npos);
  REQUIRE(graph.find(fmt::format("\"{}\" -> \"{}\"", firstParentId, firstChildId)) != std::string::npos);
  REQUIRE(graph.find(fmt::format("\"{}\" -> \"{}\"", secondParentId, secondChildId)) != std::string::npos);
  REQUIRE(firstChildId != secondChildId);
}

// Verify that text export completes without a runtime exception.
TEST_CASE("SimplnxCore::exportHierarchyAsText")
{
  unit_test::LoadPlugins();

  const DataStructure dataStructure = unit_test::CreateComplexMultiLevelDataGraph();
  auto outputPath = fs::path(fmt::format("{}/exportHierarchyAsText_test.txt", unit_test::k_BinaryTestOutputDir));
  std::cout << outputPath << '\n';
  std::ofstream output(outputPath, std::ios_base::trunc);
  dataStructure.exportHierarchyAsText(output);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataStructure::exportHierarchyAsText::EscapedControls")
{
  DataStructure dataStructure;
  REQUIRE(DataGroup::Create(dataStructure, "Name\\Path\nNext\tTab\rReturn") != nullptr);

  std::ostringstream output;
  dataStructure.exportHierarchyAsText(output);

  REQUIRE(output.str() == "|--Name\\\\Path\\nNext\\tTab\\rReturn\n");
}

namespace
{
/**
 * @brief Creates the complete DataStructure fixture used by hierarchy tests.
 * @param dynamicListMode Controls creation of derived DynamicListArray objects.
 * @return Populated test DataStructure.
 */
DataStructure CreateTestDataStructure(DynamicListMode dynamicListMode = DynamicListMode::Include)
{
  DataStructure dataStruct;
  const auto createDynamicLists = [dynamicListMode](auto& geometry) {
    if(dynamicListMode == DynamicListMode::Include)
    {
      geometry.findElementsContainingVert(false);
      geometry.findElementNeighbors(false);
    }
  };

  // Image Geometry
  {
    ImageGeom* imageGeomPtr = ImageGeom::Create(dataStruct, constants::k_ImageGeometry);
    REQUIRE(imageGeomPtr->setSpacing({0.25f, 0.55f, 1.86}).valid());
    REQUIRE(imageGeomPtr->setOrigin({0.0f, 20.0f, 66.0f}).valid());
    std::vector<usize> imageGeomDims = {40, 60, 80};
    imageGeomPtr->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

    const DataGroup* levelOneGroupPtr = DataGroup::Create(dataStruct, constants::k_LevelOne, imageGeomPtr->getId());

    const DataGroup* levelTwoGroupPtr = DataGroup::Create(dataStruct, constants::k_LevelTwo, levelOneGroupPtr->getId());
    auto* neighborListPtr = NeighborList<int16>::Create(dataStruct, constants::k_Int16DataSet, ShapeType{3}, levelOneGroupPtr->getId());
    std::vector<int16> list1 = {117, 875, 1035, 3905, 4214};
    std::vector<int16> list2 = {750, 1905, 1912, 2015, 2586, 3180, 3592, 4041, 4772};
    std::vector<int16> list3 = {309, 775, 2625, 2818, 3061, 3751, 4235, 4817};
    neighborListPtr->setList(0, std::make_shared<std::vector<int16>>(list1));
    neighborListPtr->setList(1, std::make_shared<std::vector<int16>>(list2));
    neighborListPtr->setList(2, std::make_shared<std::vector<int16>>(list3));

    Int32ArrayType* testIntArrayPtr = unit_test::CreateTestDataArray<int32>(dataStruct, constants::k_Int32DataSet, {5}, {1}, levelTwoGroupPtr->getId());
    (*testIntArrayPtr)[0] = 1;
    (*testIntArrayPtr)[1] = 5;
    (*testIntArrayPtr)[2] = 10;
    (*testIntArrayPtr)[3] = 15;
    (*testIntArrayPtr)[4] = 20;

    const std::vector<usize> reversedDims(imageGeomDims.rbegin(), imageGeomDims.rend());
    AttributeMatrix* levelOneAttMatrixPtr = AttributeMatrix::Create(dataStruct, constants::k_CellData, reversedDims, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*levelOneAttMatrixPtr);

    BoolArrayType* testBoolArrayPtr = unit_test::CreateTestDataArray<bool>(dataStruct, constants::k_ConditionalArray, reversedDims, {1}, levelOneAttMatrixPtr->getId());
    testBoolArrayPtr->fill(true);
    (*testBoolArrayPtr)[0] = false;
    (*testBoolArrayPtr)[5] = false;
    (*testBoolArrayPtr)[10] = false;
    (*testBoolArrayPtr)[20] = false;
    (*testBoolArrayPtr)[50] = false;
  }

  // Rectilinear Grid Geometry
  {
    RectGridGeom* rectGeomPtr = RectGridGeom::Create(dataStruct, k_RectGridGeo);
    std::vector<usize> dims = {10, 10, 5};
    rectGeomPtr->setDimensions(dims);

    Float32ArrayType* xBoundsArrayPtr = unit_test::CreateTestDataArray<float32>(dataStruct, k_XBounds, {14}, {1});
    (*xBoundsArrayPtr)[0] = 0;
    (*xBoundsArrayPtr)[1] = 1;
    (*xBoundsArrayPtr)[2] = 2;
    (*xBoundsArrayPtr)[3] = 3;
    (*xBoundsArrayPtr)[4] = 4;
    (*xBoundsArrayPtr)[5] = 5;
    (*xBoundsArrayPtr)[6] = 6;
    (*xBoundsArrayPtr)[7] = 7;
    (*xBoundsArrayPtr)[8] = 8;
    (*xBoundsArrayPtr)[9] = 9;
    (*xBoundsArrayPtr)[10] = 10;
    (*xBoundsArrayPtr)[11] = 11;
    (*xBoundsArrayPtr)[12] = 12;
    (*xBoundsArrayPtr)[13] = 14;
    Float32ArrayType* yBoundsArrayPtr = unit_test::CreateTestDataArray<float32>(dataStruct, k_YBounds, {14}, {1});
    (*yBoundsArrayPtr)[0] = 0;
    (*yBoundsArrayPtr)[1] = 2;
    (*yBoundsArrayPtr)[2] = 4;
    (*yBoundsArrayPtr)[3] = 6;
    (*yBoundsArrayPtr)[4] = 8;
    (*yBoundsArrayPtr)[5] = 10;
    (*yBoundsArrayPtr)[6] = 12;
    (*yBoundsArrayPtr)[7] = 14;
    (*yBoundsArrayPtr)[8] = 16;
    (*yBoundsArrayPtr)[9] = 18;
    (*yBoundsArrayPtr)[10] = 20;
    (*yBoundsArrayPtr)[11] = 22;
    (*yBoundsArrayPtr)[12] = 24;
    (*yBoundsArrayPtr)[13] = 26;
    Float32ArrayType* zBoundsArrayPtr = unit_test::CreateTestDataArray<float32>(dataStruct, k_ZBounds, {14}, {1});
    (*zBoundsArrayPtr)[0] = 0;
    (*zBoundsArrayPtr)[1] = 5;
    (*zBoundsArrayPtr)[2] = 10;
    (*zBoundsArrayPtr)[3] = 15;
    (*zBoundsArrayPtr)[4] = 20;
    (*zBoundsArrayPtr)[5] = 25;
    (*zBoundsArrayPtr)[6] = 30;
    (*zBoundsArrayPtr)[7] = 35;
    (*zBoundsArrayPtr)[8] = 40;
    (*zBoundsArrayPtr)[9] = 45;
    (*zBoundsArrayPtr)[10] = 50;
    (*zBoundsArrayPtr)[11] = 55;
    (*zBoundsArrayPtr)[12] = 60;
    (*zBoundsArrayPtr)[13] = 65;

    REQUIRE(rectGeomPtr->setBounds(xBoundsArrayPtr, yBoundsArrayPtr, zBoundsArrayPtr).valid());
    rectGeomPtr->findElementSizes(false);
  }

  // Vertex Geometry
  {
    VertexGeom* vertexGeomPtr = VertexGeom::Create(dataStruct, constants::k_VertexGeometry);

    std::vector<usize> vertTupleShape = {4};
    AttributeMatrix* vertAttMatrixPtr = AttributeMatrix::Create(dataStruct, constants::k_VertexDataGroupName, vertTupleShape, vertexGeomPtr->getId());
    vertexGeomPtr->setVertexAttributeMatrix(*vertAttMatrixPtr);
    StringArray* testStringArrayPtr =
        StringArray::CreateWithValues(dataStruct, k_StringArray, vertAttMatrixPtr->getShape(), {"stringone", "stringtwo", "stringthree", "stringfour"}, vertAttMatrixPtr->getId());
    Float32ArrayType* vertListArrayPtr = unit_test::CreateTestDataArray<float32>(dataStruct, constants::k_Float32DataSet, vertTupleShape, {3}, vertexGeomPtr->getId());
    (*vertListArrayPtr)[0] = 0;
    (*vertListArrayPtr)[1] = 0;
    (*vertListArrayPtr)[2] = 0;
    (*vertListArrayPtr)[3] = 1;
    (*vertListArrayPtr)[4] = 1;
    (*vertListArrayPtr)[5] = 1;
    (*vertListArrayPtr)[6] = 2;
    (*vertListArrayPtr)[7] = 2;
    (*vertListArrayPtr)[8] = 2;
    (*vertListArrayPtr)[9] = 3;
    (*vertListArrayPtr)[10] = 3;
    (*vertListArrayPtr)[11] = 3;
    vertexGeomPtr->setVertices(*vertListArrayPtr);

    vertexGeomPtr->findElementSizes(false);
  }

  // Edge Geometry
  {
    EdgeGeom* edgeGeomPtr = EdgeGeom::Create(dataStruct, k_EdgeGeo);

    auto* scalarPtr = ScalarData<int64>::Create(dataStruct, k_ScalarData, 60, edgeGeomPtr->getId());
    std::vector<usize> edgeTupleShape = {4};
    AttributeMatrix* edgeAttMatrixPtr = AttributeMatrix::Create(dataStruct, EdgeGeom::k_EdgeAttributeMatrixName, edgeTupleShape, edgeGeomPtr->getId());
    edgeGeomPtr->setEdgeAttributeMatrix(*edgeAttMatrixPtr);

    std::vector<usize> edgeVertTupleShape = {5};
    AttributeMatrix* edgeVertAttMatrixPtr = AttributeMatrix::Create(dataStruct, constants::k_VertexDataGroupName, edgeVertTupleShape, edgeGeomPtr->getId());
    edgeGeomPtr->setVertexAttributeMatrix(*edgeVertAttMatrixPtr);
    Float32ArrayType* edgeVertListArrayPtr = unit_test::CreateTestDataArray<float32>(dataStruct, constants::k_Float32DataSet, edgeVertTupleShape, {3}, edgeGeomPtr->getId());
    (*edgeVertListArrayPtr)[0] = 0;
    (*edgeVertListArrayPtr)[1] = 0;
    (*edgeVertListArrayPtr)[2] = 0;
    (*edgeVertListArrayPtr)[3] = 1;
    (*edgeVertListArrayPtr)[4] = 1;
    (*edgeVertListArrayPtr)[5] = 1;
    (*edgeVertListArrayPtr)[6] = 2;
    (*edgeVertListArrayPtr)[7] = 2;
    (*edgeVertListArrayPtr)[8] = 2;
    (*edgeVertListArrayPtr)[9] = 3;
    (*edgeVertListArrayPtr)[10] = 3;
    (*edgeVertListArrayPtr)[11] = 3;
    (*edgeVertListArrayPtr)[12] = 4;
    (*edgeVertListArrayPtr)[13] = 4;
    (*edgeVertListArrayPtr)[14] = 4;
    edgeGeomPtr->setVertices(*edgeVertListArrayPtr);
    IGeometry::SharedEdgeListType* edgesListArrayPtr = unit_test::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedEdges, edgeTupleShape, {2}, edgeGeomPtr->getId());
    (*edgesListArrayPtr)[0] = 0;
    (*edgesListArrayPtr)[1] = 1;
    (*edgesListArrayPtr)[2] = 1;
    (*edgesListArrayPtr)[3] = 2;
    (*edgesListArrayPtr)[4] = 2;
    (*edgesListArrayPtr)[5] = 3;
    (*edgesListArrayPtr)[6] = 3;
    (*edgesListArrayPtr)[7] = 4;
    edgeGeomPtr->setEdgeList(*edgesListArrayPtr);

    edgeGeomPtr->findElementSizes(false);
    createDynamicLists(*edgeGeomPtr);
    edgeGeomPtr->findElementCentroids(false);
  }

  // Triangle Geometry
  {
    TriangleGeom* triangleGeomPtr = TriangleGeom::Create(dataStruct, constants::k_TriangleGeometryName);

    std::vector<usize> faceTupleShape = {5};
    AttributeMatrix* faceAttMatrixPtr = AttributeMatrix::Create(dataStruct, TriangleGeom::k_FaceAttributeMatrixName, faceTupleShape, triangleGeomPtr->getId());
    triangleGeomPtr->setFaceAttributeMatrix(*faceAttMatrixPtr);

    std::vector<usize> faceVertTupleShape = {6};
    AttributeMatrix* faceVertAttMatrixPtr = AttributeMatrix::Create(dataStruct, constants::k_VertexDataGroupName, faceVertTupleShape, triangleGeomPtr->getId());
    triangleGeomPtr->setVertexAttributeMatrix(*faceVertAttMatrixPtr);
    Float32ArrayType* faceVertListArrayPtr = unit_test::CreateTestDataArray<float32>(dataStruct, constants::k_Float32DataSet, faceVertTupleShape, {3}, triangleGeomPtr->getId());
    (*faceVertListArrayPtr)[0] = 0;
    (*faceVertListArrayPtr)[1] = 0;
    (*faceVertListArrayPtr)[2] = 0;
    (*faceVertListArrayPtr)[3] = 0;
    (*faceVertListArrayPtr)[4] = -1;
    (*faceVertListArrayPtr)[5] = 1;
    (*faceVertListArrayPtr)[6] = 1;
    (*faceVertListArrayPtr)[7] = -0.5;
    (*faceVertListArrayPtr)[8] = 1;
    (*faceVertListArrayPtr)[9] = 0.5;
    (*faceVertListArrayPtr)[10] = 1;
    (*faceVertListArrayPtr)[11] = 0.5;
    (*faceVertListArrayPtr)[12] = -0.5;
    (*faceVertListArrayPtr)[13] = 1;
    (*faceVertListArrayPtr)[14] = 0;
    (*faceVertListArrayPtr)[15] = -1;
    (*faceVertListArrayPtr)[16] = -0.5;
    (*faceVertListArrayPtr)[17] = -1;
    triangleGeomPtr->setVertices(*faceVertListArrayPtr);
    IGeometry::SharedEdgeListType* facesListArrayPtr = unit_test::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedFaces, faceTupleShape, {3}, triangleGeomPtr->getId());
    (*facesListArrayPtr)[0] = 0;
    (*facesListArrayPtr)[1] = 1;
    (*facesListArrayPtr)[2] = 2;
    (*facesListArrayPtr)[3] = 0;
    (*facesListArrayPtr)[4] = 2;
    (*facesListArrayPtr)[5] = 3;
    (*facesListArrayPtr)[6] = 0;
    (*facesListArrayPtr)[7] = 3;
    (*facesListArrayPtr)[8] = 4;
    (*facesListArrayPtr)[9] = 0;
    (*facesListArrayPtr)[10] = 4;
    (*facesListArrayPtr)[11] = 5;
    (*facesListArrayPtr)[12] = 0;
    (*facesListArrayPtr)[13] = 5;
    (*facesListArrayPtr)[14] = 1;
    triangleGeomPtr->setFaceList(*facesListArrayPtr);

    triangleGeomPtr->findEdges(false);
    // TODO: triangleGeom->findElementSizes(false);
    createDynamicLists(*triangleGeomPtr);
    triangleGeomPtr->findElementCentroids(false);
    triangleGeomPtr->findUnsharedEdges(false);
  }

  // Quad Geometry
  {
    QuadGeom* quadGeomPtr = QuadGeom::Create(dataStruct, k_QuadGeo);

    std::vector<usize> faceTupleShape = {2};
    AttributeMatrix* faceAttMatrixPtr = AttributeMatrix::Create(dataStruct, QuadGeom::k_FaceAttributeMatrixName, faceTupleShape, quadGeomPtr->getId());
    quadGeomPtr->setFaceAttributeMatrix(*faceAttMatrixPtr);
    std::vector<usize> faceVertTupleShape = {6};
    AttributeMatrix* faceVertAttMatrixPtr = AttributeMatrix::Create(dataStruct, constants::k_VertexDataGroupName, faceVertTupleShape, quadGeomPtr->getId());
    quadGeomPtr->setVertexAttributeMatrix(*faceVertAttMatrixPtr);
    Float32ArrayType* faceVertListArrayPtr = unit_test::CreateTestDataArray<float32>(dataStruct, constants::k_Float32DataSet, faceVertTupleShape, {3}, quadGeomPtr->getId());
    (*faceVertListArrayPtr)[0] = -1;
    (*faceVertListArrayPtr)[1] = 1;
    (*faceVertListArrayPtr)[2] = -1;
    (*faceVertListArrayPtr)[3] = 0;
    (*faceVertListArrayPtr)[4] = 1;
    (*faceVertListArrayPtr)[5] = 0;
    (*faceVertListArrayPtr)[6] = 1;
    (*faceVertListArrayPtr)[7] = 1;
    (*faceVertListArrayPtr)[8] = 1;
    (*faceVertListArrayPtr)[9] = -1;
    (*faceVertListArrayPtr)[10] = -1;
    (*faceVertListArrayPtr)[11] = -1;
    (*faceVertListArrayPtr)[12] = 0;
    (*faceVertListArrayPtr)[13] = -1;
    (*faceVertListArrayPtr)[14] = 0;
    (*faceVertListArrayPtr)[15] = 1;
    (*faceVertListArrayPtr)[16] = -1;
    (*faceVertListArrayPtr)[17] = 1;
    quadGeomPtr->setVertices(*faceVertListArrayPtr);
    IGeometry::SharedEdgeListType* facesListArrayPtr = unit_test::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedFaces, faceTupleShape, {4}, quadGeomPtr->getId());
    (*facesListArrayPtr)[0] = 0;
    (*facesListArrayPtr)[1] = 3;
    (*facesListArrayPtr)[2] = 4;
    (*facesListArrayPtr)[3] = 1;
    (*facesListArrayPtr)[4] = 1;
    (*facesListArrayPtr)[5] = 4;
    (*facesListArrayPtr)[6] = 5;
    (*facesListArrayPtr)[7] = 2;
    quadGeomPtr->setFaceList(*facesListArrayPtr);

    quadGeomPtr->findEdges(false);
    // TODO: quadGeom->findElementSizes();
    createDynamicLists(*quadGeomPtr);
    quadGeomPtr->findElementCentroids(false);
    quadGeomPtr->findUnsharedEdges(false);
  }

  // Tetrahedral Geometry
  {
    TetrahedralGeom* tetGeomPtr = TetrahedralGeom::Create(dataStruct, k_TetGeo);

    std::vector<usize> cellTupleShape = {2};
    AttributeMatrix* polyAttMatrixPtr = AttributeMatrix::Create(dataStruct, INodeGeometry3D::k_PolyhedronDataName, cellTupleShape, tetGeomPtr->getId());
    tetGeomPtr->setPolyhedraAttributeMatrix(*polyAttMatrixPtr);
    std::vector<usize> vertTupleShape = {5};
    AttributeMatrix* cellVertAttMatrixPtr = AttributeMatrix::Create(dataStruct, constants::k_VertexDataGroupName, vertTupleShape, tetGeomPtr->getId());
    tetGeomPtr->setVertexAttributeMatrix(*cellVertAttMatrixPtr);
    Float32ArrayType* vertListArrayPtr = unit_test::CreateTestDataArray<float32>(dataStruct, constants::k_Float32DataSet, vertTupleShape, {3}, tetGeomPtr->getId());
    (*vertListArrayPtr)[0] = -1;
    (*vertListArrayPtr)[1] = 0.5;
    (*vertListArrayPtr)[2] = 0;
    (*vertListArrayPtr)[3] = 0;
    (*vertListArrayPtr)[4] = 0;
    (*vertListArrayPtr)[5] = 0;
    (*vertListArrayPtr)[6] = 0;
    (*vertListArrayPtr)[7] = 1;
    (*vertListArrayPtr)[8] = 0;
    (*vertListArrayPtr)[9] = -0.5;
    (*vertListArrayPtr)[10] = 0.5;
    (*vertListArrayPtr)[11] = 1;
    (*vertListArrayPtr)[12] = 1;
    (*vertListArrayPtr)[13] = 0.5;
    (*vertListArrayPtr)[14] = 0;
    tetGeomPtr->setVertices(*vertListArrayPtr);
    IGeometry::SharedEdgeListType* polyListArrayPtr = unit_test::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedPolyhedrons, cellTupleShape, {4}, tetGeomPtr->getId());
    (*polyListArrayPtr)[0] = 0;
    (*polyListArrayPtr)[1] = 1;
    (*polyListArrayPtr)[2] = 2;
    (*polyListArrayPtr)[3] = 3;
    (*polyListArrayPtr)[4] = 1;
    (*polyListArrayPtr)[5] = 4;
    (*polyListArrayPtr)[6] = 2;
    (*polyListArrayPtr)[7] = 3;
    tetGeomPtr->setPolyhedraList(*polyListArrayPtr);

    tetGeomPtr->findEdges(false);
    tetGeomPtr->findFaces(false);
    tetGeomPtr->findElementSizes(false);
    createDynamicLists(*tetGeomPtr);
    tetGeomPtr->findElementCentroids(false);
    tetGeomPtr->findUnsharedEdges(false);
    tetGeomPtr->findUnsharedFaces(false);
  }

  // Hexahedral Geometry
  {
    HexahedralGeom* hexGeomPtr = HexahedralGeom::Create(dataStruct, k_HexGeo);

    std::vector<usize> cellTupleShape = {2};
    AttributeMatrix* polyAttMatrixPtr = AttributeMatrix::Create(dataStruct, INodeGeometry3D::k_PolyhedronDataName, cellTupleShape, hexGeomPtr->getId());
    hexGeomPtr->setPolyhedraAttributeMatrix(*polyAttMatrixPtr);
    std::vector<usize> vertTupleShape = {12};
    AttributeMatrix* cellVertAttMatrixPtr = AttributeMatrix::Create(dataStruct, constants::k_VertexDataGroupName, vertTupleShape, hexGeomPtr->getId());
    hexGeomPtr->setVertexAttributeMatrix(*cellVertAttMatrixPtr);
    Float32ArrayType* vertListArrayPtr = unit_test::CreateTestDataArray<float32>(dataStruct, constants::k_Float32DataSet, vertTupleShape, {3}, hexGeomPtr->getId());
    (*vertListArrayPtr)[0] = -1;
    (*vertListArrayPtr)[1] = 1;
    (*vertListArrayPtr)[2] = 1;
    (*vertListArrayPtr)[3] = -1;
    (*vertListArrayPtr)[4] = -1;
    (*vertListArrayPtr)[5] = 1;
    (*vertListArrayPtr)[6] = 0;
    (*vertListArrayPtr)[7] = -1;
    (*vertListArrayPtr)[8] = 1;
    (*vertListArrayPtr)[9] = 0;
    (*vertListArrayPtr)[10] = 1;
    (*vertListArrayPtr)[11] = 1;
    (*vertListArrayPtr)[12] = 0;
    (*vertListArrayPtr)[13] = 1;
    (*vertListArrayPtr)[14] = -1;
    (*vertListArrayPtr)[15] = -1;
    (*vertListArrayPtr)[16] = 1;
    (*vertListArrayPtr)[17] = -1;
    (*vertListArrayPtr)[18] = -1;
    (*vertListArrayPtr)[19] = -1;
    (*vertListArrayPtr)[20] = -1;
    (*vertListArrayPtr)[21] = 0;
    (*vertListArrayPtr)[22] = -1;
    (*vertListArrayPtr)[23] = -1;
    (*vertListArrayPtr)[24] = 1;
    (*vertListArrayPtr)[25] = -1;
    (*vertListArrayPtr)[26] = -1;
    (*vertListArrayPtr)[27] = 1;
    (*vertListArrayPtr)[28] = 1;
    (*vertListArrayPtr)[29] = -1;
    (*vertListArrayPtr)[30] = 1;
    (*vertListArrayPtr)[31] = 1;
    (*vertListArrayPtr)[32] = 1;
    (*vertListArrayPtr)[33] = 1;
    (*vertListArrayPtr)[34] = -1;
    (*vertListArrayPtr)[35] = 1;
    hexGeomPtr->setVertices(*vertListArrayPtr);
    IGeometry::SharedEdgeListType* polyListArrayPtr = unit_test::CreateTestDataArray<IGeometry::MeshIndexType>(dataStruct, k_SharedPolyhedrons, cellTupleShape, {8}, hexGeomPtr->getId());
    (*polyListArrayPtr)[0] = 6;
    (*polyListArrayPtr)[1] = 7;
    (*polyListArrayPtr)[2] = 4;
    (*polyListArrayPtr)[3] = 5;
    (*polyListArrayPtr)[4] = 1;
    (*polyListArrayPtr)[5] = 2;
    (*polyListArrayPtr)[6] = 3;
    (*polyListArrayPtr)[7] = 0;
    (*polyListArrayPtr)[8] = 7;
    (*polyListArrayPtr)[9] = 8;
    (*polyListArrayPtr)[10] = 9;
    (*polyListArrayPtr)[11] = 4;
    (*polyListArrayPtr)[12] = 2;
    (*polyListArrayPtr)[13] = 11;
    (*polyListArrayPtr)[14] = 10;
    (*polyListArrayPtr)[15] = 3;
    hexGeomPtr->setPolyhedraList(*polyListArrayPtr);

    hexGeomPtr->findEdges(false);
    hexGeomPtr->findFaces(false);
    hexGeomPtr->findElementSizes(false);
    createDynamicLists(*hexGeomPtr);
    hexGeomPtr->findElementCentroids(false);
    hexGeomPtr->findUnsharedEdges(false);
    hexGeomPtr->findUnsharedFaces(false);
  }

  return dataStruct;
}

/**
 * @brief Creates serializable data for disk round-trip tests.
 * @return Test DataStructure without derived DynamicListArray objects.
 *
 * The DREAM3D writer does not serialize the derived lists. It reports
 * "-5 : Could not find IO factory for datatype: DynamicListArray".
 */
DataStructure CreateRoundTripDataStructure()
{
  return CreateTestDataStructure(DynamicListMode::Exclude);
}

/**
 * @brief Finds the child node with the given name in a JSON "children" or "objects" array.
 * @param nodes JSON node array to search.
 * @param name Object name to find.
 * @return The matching node.
 * @throws std::runtime_error If no object has the requested name.
 */
const nlohmann::json& FindNode(const nlohmann::json& nodes, std::string_view name)
{
  for(const auto& node : nodes)
  {
    if(node.at("name").get<std::string>() == name)
    {
      return node;
    }
  }
  throw std::runtime_error(fmt::format("No JSON hierarchy node named '{}' was found.", name));
}
} // namespace

TEST_CASE("DataStructure::exportHierarchyAsJson::Fields")
{
  unit_test::LoadPlugins();

  const DataStructure dataStructure = unit_test::CreateComplexMultiLevelDataGraph();
  const nlohmann::json json = dataStructure.exportHierarchyAsJson();

  REQUIRE(json.at("schema_version").get<int>() == 1);
  REQUIRE(json.contains("objects"));
  REQUIRE(json.at("objects").is_array());
  REQUIRE(json.at("objects").size() == dataStructure.getTopLevelData().size());

  // Top-level DataGroup "A" has two children: "H" and "C".
  const auto& groupA = FindNode(json.at("objects"), constants::k_GroupAName.view());
  REQUIRE(groupA.at("path").get<std::string>() == constants::k_GroupAName.view());
  REQUIRE(groupA.at("type").get<std::string>() == DataGroup::k_TypeName.view());
  const auto groupAId = dataStructure.getId(DataPath({std::string(constants::k_GroupAName.view())}));
  REQUIRE(groupAId == groupA.at("id").get<uint64>());
  REQUIRE(groupA.at("children").size() == 2);
  REQUIRE_FALSE(groupA.contains("tuple_shape"));
  REQUIRE_FALSE(groupA.contains("data_type"));

  // A/H/N is an int8 DataArray with tuple shape {1} and component shape {1}.
  const auto& groupH = FindNode(groupA.at("children"), constants::k_GroupHName.view());
  const auto& arrayN = FindNode(groupH.at("children"), constants::k_ArrayNName.view());
  REQUIRE(arrayN.at("path").get<std::string>() == "A/H/N");
  REQUIRE(arrayN.at("type").get<std::string>() == "DataArray<int8>");
  REQUIRE(arrayN.at("data_type").get<std::string>() == "int8");
  REQUIRE(arrayN.at("tuple_shape") == nlohmann::json::array({1}));
  REQUIRE(arrayN.at("component_shape") == nlohmann::json::array({1}));
  REQUIRE(arrayN.at("num_tuples").get<uint64>() == 1);
  REQUIRE(arrayN.at("num_components").get<uint64>() == 1);
  REQUIRE(arrayN.at("store_type").get<std::string>() == "InMemory");
  REQUIRE(arrayN.at("children").is_array());
  REQUIRE(arrayN.at("children").empty());
}

TEST_CASE("DataStructure::exportHierarchyAsJson::AlphabeticOrder")
{
  DataStructure dataStructure;
  auto* zGroupPtr = DataGroup::Create(dataStructure, "Zulu");
  auto* aGroupPtr = DataGroup::Create(dataStructure, "Alpha");
  REQUIRE(zGroupPtr != nullptr);
  REQUIRE(aGroupPtr != nullptr);
  REQUIRE(DataGroup::Create(dataStructure, "Zulu Child", aGroupPtr->getId()) != nullptr);
  REQUIRE(DataGroup::Create(dataStructure, "Alpha Child", aGroupPtr->getId()) != nullptr);

  const nlohmann::json json = dataStructure.exportHierarchyAsJson();
  REQUIRE(json.at("objects").at(0).at("name") == "Alpha");
  REQUIRE(json.at("objects").at(1).at("name") == "Zulu");

  const auto& alphaChildren = json.at("objects").at(0).at("children");
  REQUIRE(alphaChildren.at(0).at("name") == "Alpha Child");
  REQUIRE(alphaChildren.at(1).at("name") == "Zulu Child");
}

TEST_CASE("DataStructure::HierarchyExportsHandleDeepTrees")
{
  constexpr usize k_Depth = 512;
  DataStructure dataStructure;
  std::optional<DataObject::IdType> parentId;
  for(usize index = 0; index < k_Depth; ++index)
  {
    DataGroup* groupPtr = parentId.has_value() ? DataGroup::Create(dataStructure, fmt::format("Level {}", index), *parentId) : DataGroup::Create(dataStructure, fmt::format("Level {}", index));
    REQUIRE(groupPtr != nullptr);
    parentId = groupPtr->getId();
  }

  const nlohmann::json json = dataStructure.exportHierarchyAsJson();
  const nlohmann::json* nodePtr = &json.at("objects").at(0);
  for(usize index = 0; index < k_Depth; ++index)
  {
    REQUIRE(nodePtr->at("name") == fmt::format("Level {}", index));
    if(index + 1 < k_Depth)
    {
      nodePtr = &nodePtr->at("children").at(0);
    }
  }

  std::ostringstream textOutput;
  dataStructure.exportHierarchyAsText(textOutput);
  REQUIRE(static_cast<usize>(std::ranges::count(textOutput.str(), '\n')) == k_Depth);

  std::ostringstream dotOutput;
  dataStructure.exportHierarchyAsGraphViz(dotOutput);
  REQUIRE(static_cast<usize>(std::ranges::count(dotOutput.str(), '\n')) >= k_Depth);
}

TEST_CASE("DataStructure::exportHierarchyAsJson::AttributeMatrixAndNeighborList")
{
  unit_test::LoadPlugins();

  const DataStructure dataStructure = CreateTestDataStructure();
  const nlohmann::json json = dataStructure.exportHierarchyAsJson();

  const auto& imageGeom = FindNode(json.at("objects"), constants::k_ImageGeometry.view());

  // Image Geometry/CellData is an AttributeMatrix with shape {80, 60, 40} (X fastest).
  const auto& cellData = FindNode(imageGeom.at("children"), constants::k_CellData.view());
  REQUIRE(cellData.at("type").get<std::string>() == AttributeMatrix::k_TypeName.view());
  REQUIRE(cellData.at("tuple_shape") == nlohmann::json::array({80, 60, 40}));
  REQUIRE_FALSE(cellData.contains("data_type"));

  // Image Geometry/CellData/Conditional [bool] is a bool DataArray.
  const auto& boolArray = FindNode(cellData.at("children"), constants::k_ConditionalArray.view());
  REQUIRE(boolArray.at("data_type").get<std::string>() == "boolean");
  REQUIRE(boolArray.at("tuple_shape") == nlohmann::json::array({80, 60, 40}));
  REQUIRE(boolArray.at("num_tuples").get<uint64>() == uint64{80} * 60 * 40);

  // Image Geometry/ONE/int16 DataSet is a NeighborList<int16> with 3 tuples.
  const auto& levelOne = FindNode(imageGeom.at("children"), constants::k_LevelOne.view());
  const auto& neighborList = FindNode(levelOne.at("children"), constants::k_Int16DataSet.view());
  REQUIRE(neighborList.at("data_type").get<std::string>() == "int16");
  REQUIRE(neighborList.at("num_tuples").get<uint64>() == 3);
  REQUIRE_FALSE(neighborList.contains("component_shape"));

  // Vertex Geometry/Vertex Data/String Array is serialized explicitly as a string array.
  const auto& vertexGeom = FindNode(json.at("objects"), constants::k_VertexGeometry.view());
  const auto& vertexData = FindNode(vertexGeom.at("children"), constants::k_VertexDataGroupName.view());
  const auto& stringArray = FindNode(vertexData.at("children"), k_StringArray.view());
  REQUIRE(stringArray.at("type").get<std::string>() == StringArray::k_TypeName.view());
  REQUIRE(stringArray.at("data_type").get<std::string>() == "string");
  REQUIRE(stringArray.at("num_tuples").get<uint64>() == 4);
  REQUIRE_FALSE(stringArray.contains("component_shape"));
}

TEST_CASE("DataStructure::exportHierarchyAsJson::Geometry")
{
  unit_test::LoadPlugins();

  const DataStructure dataStructure = CreateTestDataStructure();
  const nlohmann::json json = dataStructure.exportHierarchyAsJson();

  const auto& imageGeom = FindNode(json.at("objects"), constants::k_ImageGeometry.view());
  REQUIRE(imageGeom.at("type").get<std::string>() == ImageGeom::k_TypeName.view());
  REQUIRE(imageGeom.contains("geometry"));
  const auto& geom = imageGeom.at("geometry");

  REQUIRE(geom.at("geometry_type").get<std::string>() == "Image");
  REQUIRE(geom.at("unit_dimensionality").get<uint32>() == 3);
  REQUIRE(geom.at("length_units").get<std::string>() == "Meter");
  REQUIRE(geom.at("num_cells").get<uint64>() == 192000);
  REQUIRE(geom.at("dimensions") == nlohmann::json::array({40, 60, 80}));
  REQUIRE(geom.at("origin").at(0).get<float32>() == Approx(0.0f));
  REQUIRE(geom.at("origin").at(1).get<float32>() == Approx(20.0f));
  REQUIRE(geom.at("origin").at(2).get<float32>() == Approx(66.0f));
  REQUIRE(geom.at("spacing").at(0).get<float32>() == Approx(0.25f));
  REQUIRE(geom.at("spacing").at(1).get<float32>() == Approx(0.55f));
  REQUIRE(geom.at("spacing").at(2).get<float32>() == Approx(1.86f));
  REQUIRE(geom.at("cell_data_path").get<std::string>() == "Image Geometry/CellData");

  // Node geometry keys must not appear on a grid geometry.
  REQUIRE_FALSE(geom.contains("num_vertices"));
  REQUIRE_FALSE(geom.contains("vertex_data_path"));

  // RectGridGeom has dimensions and no spacing. Its origin is present because the fixture sets bounds arrays.
  const auto& rectGeom = FindNode(json.at("objects"), k_RectGridGeo.view());
  const auto& rectGeomBlock = rectGeom.at("geometry");
  REQUIRE(rectGeomBlock.at("geometry_type").get<std::string>() == "RectGrid");
  REQUIRE(rectGeomBlock.at("dimensions") == nlohmann::json::array({10, 10, 5}));
  REQUIRE(rectGeomBlock.contains("origin"));
  REQUIRE(rectGeomBlock.at("origin").at(0).get<float32>() == Approx(0.0f));
  REQUIRE(rectGeomBlock.at("origin").at(1).get<float32>() == Approx(0.0f));
  REQUIRE(rectGeomBlock.at("origin").at(2).get<float32>() == Approx(0.0f));
  REQUIRE_FALSE(rectGeomBlock.contains("spacing"));

  // A DataGroup must not carry a geometry block.
  const auto& levelOne = FindNode(imageGeom.at("children"), constants::k_LevelOne.view());
  REQUIRE_FALSE(levelOne.contains("geometry"));
}

TEST_CASE("DataStructure::exportHierarchyAsJson::RectGridPreflightBounds")
{
  unit_test::LoadPlugins();

  DataStructure dataStructure;
  auto* rectGridGeomPtr = RectGridGeom::Create(dataStructure, "Rect Grid");
  REQUIRE(rectGridGeomPtr != nullptr);
  rectGridGeomPtr->setDimensions({2, 2, 2});

  const auto createBounds = [&dataStructure](const std::string& name) {
    auto result = Float32ArrayType::CreatePlanned(dataStructure, name, {3}, {1}, "");
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    return result.value();
  };
  auto* xBoundsPtr = createBounds("X Bounds");
  auto* yBoundsPtr = createBounds("Y Bounds");
  auto* zBoundsPtr = createBounds("Z Bounds");
  REQUIRE(xBoundsPtr != nullptr);
  REQUIRE(yBoundsPtr != nullptr);
  REQUIRE(zBoundsPtr != nullptr);
  REQUIRE(rectGridGeomPtr->setBounds(xBoundsPtr, yBoundsPtr, zBoundsPtr).valid());

  nlohmann::json json;
  REQUIRE_NOTHROW(json = dataStructure.exportHierarchyAsJson());
  const auto& node = FindNode(json.at("objects"), "Rect Grid");
  const auto& geom = node.at("geometry");

  REQUIRE(geom.at("dimensions") == nlohmann::json::array({2, 2, 2}));
  REQUIRE_FALSE(geom.contains("origin"));
}

TEST_CASE("DataStructure::exportHierarchyAsJson::RectGridPlannedOutOfCoreBounds")
{
  DataStructure dataStructure;
  auto* rectGridGeomPtr = RectGridGeom::Create(dataStructure, "Rect Grid");
  REQUIRE(rectGridGeomPtr != nullptr);
  rectGridGeomPtr->setDimensions({2, 2, 2});

  class PlaceholderIOManager : public IDataIOManager
  {
  public:
    PlaceholderIOManager()
    {
      addDataStoreCreationFnc(formatName(), [](DataType, const ShapeType&, const ShapeType&, const std::optional<ShapeType>&, DataStoreInitializationMode) -> std::unique_ptr<IDataStore> {
        throw std::runtime_error("Hierarchy export must not allocate a value store");
      });
    }

    std::string formatName() const override
    {
      return "hierarchy-placeholder-test";
    }
  };
  auto manager = std::make_shared<PlaceholderIOManager>();
  auto registrationResult = Application::GetOrCreateInstance()->getIOCollection().addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(registrationResult);

  const auto createBounds = [&dataStructure, &manager](const std::string& name) {
    auto result = Float32ArrayType::CreatePlanned(dataStructure, name, {3}, {1}, manager->formatName());
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value()->getIDataStoreRef().getPlannedStoreType() == IDataStore::StoreType::OutOfCore);
    return result.value();
  };
  auto* xBoundsPtr = createBounds("X Bounds");
  auto* yBoundsPtr = createBounds("Y Bounds");
  auto* zBoundsPtr = createBounds("Z Bounds");
  REQUIRE(xBoundsPtr != nullptr);
  REQUIRE(yBoundsPtr != nullptr);
  REQUIRE(zBoundsPtr != nullptr);
  REQUIRE(rectGridGeomPtr->setBounds(xBoundsPtr, yBoundsPtr, zBoundsPtr).valid());

  const nlohmann::json json = dataStructure.exportHierarchyAsJson();
  const auto& rectGridNode = FindNode(json.at("objects"), "Rect Grid");
  REQUIRE_FALSE(rectGridNode.at("geometry").contains("origin"));
  REQUIRE(FindNode(json.at("objects"), "X Bounds").at("store_type").get<std::string>() == "Empty");
}

TEST_CASE("DataStructure::exportHierarchyAsJson::NodeGeometry")
{
  unit_test::LoadPlugins();

  DataStructure dataStructure;
  auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(triangleGeomPtr != nullptr);

  // The geometry has 4 vertices and 2 faces.
  auto* verticesPtr = unit_test::CreateTestDataArray<float32>(dataStructure, "SharedVertexList", {4}, {3}, triangleGeomPtr->getId());
  auto* facesPtr = unit_test::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, "SharedTriList", {2}, {3}, triangleGeomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  triangleGeomPtr->setVertices(*verticesPtr);
  triangleGeomPtr->setFaceList(*facesPtr);

  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", std::vector<usize>{4}, triangleGeomPtr->getId());
  REQUIRE(vertexDataPtr != nullptr);
  triangleGeomPtr->setVertexAttributeMatrix(*vertexDataPtr);
  // Do not assign a face attribute matrix. The exporter must omit face_data_path.

  const nlohmann::json json = dataStructure.exportHierarchyAsJson();
  const auto& node = FindNode(json.at("objects"), "Triangles");
  const auto& geom = node.at("geometry");

  REQUIRE(geom.at("geometry_type").get<std::string>() == "Triangle");
  REQUIRE(geom.at("num_vertices").get<uint64>() == 4);
  REQUIRE(geom.at("num_faces").get<uint64>() == 2);
  REQUIRE(geom.at("num_cells").get<uint64>() == 2);
  REQUIRE(geom.at("vertex_data_path").get<std::string>() == "Triangles/Vertex Data");
  REQUIRE_FALSE(geom.contains("face_data_path"));
  REQUIRE_FALSE(geom.contains("dimensions"));
  REQUIRE_FALSE(geom.contains("num_polyhedra"));
  // A triangle geometry reports zero edges without an edge list. The edge-data path remains optional.
  REQUIRE(geom.at("num_edges").get<uint64>() == 0);
  REQUIRE_FALSE(geom.contains("edge_data_path"));
}

TEST_CASE("DataStructure::exportHierarchyAsJson::NodeGeometryPaths")
{
  unit_test::LoadPlugins();

  const DataStructure dataStructure = CreateTestDataStructure();
  const nlohmann::json json = dataStructure.exportHierarchyAsJson();

  const auto& edgeGeometry = FindNode(json.at("objects"), k_EdgeGeo.view()).at("geometry");
  REQUIRE(edgeGeometry.at("geometry_type").get<std::string>() == "Edge");
  REQUIRE(edgeGeometry.at("num_vertices").get<uint64>() == 5);
  REQUIRE(edgeGeometry.at("num_edges").get<uint64>() == 4);
  REQUIRE(edgeGeometry.at("edge_data_path").get<std::string>() == "Edge Geometry/Edge Data");

  const auto& quadGeometry = FindNode(json.at("objects"), k_QuadGeo.view()).at("geometry");
  REQUIRE(quadGeometry.at("geometry_type").get<std::string>() == "Quad");
  REQUIRE(quadGeometry.at("num_faces").get<uint64>() == 2);
  REQUIRE(quadGeometry.at("face_data_path").get<std::string>() == "Quad Geometry/Face Data");

  const auto& tetGeometry = FindNode(json.at("objects"), k_TetGeo.view()).at("geometry");
  REQUIRE(tetGeometry.at("geometry_type").get<std::string>() == "Tetrahedral");
  REQUIRE(tetGeometry.at("num_polyhedra").get<uint64>() == 2);
  REQUIRE(tetGeometry.at("polyhedron_data_path").get<std::string>() == "Tet Geometry/Polyhedron Data");

  const auto& hexGeometry = FindNode(json.at("objects"), k_HexGeo.view()).at("geometry");
  REQUIRE(hexGeometry.at("geometry_type").get<std::string>() == "Hexahedral");
  REQUIRE(hexGeometry.at("num_vertices").get<uint64>() == 12);
  REQUIRE(hexGeometry.at("num_edges").get<uint64>() == 20);
  REQUIRE(hexGeometry.at("num_faces").get<uint64>() == 11);
  REQUIRE(hexGeometry.at("num_polyhedra").get<uint64>() == 2);
  REQUIRE(hexGeometry.at("polyhedron_data_path").get<std::string>() == "Hex Geometry/Polyhedron Data");
}

TEST_CASE("DataStructure::exportHierarchyAsJson::PlannedScalarStore")
{
  DataStructure dataStructure;
  auto arrayResult = Int32ArrayType::CreatePlanned(dataStructure, "Array", {}, {}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(arrayResult);
  auto* dataArrayPtr = arrayResult.value();
  REQUIRE(dataArrayPtr != nullptr);
  REQUIRE(dataArrayPtr->getIDataStore() != nullptr);

  const nlohmann::json json = dataStructure.exportHierarchyAsJson();
  const auto& arrayNode = FindNode(json.at("objects"), "Array");
  REQUIRE(arrayNode.at("store_type").get<std::string>() == "Empty");
  REQUIRE(arrayNode.at("tuple_shape") == nlohmann::json::array());
  REQUIRE(arrayNode.at("component_shape") == nlohmann::json::array());
  REQUIRE(arrayNode.at("num_tuples").get<uint64>() == 1);
  REQUIRE(arrayNode.at("num_components").get<uint64>() == 1);
}

namespace
{
/**
 * @brief Normalizes volatile fields and ordering in a hierarchy document.
 * @param root Root hierarchy object to normalize.
 *
 * The reader assigns identifiers and creates Empty stores. Preflight mode does not read RectGrid bounds values.
 */
void StripVolatileKeysFromDocument(nlohmann::json& root)
{
  auto& objects = root.at("objects");
  const auto pathLess = [](const nlohmann::json& lhs, const nlohmann::json& rhs) { return lhs.at("path").get<std::string>() < rhs.at("path").get<std::string>(); };
  // nlohmann JSON iterators do not satisfy the std::ranges::sort constraints.
  std::sort(objects.begin(), objects.end(), pathLess); // NOLINT(modernize-use-ranges)

  std::vector<nlohmann::json*> pendingNodes;
  pendingNodes.reserve(objects.size());
  for(auto& object : objects)
  {
    pendingNodes.push_back(&object);
  }

  while(!pendingNodes.empty())
  {
    nlohmann::json* nodePtr = pendingNodes.back();
    pendingNodes.pop_back();
    nodePtr->erase("id");
    nodePtr->erase("store_type");
    if(nodePtr->contains("geometry") && nodePtr->at("geometry").is_object())
    {
      auto& geometry = nodePtr->at("geometry");
      if(geometry.contains("geometry_type") && geometry.at("geometry_type") == "RectGrid")
      {
        geometry.erase("origin");
      }
    }

    auto& children = nodePtr->at("children");
    std::sort(children.begin(), children.end(), pathLess); // NOLINT(modernize-use-ranges)
    for(auto& child : children)
    {
      pendingNodes.push_back(&child);
    }
  }
}
} // namespace

TEST_CASE("DataStructure::exportHierarchyAsJson::PreflightRoundTrip")
{
  unit_test::LoadPlugins();

  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "exportHierarchyAsJson_roundtrip.dream3d";
  if(fs::exists(filePath))
  {
    fs::remove(filePath);
  }

  const DataStructure written = CreateRoundTripDataStructure();
  nlohmann::json writtenJson = written.exportHierarchyAsJson();

  Result<> writeResult = dream3d::WriteFile(filePath, written);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  REQUIRE(fs::exists(filePath));

  const Result<DataStructure> readResult = dream3d::ImportDataStructureFromFile(filePath, true);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  nlohmann::json readJson = readResult.value().exportHierarchyAsJson();

  // Preflight mode must create every array with an empty store and read no values.
  const auto& imageGeom = FindNode(readJson.at("objects"), constants::k_ImageGeometry.view());
  const auto& cellData = FindNode(imageGeom.at("children"), constants::k_CellData.view());
  const auto& boolArray = FindNode(cellData.at("children"), constants::k_ConditionalArray.view());
  REQUIRE(boolArray.at("store_type").get<std::string>() == "Empty");
  REQUIRE(boolArray.at("num_tuples").get<uint64>() == uint64{80} * 60 * 40);

  const auto& writtenRect = FindNode(writtenJson.at("objects"), k_RectGridGeo.view());
  const auto& readRect = FindNode(readJson.at("objects"), k_RectGridGeo.view());
  // RectGrid origin comes from bounds values, which preflight mode does not read.
  REQUIRE(writtenRect.at("geometry").contains("origin"));
  REQUIRE_FALSE(readRect.at("geometry").contains("origin"));

  StripVolatileKeysFromDocument(writtenJson);
  StripVolatileKeysFromDocument(readJson);
  INFO("Written JSON:\n" << writtenJson.dump(2));
  INFO("Read JSON:\n" << readJson.dump(2));
  REQUIRE(writtenJson == readJson);
}

/**
 * @brief Test creation and removal of items in a tree-style structure. No node has more than one parent.
 */
TEST_CASE("DataGroupTree")
{
  // Create structuure
  DataStructure dataStr;
  auto* groupPtr = DataGroup::Create(dataStr, "Foo");
  auto* childPtr = DataGroup::Create(dataStr, "bar", groupPtr->getId());
  auto* grandchildPtr = DataGroup::Create(dataStr, "bazz", childPtr->getId());

  auto groupId = groupPtr->getId();
  auto childId = childPtr->getId();
  auto grandchildId = grandchildPtr->getId();

  SECTION("find data")
  {
    REQUIRE(dataStr.getData(groupId) != nullptr);
    REQUIRE(dataStr.getData(childId) != nullptr);
    REQUIRE(dataStr.getData(grandchildId) != nullptr);
  }
  SECTION("remove mid-level group")
  {
    groupPtr->remove(childPtr->getName());
    REQUIRE(dataStr.getData(childId) == nullptr);
    REQUIRE(dataStr.getData(grandchildId) == nullptr);
  }
  SECTION("remove top-level group")
  {
    dataStr.removeData(groupPtr->getId());
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
  auto* groupPtr = DataGroup::Create(dataStr, "Foo");
  auto* child1Ptr = DataGroup::Create(dataStr, "Bar1", groupPtr->getId());
  auto* child2Ptr = DataGroup::Create(dataStr, "Bar2", groupPtr->getId());
  auto* grandchildPtr = DataGroup::Create(dataStr, "Bazz", child1Ptr->getId());

  // Get IDs
  auto groupId = groupPtr->getId();
  auto child1Id = child1Ptr->getId();
  auto child2Id = child2Ptr->getId();
  auto grandchildId = grandchildPtr->getId();

  // Multi-parent connections
  dataStr.setAdditionalParent(grandchildId, child2Id);

  // Test findAllObjectsOfType
  {
    auto findAll = groupPtr->findAllChildrenOfType<BaseGroup>();
    REQUIRE(findAll.size() == 2);

    auto findAllRecur = groupPtr->findAllChildrenOfType<BaseGroup>(true);
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
    groupPtr->remove(child1Ptr->getName());
    REQUIRE(dataStr.getData(child1Id) == nullptr);
    REQUIRE(dataStr.getData(grandchildId) != nullptr);
  }
  SECTION("remove top-level group")
  {
    dataStr.removeData(groupPtr->getId());
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
  auto* groupPtr = DataGroup::Create(dataStr, "Foo");
  auto* child1Ptr = DataGroup::Create(dataStr, "Bar1", groupPtr->getId());
  auto* child2Ptr = DataGroup::Create(dataStr, "Bar2", groupPtr->getId());
  auto* grandchildPtr = DataGroup::Create(dataStr, "Bazz", child1Ptr->getId());

  auto child2Id = child2Ptr->getId();
  auto grandchildId = grandchildPtr->getId();
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
  auto* gc2Ptr = dataStr.getData(gcPath2);
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
  auto* groupPtr = DataGroup::Create(dataStr, "Foo");
  auto* child1Ptr = DataGroup::Create(dataStr, "Bar1", groupPtr->getId());
  auto* child2Ptr = DataGroup::Create(dataStr, "Bar2", groupPtr->getId());
  auto* grandchildPtr = DataGroup::Create(dataStr, "Bazz", child1Ptr->getId());

  auto child2Id = child2Ptr->getId();
  auto grandchildId = grandchildPtr->getId();

  REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));

  // DataPath
  const DataPath grandPath({"Foo", "Bar1", "Bazz"});
  const LinkedPath linkedPath = dataStr.getLinkedPath(grandPath);

  REQUIRE(linkedPath.isValid());
  REQUIRE(grandchildPtr == linkedPath.getData());

  REQUIRE(child1Ptr->rename("Bar1.3"));
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
  auto* groupPtr = DataGroup::Create(dataStr, "Foo");
  auto* child1Ptr = DataGroup::Create(dataStr, "Bar1", groupPtr->getId());
  auto* child2Ptr = DataGroup::Create(dataStr, "Bar2", groupPtr->getId());
  auto* grandchildPtr = DataGroup::Create(dataStr, "Bazz", child1Ptr->getId());

  auto groupId = groupPtr->getId();
  auto child2Id = child2Ptr->getId();
  auto grandchildId = grandchildPtr->getId();

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
  auto* groupPtr = DataGroup::Create(dataStr, "Foo");
  auto* child1Ptr = DataGroup::Create(dataStr, "Bar1", groupPtr->getId());
  auto* child2Ptr = DataGroup::Create(dataStr, "Bar2", groupPtr->getId());
  auto* grandchildPtr = DataGroup::Create(dataStr, "Bazz", child1Ptr->getId());

  auto groupId = groupPtr->getId();
  auto child1Id = child1Ptr->getId();
  auto child2Id = child2Ptr->getId();
  auto grandchildId = grandchildPtr->getId();

  REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));

  // Copy DataStructure
  DataStructure dataStrCopy(dataStr);

  REQUIRE(dataStrCopy.getData(groupId));
  REQUIRE(dataStrCopy.getData(child1Id));
  REQUIRE(dataStrCopy.getData(child2Id));
  REQUIRE(dataStrCopy.getData(grandchildId));

  DataObject* dataPtr = dataStr.getData(groupId);
  DataObject* dataCopyPtr = dataStrCopy.getData(groupId);
  REQUIRE(dataStrCopy.getData(groupId) != dataStr.getData(groupId));
  REQUIRE(dataStrCopy.getData(child1Id) != dataStr.getData(child1Id));
  REQUIRE(dataStrCopy.getData(child2Id) != dataStr.getData(child2Id));
  REQUIRE(dataStrCopy.getData(grandchildId) != dataStr.getData(grandchildId));

  // Create new group
  auto* newGroupPtr = DataGroup::Create(dataStr, "New Group", child2Id);
  auto newId = newGroupPtr->getId();
  REQUIRE(dataStr.getData(newId));
  REQUIRE(!dataStrCopy.getData(newId));

  auto* newGroup2Ptr = DataGroup::Create(dataStrCopy, "New Group (2)", child2Id);
  auto newId2 = newGroup2Ptr->getId();
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
  auto* dataArrPtr = DataArray<int32>::Create(dataStr, "array", store);
  REQUIRE(dataArrPtr != nullptr);

  SECTION("test size")
  {
    REQUIRE(dataArrPtr->getSize() == store->getSize());
    REQUIRE(dataArrPtr->getNumberOfComponents() == store->getNumberOfComponents());
    REQUIRE(dataArrPtr->getNumberOfTuples() == store->getNumberOfTuples());
  }

  SECTION("test reading / writing to memory")
  {
    SECTION("test subscript operators")
    {
      for(usize i = 0; i < dataArrPtr->getSize(); i++)
      {
        (*dataArrPtr)[i] = i + 1;
      }
      int32 x = 1;
      for(usize i = 0; i < dataArrPtr->getSize(); i++)
      {
        REQUIRE((*dataArrPtr)[i] == x++);
      }
    }
    SECTION("test iterators")
    {
      const int32 initVal = -30;
      int32 x = initVal;
      for(auto value : *dataArrPtr)
      {
        value = x++;
      }
      x = initVal;
      for(const auto& value : *dataArrPtr)
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
  auto* scalarPtr = ScalarData<int32>::Create(dataStr, "scalar", value);

  // get value
  REQUIRE(value == scalarPtr->getValue());
  REQUIRE((*scalarPtr) == value);
  REQUIRE((*scalarPtr) != (value + 1));

  // edit values
  const int32 newValue = 11;
  scalarPtr->setValue(newValue);
  REQUIRE(newValue == scalarPtr->getValue());

  const int32 newValue2 = 14;
  (*scalarPtr) = newValue2;
  REQUIRE(scalarPtr->getValue() == newValue2);
}

TEST_CASE("DataStructureDuplicateNames")
{
  static constexpr StringLiteralType k_Name = "foo";

  DataStructure dataStructure;

  // Top level test

  DataGroup* group1Ptr = DataGroup::Create(dataStructure, k_Name);
  REQUIRE(group1Ptr != nullptr);

  DataGroup* group2Ptr = DataGroup::Create(dataStructure, k_Name);
  REQUIRE(group2Ptr == nullptr);

  // Nested test

  DataGroup* childGroup1Ptr = DataGroup::Create(dataStructure, k_Name, group1Ptr->getId());
  REQUIRE(group1Ptr != nullptr);

  DataGroup* childGroup2Ptr = DataGroup::Create(dataStructure, k_Name, group1Ptr->getId());
  REQUIRE(group2Ptr == nullptr);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataStructureAddingObjectToNonBaseGroup")
{
  DataStructure dataStructure;
  auto store = std::make_shared<DataStore<int32>>(std::vector<usize>{10}, std::vector<usize>{1}, 0);
  auto* dataArrayPtr = DataArray<int32>::Create(dataStructure, "foo", std::move(store));
  REQUIRE(dataArrayPtr != nullptr);

  DataGroup* groupPtr = DataGroup::Create(dataStructure, "bar", dataArrayPtr->getId());
  REQUIRE(groupPtr == nullptr);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataObjectsDeepCopyTest")
{
  DataStructure dataStruct = CreateTestDataStructure();

  SECTION("Image Geometry")
  {
    const DataPath srcImageGeoPath({constants::k_ImageGeometry});
    const DataPath destImageGeoPath({constants::k_ImageGeometry.str() + "_COPY"});
    auto* dataPtr = dataStruct.getData(srcImageGeoPath);
    std::shared_ptr<DataObject> imageGeoCopy = dataPtr->deepCopy(destImageGeoPath);

    auto allSrcImageGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcImageGeoPath).value();
    auto destImageGeoResults = GetAllChildDataPathsRecursive(dataStruct, destImageGeoPath);
    REQUIRE(destImageGeoResults.has_value());
    const auto& allDestImageGeoDataPaths = destImageGeoResults.value();
    REQUIRE(allSrcImageGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    auto* const srcImageGeoPtr = dataStruct.getDataAs<ImageGeom>(srcImageGeoPath);
    auto* const destImageGeoPtr = dataStruct.getDataAs<ImageGeom>(destImageGeoPath);
    REQUIRE(srcImageGeoPtr != destImageGeoPtr);
    REQUIRE(srcImageGeoPtr->getOrigin() == destImageGeoPtr->getOrigin());
    REQUIRE(srcImageGeoPtr->getSpacing() == destImageGeoPtr->getSpacing());
    REQUIRE(srcImageGeoPtr->getDimensions() == destImageGeoPtr->getDimensions());

    const DataPath srcLevelOnePath = srcImageGeoPath.createChildPath(constants::k_LevelOne);
    const DataPath destLevelOnePath = destImageGeoPath.createChildPath(constants::k_LevelOne);
    REQUIRE(dataStruct.getDataAs<DataGroup>(destLevelOnePath) != nullptr);
    REQUIRE(dataStruct.getDataAs<DataGroup>(destLevelOnePath) != dataStruct.getDataAs<DataGroup>(srcLevelOnePath));

    const DataPath srcLevelTwoPath = srcLevelOnePath.createChildPath(constants::k_LevelTwo);
    const DataPath destLevelTwoPath = destLevelOnePath.createChildPath(constants::k_LevelTwo);
    REQUIRE(dataStruct.getDataAs<DataGroup>(destLevelTwoPath) != nullptr);
    REQUIRE(dataStruct.getDataAs<DataGroup>(destLevelTwoPath) != dataStruct.getDataAs<DataGroup>(srcLevelTwoPath));

    const DataPath srcNeighborListPath = srcLevelOnePath.createChildPath(constants::k_Int16DataSet);
    const DataPath destNeighborListPath = destLevelOnePath.createChildPath(constants::k_Int16DataSet);
    REQUIRE(dataStruct.getDataAs<NeighborList<int16>>(srcNeighborListPath) != dataStruct.getDataAs<NeighborList<int16>>(destNeighborListPath));
    unit_test::CompareNeighborLists<int16>(dataStruct, srcNeighborListPath, destNeighborListPath);

    const DataPath srcDataArrayPath = srcLevelTwoPath.createChildPath(constants::k_Int32DataSet);
    const DataPath destDataArrayPath = destLevelTwoPath.createChildPath(constants::k_Int32DataSet);
    REQUIRE(dataStruct.getDataAs<Int32ArrayType>(srcDataArrayPath) != dataStruct.getDataAs<Int32ArrayType>(destDataArrayPath));
    unit_test::CompareArrays<int32>(dataStruct, srcDataArrayPath, destDataArrayPath);

    const DataPath srcAttMatrixPath = srcImageGeoPath.createChildPath(constants::k_CellData);
    const DataPath destAttMatrixPath = destImageGeoPath.createChildPath(constants::k_CellData);
    auto* const srcAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcAttMatrixPath);
    auto* const destAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destAttMatrixPath);
    REQUIRE(srcAttMatrixPtr != destAttMatrixPtr);
    REQUIRE(srcAttMatrixPtr->getShape() == destAttMatrixPtr->getShape());

    const DataPath srcBoolArrayPath = srcAttMatrixPath.createChildPath(constants::k_ConditionalArray);
    const DataPath destBoolArrayPath = destAttMatrixPath.createChildPath(constants::k_ConditionalArray);
    REQUIRE(dataStruct.getDataAs<BoolArrayType>(srcBoolArrayPath) != dataStruct.getDataAs<BoolArrayType>(destBoolArrayPath));
    unit_test::CompareArrays<bool>(dataStruct, srcBoolArrayPath, destBoolArrayPath);
  }

  SECTION("Rectilinear Grid Geometry")
  {
    const DataPath srcRectGeoPath({k_RectGridGeo});
    const DataPath destRectGeoPath({k_RectGridGeo.str() + "_COPY"});
    auto* dataPtr = dataStruct.getData(srcRectGeoPath);
    std::shared_ptr<DataObject> rectGeoCopy = dataPtr->deepCopy(destRectGeoPath);

    auto allSrcRectGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcRectGeoPath).value();
    auto destRectGeoResults = GetAllChildDataPathsRecursive(dataStruct, destRectGeoPath);
    REQUIRE(destRectGeoResults.has_value());
    const auto& allDestRectGeoDataPaths = destRectGeoResults.value();
    REQUIRE(allSrcRectGeoDataPaths.size() == allDestRectGeoDataPaths.size() - 3);

    auto* const srcRectGeoPtr = dataStruct.getDataAs<RectGridGeom>(srcRectGeoPath);
    auto* const destRectGeoPtr = dataStruct.getDataAs<RectGridGeom>(destRectGeoPath);
    REQUIRE(srcRectGeoPtr != destRectGeoPtr);
    REQUIRE(srcRectGeoPtr->getDimensions() == destRectGeoPtr->getDimensions());

    const DataPath srcXBoundsArrayPath({k_XBounds});
    const DataPath destXBoundsArrayPath = destRectGeoPath.createChildPath(k_XBounds);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcXBoundsArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destXBoundsArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcXBoundsArrayPath, destXBoundsArrayPath);

    const DataPath srcYBoundsArrayPath({k_YBounds});
    const DataPath destYBoundsArrayPath = destRectGeoPath.createChildPath(k_YBounds);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcYBoundsArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destYBoundsArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcYBoundsArrayPath, destYBoundsArrayPath);

    const DataPath srcZBoundsArrayPath({k_ZBounds});
    const DataPath destZBoundsArrayPath = destRectGeoPath.createChildPath(k_ZBounds);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcZBoundsArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destZBoundsArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcZBoundsArrayPath, destZBoundsArrayPath);

    const DataPath srcVoxelSizesArrayPath = srcRectGeoPath.createChildPath(RectGridGeom::k_VoxelSizes);
    const DataPath destVoxelSizesArrayPath = destRectGeoPath.createChildPath(RectGridGeom::k_VoxelSizes);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcVoxelSizesArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destVoxelSizesArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcVoxelSizesArrayPath, destVoxelSizesArrayPath);
  }

  SECTION("Vertex Geometry")
  {
    const DataPath srcGeoPath({constants::k_VertexGeometry});
    const DataPath destGeoPath({constants::k_VertexGeometry.str() + "_COPY"});
    auto* dataPtr = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = dataPtr->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    const auto& allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcAttMatrixPath = srcGeoPath.createChildPath(constants::k_VertexDataGroupName);
    const DataPath destAttMatrixPath = destGeoPath.createChildPath(constants::k_VertexDataGroupName);
    auto* const srcAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcAttMatrixPath);
    auto* const destAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destAttMatrixPath);
    REQUIRE(srcAttMatrixPtr != destAttMatrixPtr);
    REQUIRE(srcAttMatrixPtr->getShape() == destAttMatrixPtr->getShape());

    const DataPath srcStringArrayPath = srcAttMatrixPath.createChildPath(k_StringArray);
    const DataPath destStringArrayPath = destAttMatrixPath.createChildPath(k_StringArray);
    REQUIRE(dataStruct.getDataAs<StringArray>(srcStringArrayPath) != dataStruct.getDataAs<StringArray>(destStringArrayPath));
    unit_test::CompareStringArrays(dataStruct, srcStringArrayPath, destStringArrayPath);

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcVertArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destVertArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcVoxelSizesArrayPath = srcGeoPath.createChildPath(VertexGeom::k_VoxelSizes);
    const DataPath destVoxelSizesArrayPath = destGeoPath.createChildPath(VertexGeom::k_VoxelSizes);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcVoxelSizesArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destVoxelSizesArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcVoxelSizesArrayPath, destVoxelSizesArrayPath);
  }

  SECTION("Edge Geometry")
  {
    const DataPath srcGeoPath({k_EdgeGeo});
    const DataPath destGeoPath({k_EdgeGeo.str() + "_COPY"});
    auto* dataPtr = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = dataPtr->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    const auto& allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(constants::k_VertexDataGroupName);
    auto* const srcVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    auto* const destVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrixPtr != destVertAttMatrixPtr);
    REQUIRE(srcVertAttMatrixPtr->getShape() == destVertAttMatrixPtr->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(EdgeGeom::k_EdgeAttributeMatrixName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(EdgeGeom::k_EdgeAttributeMatrixName);
    auto* const srcCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    auto* const destCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrixPtr != destCellAttMatrixPtr);
    REQUIRE(srcCellAttMatrixPtr->getShape() == destCellAttMatrixPtr->getShape());

    const DataPath srcScalarPath = srcGeoPath.createChildPath(k_ScalarData);
    const DataPath destScalarPath = destGeoPath.createChildPath(k_ScalarData);
    auto* const srcScalarDataPtr = dataStruct.getDataAs<ScalarData<int64>>(srcScalarPath);
    auto* const destScalarDataPtr = dataStruct.getDataAs<ScalarData<int64>>(destScalarPath);
    REQUIRE(srcScalarDataPtr != destScalarDataPtr);
    REQUIRE(srcScalarDataPtr->getValue() == destScalarDataPtr->getValue());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcVertArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destVertArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedEdges);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedEdges);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcVoxelSizesArrayPath = srcGeoPath.createChildPath(EdgeGeom::k_VoxelSizes);
    const DataPath destVoxelSizesArrayPath = destGeoPath.createChildPath(EdgeGeom::k_VoxelSizes);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcVoxelSizesArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destVoxelSizesArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcVoxelSizesArrayPath, destVoxelSizesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(EdgeGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(EdgeGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(EdgeGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(EdgeGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(EdgeGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(EdgeGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32ArrayType>(destEltCentroidsPath));
    unit_test::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);
  }

  SECTION("Triangle Geometry")
  {
    const DataPath srcGeoPath({constants::k_TriangleGeometryName});
    const DataPath destGeoPath({constants::k_TriangleGeometryName.str() + "_COPY"});
    auto* dataPtr = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = dataPtr->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    const auto& allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(constants::k_VertexDataGroupName);
    auto* const srcVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    auto* const destVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrixPtr != destVertAttMatrixPtr);
    REQUIRE(srcVertAttMatrixPtr->getShape() == destVertAttMatrixPtr->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(TriangleGeom::k_FaceAttributeMatrixName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(TriangleGeom::k_FaceAttributeMatrixName);
    auto* const srcCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    auto* const destCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrixPtr != destCellAttMatrixPtr);
    REQUIRE(srcCellAttMatrixPtr->getShape() == destCellAttMatrixPtr->getShape());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcVertArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destVertArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedFaces);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedFaces);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(TriangleGeom::k_SharedEdgeListName);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(TriangleGeom::k_SharedEdgeListName);
    REQUIRE(dataStruct.getDataAs<UInt64ArrayType>(srcEdgesArrayPath) != dataStruct.getDataAs<UInt64ArrayType>(destEdgesArrayPath));
    unit_test::CompareArrays<uint64>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(TriangleGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(TriangleGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(TriangleGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(TriangleGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(TriangleGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(TriangleGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32ArrayType>(destEltCentroidsPath));
    unit_test::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(TriangleGeom::k_UnsharedEdgesListName);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(TriangleGeom::k_UnsharedEdgesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);
  }

  SECTION("Quad Geometry")
  {
    const DataPath srcGeoPath({k_QuadGeo});
    const DataPath destGeoPath({k_QuadGeo.str() + "_COPY"});
    auto* dataPtr = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = dataPtr->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    const auto& allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(constants::k_VertexDataGroupName);
    auto* const srcVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    auto* const destVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrixPtr != destVertAttMatrixPtr);
    REQUIRE(srcVertAttMatrixPtr->getShape() == destVertAttMatrixPtr->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName);
    auto* const srcCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    auto* const destCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrixPtr != destCellAttMatrixPtr);
    REQUIRE(srcCellAttMatrixPtr->getShape() == destCellAttMatrixPtr->getShape());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcVertArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destVertArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedFaces);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedFaces);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(QuadGeom::k_SharedEdgeListName);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(QuadGeom::k_SharedEdgeListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destEdgesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(QuadGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(QuadGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(QuadGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(QuadGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(QuadGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(QuadGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32ArrayType>(destEltCentroidsPath));
    unit_test::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(QuadGeom::k_UnsharedEdgesListName);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(QuadGeom::k_UnsharedEdgesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);
  }

  SECTION("Tetrahedral Geometry")
  {
    const DataPath srcGeoPath({k_TetGeo});
    const DataPath destGeoPath({k_TetGeo.str() + "_COPY"});
    auto* dataPtr = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = dataPtr->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    const auto& allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(constants::k_VertexDataGroupName);
    auto* const srcVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    auto* const destVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrixPtr != destVertAttMatrixPtr);
    REQUIRE(srcVertAttMatrixPtr->getShape() == destVertAttMatrixPtr->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName);
    auto* const srcCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    auto* const destCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrixPtr != destCellAttMatrixPtr);
    REQUIRE(srcCellAttMatrixPtr->getShape() == destCellAttMatrixPtr->getShape());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcVertArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destVertArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedPolyhedrons);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedPolyhedrons);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_SharedEdgeListName);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_SharedEdgeListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destEdgesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcFaceArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_SharedFacesListName);
    const DataPath destFacesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_SharedFacesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcFaceArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destFacesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcFaceArrayPath, destFacesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(TetrahedralGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(TetrahedralGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(TetrahedralGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(TetrahedralGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(TetrahedralGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(TetrahedralGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32ArrayType>(destEltCentroidsPath));
    unit_test::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_UnsharedEdgesListName);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_UnsharedEdgesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);

    const DataPath srcUnsharedFacesArrayPath = srcGeoPath.createChildPath(TetrahedralGeom::k_UnsharedFacesListName);
    const DataPath destUnsharedFacesArrayPath = destGeoPath.createChildPath(TetrahedralGeom::k_UnsharedFacesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedFacesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedFacesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedFacesArrayPath, destUnsharedFacesArrayPath);
  }

  SECTION("Hexahedral Geometry")
  {
    const DataPath srcGeoPath({k_HexGeo});
    const DataPath destGeoPath({k_HexGeo.str() + "_COPY"});
    auto* dataPtr = dataStruct.getData(srcGeoPath);
    std::shared_ptr<DataObject> geoCopy = dataPtr->deepCopy(destGeoPath);

    auto allSrcGeoDataPaths = GetAllChildDataPathsRecursive(dataStruct, srcGeoPath).value();
    auto destGeoResults = GetAllChildDataPathsRecursive(dataStruct, destGeoPath);
    REQUIRE(destGeoResults.has_value());
    const auto& allDestImageGeoDataPaths = destGeoResults.value();
    REQUIRE(allSrcGeoDataPaths.size() == allDestImageGeoDataPaths.size());

    const DataPath srcVertAttMatrixPath = srcGeoPath.createChildPath(constants::k_VertexDataGroupName);
    const DataPath destVertAttMatrixPath = destGeoPath.createChildPath(constants::k_VertexDataGroupName);
    auto* const srcVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcVertAttMatrixPath);
    auto* const destVertAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destVertAttMatrixPath);
    REQUIRE(srcVertAttMatrixPtr != destVertAttMatrixPtr);
    REQUIRE(srcVertAttMatrixPtr->getShape() == destVertAttMatrixPtr->getShape());

    const DataPath srcCellAttMatrixPath = srcGeoPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName);
    const DataPath destCellAttMatrixPath = destGeoPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName);
    auto* const srcCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(srcCellAttMatrixPath);
    auto* const destCellAttMatrixPtr = dataStruct.getDataAs<AttributeMatrix>(destCellAttMatrixPath);
    REQUIRE(srcCellAttMatrixPtr != destCellAttMatrixPtr);
    REQUIRE(srcCellAttMatrixPtr->getShape() == destCellAttMatrixPtr->getShape());

    const DataPath srcVertArrayPath = srcGeoPath.createChildPath(constants::k_Float32DataSet);
    const DataPath destVertArrayPath = destGeoPath.createChildPath(constants::k_Float32DataSet);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcVertArrayPath) != dataStruct.getDataAs<Float32ArrayType>(destVertArrayPath));
    unit_test::CompareArrays<float32>(dataStruct, srcVertArrayPath, destVertArrayPath);

    const DataPath srcCellArrayPath = srcGeoPath.createChildPath(k_SharedPolyhedrons);
    const DataPath destCellArrayPath = destGeoPath.createChildPath(k_SharedPolyhedrons);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcCellArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destCellArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcCellArrayPath, destCellArrayPath);

    const DataPath srcEdgesArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_SharedEdgeListName);
    const DataPath destEdgesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_SharedEdgeListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destEdgesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcEdgesArrayPath, destEdgesArrayPath);

    const DataPath srcFaceArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_SharedFacesListName);
    const DataPath destFacesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_SharedFacesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcFaceArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destFacesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcFaceArrayPath, destFacesArrayPath);

    const DataPath srcEltsContVertPath = srcGeoPath.createChildPath(HexahedralGeom::k_EltsContainingVert);
    const DataPath destEltsContVertPath = destGeoPath.createChildPath(HexahedralGeom::k_EltsContainingVert);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltsContVertPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsContVertPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltsContVertPath, destEltsContVertPath);

    const DataPath srcEltNeighborsPath = srcGeoPath.createChildPath(HexahedralGeom::k_EltNeighbors);
    const DataPath destEltsNeighborsPath = destGeoPath.createChildPath(HexahedralGeom::k_EltNeighbors);
    REQUIRE(dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(srcEltNeighborsPath) !=
            dataStruct.getDataAs<DynamicListArray<uint16, IGeometry::MeshIndexType>>(destEltsNeighborsPath));
    unit_test::CompareDynamicListArrays<uint16, IGeometry::MeshIndexType>(dataStruct, srcEltNeighborsPath, destEltsNeighborsPath);

    const DataPath srcEltCentroidsPath = srcGeoPath.createChildPath(HexahedralGeom::k_EltCentroids);
    const DataPath destEltCentroidsPath = destGeoPath.createChildPath(HexahedralGeom::k_EltCentroids);
    REQUIRE(dataStruct.getDataAs<Float32ArrayType>(srcEltCentroidsPath) != dataStruct.getDataAs<Float32ArrayType>(destEltCentroidsPath));
    unit_test::CompareArrays<float32>(dataStruct, srcEltCentroidsPath, destEltCentroidsPath);

    const DataPath srcUnsharedEdgesArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_UnsharedEdgesListName);
    const DataPath destUnsharedEdgesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_UnsharedEdgesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedEdgesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedEdgesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedEdgesArrayPath, destUnsharedEdgesArrayPath);

    const DataPath srcUnsharedFacesArrayPath = srcGeoPath.createChildPath(HexahedralGeom::k_UnsharedFacesListName);
    const DataPath destUnsharedFacesArrayPath = destGeoPath.createChildPath(HexahedralGeom::k_UnsharedFacesListName);
    REQUIRE(dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(srcUnsharedFacesArrayPath) != dataStruct.getDataAs<DataArray<IGeometry::MeshIndexType>>(destUnsharedFacesArrayPath));
    unit_test::CompareArrays<IGeometry::MeshIndexType>(dataStruct, srcUnsharedFacesArrayPath, destUnsharedFacesArrayPath);
  }

  SECTION("Invalid copies")
  {
    DataStructure badCopyDataStruct = CreateTestDataStructure();
    const DataPath srcGeoPath({constants::k_ImageGeometry});
    const DataPath srcGroupPath = srcGeoPath.createChildPath(constants::k_LevelOne);
    const DataPath srcArrayPath = srcGroupPath.createChildPath(constants::k_Int16DataSet);
    auto* imageGeoPtr = dataStruct.getData(srcGeoPath);
    auto* dataGroupPtr = dataStruct.getData(srcGroupPath);
    auto* nlArrayPtr = dataStruct.getData(srcArrayPath);

    std::shared_ptr<DataObject> copyToChildGeo = imageGeoPtr->deepCopy(srcGroupPath);
    REQUIRE(copyToChildGeo == nullptr);
    REQUIRE(badCopyDataStruct.getDataAs<VertexGeom>(srcGroupPath) == nullptr);

    std::shared_ptr<DataObject> copyGroupToSelfGeo = dataGroupPtr->deepCopy(srcGroupPath);
    REQUIRE(copyGroupToSelfGeo == nullptr);

    std::shared_ptr<DataObject> copyArrayToSelfGeo = nlArrayPtr->deepCopy(srcArrayPath);
    REQUIRE(copyArrayToSelfGeo == nullptr);
  }
}

TEST_CASE("NodeBasedGeometryFindElementsTest")
{
  DataStructure dataStruct = CreateTestDataStructure();

  SECTION("Image Geometry")
  {
    const DataPath srcImageGeoPath({constants::k_ImageGeometry});
    auto* geomPtr = dataStruct.getDataAs<ImageGeom>(srcImageGeoPath);

    Result<> status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(true);
    REQUIRE(status.valid());
  }

  SECTION("Rectilinear Grid Geometry")
  {
    const DataPath srcRectGeoPath({k_RectGridGeo});
    auto* geomPtr = dataStruct.getDataAs<RectGridGeom>(srcRectGeoPath);

    Result<> status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(true);
    REQUIRE(status.valid());
  }

  SECTION("Vertex Geometry")
  {
    const DataPath srcGeoPath({constants::k_VertexGeometry});
    auto* geomPtr = dataStruct.getDataAs<VertexGeom>(srcGeoPath);

    Result<> status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(true);
    REQUIRE(status.valid());
  }

  SECTION("Edge Geometry")
  {
    const DataPath srcGeoPath({k_EdgeGeo});
    auto* geomPtr = dataStruct.getDataAs<EdgeGeom>(srcGeoPath);

    Result<> status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementCentroids(true);
    REQUIRE(status.valid());
  }

  SECTION("Triangle Geometry")
  {
    const DataPath srcGeoPath({constants::k_TriangleGeometryName});
    auto* geomPtr = dataStruct.getDataAs<TriangleGeom>(srcGeoPath);

    Result<> status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementCentroids(true);
    REQUIRE(status.valid());

    status = geomPtr->findEdges(false);
    REQUIRE(status.valid());
    status = geomPtr->findEdges(true);
    REQUIRE(status.valid());

    status = geomPtr->findUnsharedEdges(false);
    REQUIRE(status.valid());
    status = geomPtr->findUnsharedEdges(true);
    REQUIRE(status.valid());
  }

  SECTION("Quad Geometry")
  {
    const DataPath srcGeoPath({k_QuadGeo});
    auto* geomPtr = dataStruct.getDataAs<QuadGeom>(srcGeoPath);

    Result<> status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementCentroids(true);
    REQUIRE(status.valid());

    status = geomPtr->findEdges(false);
    REQUIRE(status.valid());
    status = geomPtr->findEdges(true);
    REQUIRE(status.valid());

    status = geomPtr->findUnsharedEdges(false);
    REQUIRE(status.valid());
    status = geomPtr->findUnsharedEdges(true);
    REQUIRE(status.valid());
  }

  SECTION("Tetrahedral Geometry")
  {
    const DataPath srcGeoPath({k_TetGeo});
    auto* geomPtr = dataStruct.getDataAs<TetrahedralGeom>(srcGeoPath);

    Result<> status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementCentroids(true);
    REQUIRE(status.valid());

    status = geomPtr->findEdges(false);
    REQUIRE(status.valid());
    status = geomPtr->findEdges(true);
    REQUIRE(status.valid());

    status = geomPtr->findUnsharedEdges(false);
    REQUIRE(status.valid());
    status = geomPtr->findUnsharedEdges(true);
    REQUIRE(status.valid());

    status = geomPtr->findFaces(false);
    REQUIRE(status.valid());
    status = geomPtr->findFaces(true);
    REQUIRE(status.valid());

    status = geomPtr->findUnsharedFaces(false);
    REQUIRE(status.valid());
    status = geomPtr->findUnsharedFaces(true);
    REQUIRE(status.valid());
  }

  SECTION("Hexahedral Geometry")
  {
    const DataPath srcGeoPath({k_HexGeo});
    auto* geomPtr = dataStruct.getDataAs<HexahedralGeom>(srcGeoPath);

    Result<> status = geomPtr->findElementSizes(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementSizes(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementsContainingVert(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementsContainingVert(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementNeighbors(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementNeighbors(true);
    REQUIRE(status.valid());

    status = geomPtr->findElementCentroids(false);
    REQUIRE(status.valid());
    status = geomPtr->findElementCentroids(true);
    REQUIRE(status.valid());

    status = geomPtr->findEdges(false);
    REQUIRE(status.valid());
    status = geomPtr->findEdges(true);
    REQUIRE(status.valid());

    status = geomPtr->findUnsharedEdges(false);
    REQUIRE(status.valid());
    status = geomPtr->findUnsharedEdges(true);
    REQUIRE(status.valid());

    status = geomPtr->findFaces(false);
    REQUIRE(status.valid());
    status = geomPtr->findFaces(true);
    REQUIRE(status.valid());

    status = geomPtr->findUnsharedFaces(false);
    REQUIRE(status.valid());
    status = geomPtr->findUnsharedFaces(true);
    REQUIRE(status.valid());
  }
}

TEST_CASE("HexahedralGeom::findElementSizes rectangular boxes", "[DataStruct][Geometry]")
{
  DataStructure dataStructure;
  auto* hexGeomPtr = HexahedralGeom::Create(dataStructure, "Boxes");
  REQUIRE(hexGeomPtr != nullptr);
  auto* verticesPtr = unit_test::CreateTestDataArray<float32>(dataStructure, "Vertices", {16}, {3}, hexGeomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  const std::vector<float32> coordinates = {0,  0,  0,  2,  0,  0,  2,  3,  0,  0,  3,  0,  0,  0,  5,  2,  0,  5,  2,  3,  5,  0,  3,  5,
                                            10, 20, 30, 11, 20, 30, 11, 21, 30, 10, 21, 30, 10, 20, 31, 11, 20, 31, 11, 21, 31, 10, 21, 31};
  for(usize index = 0; index < coordinates.size(); index++)
  {
    (*verticesPtr)[index] = coordinates[index];
  }
  hexGeomPtr->setVertices(*verticesPtr);
  auto* cellsPtr = unit_test::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, "Cells", {2}, {8}, hexGeomPtr->getId());
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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CompareDataStructures")
{
  unit_test::LoadPlugins();

  DataStructure dataStructure0 = CreateTestDataStructure();
  DataStructure dataStructure1 = CreateTestDataStructure();

  unit_test::CompareDataStructures(dataStructure0, dataStructure1);
}

TEST_CASE("CompareDataStructures-fail", "[SimplnxReview][!mayfail]")
{
  unit_test::LoadPlugins();

  DataStructure dataStructure0 = CreateTestDataStructure();
  DataStructure dataStructure1 = CreateTestDataStructure();
  dataStructure1.removeData(DataPath({constants::k_ImageGeometry}));

  unit_test::CompareDataStructures(dataStructure0, dataStructure1);
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
std::shared_ptr<Int32ArrayType> CreateImportPublicationArray(DataStructure& structure, const std::string& name, DataObject::IdType objectId, const std::array<int32, 2>& values)
{
  auto store = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0);
  (*store)[0] = values[0];
  (*store)[1] = values[1];
  REQUIRE(Int32ArrayType::Import(structure, name, objectId, store) != nullptr);
  auto owner = std::dynamic_pointer_cast<Int32ArrayType>(structure.getSharedData(objectId));
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
  unit_test::LoadPlugins();
  DataStructure destination;
  auto* parentPtr = DataGroup::Create(destination, "Parent");
  REQUIRE(parentPtr != nullptr);
  auto* childPtr = DataGroup::Create(destination, "Child", parentPtr->getId());
  REQUIRE(childPtr != nullptr);
  auto* matrixPtr = AttributeMatrix::Create(destination, "Matrix", ShapeType{2});
  REQUIRE(matrixPtr != nullptr);
  DataStructure candidates;
  auto matching = CreateImportPublicationArray(candidates, "Matching", 10, std::array<int32, 2>{31, -7});
  auto mismatchStore = std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{1}, 0);
  auto* mismatchPtr = Int32ArrayType::Create(candidates, "Mismatch", mismatchStore);
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(candidates);
}

TEST_CASE("Import publication: occupied IDs include expired entries and hierarchy-only aliases", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
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
  auto shell = DataStructure::MakeImportPublicationCopy(*sourcePtr);
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
  unit_test::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("Import publication: detached copies preserve the source and share only value storage", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
  DataStructure source;
  auto* parentPtr = DataGroup::Create(source, "Parent");
  REQUIRE(parentPtr != nullptr);
  auto store = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0);
  (*store)[0] = 31;
  (*store)[1] = -7;
  auto* arrayPtr = Int32ArrayType::Create(source, "Values", store, parentPtr->getId());
  REQUIRE(arrayPtr != nullptr);
  const auto arrayId = arrayPtr->getId();
  const auto parentIds = arrayPtr->getParentIds();
  const auto paths = source.getAllDataPaths();
  const auto nextId = source.getNextId();
  usize notifications = 0;
  const nod::scoped_connection connection(source.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  {
    const auto groupCopy = std::dynamic_pointer_cast<BaseGroup>(DataStructure::MakeImportPublicationCopy(*parentPtr));
    REQUIRE(groupCopy != nullptr);
    CHECK(groupCopy.get() != parentPtr);
    CHECK(groupCopy->getId() == parentPtr->getId());
    CHECK(groupCopy->empty());
    CHECK(groupCopy->getDataStructure() == nullptr);
    CHECK(groupCopy->getParentIds().empty());
    const auto arrayCopy = std::dynamic_pointer_cast<Int32ArrayType>(DataStructure::MakeImportPublicationCopy(*arrayPtr));
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
  REQUIRE_THROWS_AS(DataStructure::MakeImportPublicationCopy(*failing), std::runtime_error);
  CHECK(failing->getDataStructure() == &source);
  CHECK(source.getData(failureId) == failing.get());
  CHECK(source.getAllDataPaths() == failurePaths);
  CHECK(source.getNextId() == failureNextId);
  CHECK(notifications == beforeFailure);
  unit_test::CheckArraysInheritTupleDims(source);
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS

TEST_CASE("Import publication: existing root names reject the complete selection before publication", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto marker = CreateImportPublicationArray(destination, "Marker", 1, std::array<int32, 2>{51, 52});
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto candidate = CreateImportPublicationArray(prepared, "Candidate", 2, std::array<int32, 2>{31, -7});
  auto conflict = CreateImportPublicationArray(prepared, "Marker", 3, std::array<int32, 2>{81, 82});
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
  auto published = std::dynamic_pointer_cast<Int32ArrayType>(destination.getSharedData(DataObject::IdType{2}));
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: hierarchy-only IDs in another parent reject before publication", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto* parentPtr = DataGroup::Import(destination, "Parent", 1);
  REQUIRE(parentPtr != nullptr);
  DataStructure source;
  auto* sourcePtr = DataGroup::Import(source, "HierarchyOnly", 7);
  REQUIRE(sourcePtr != nullptr);
  auto occupied = DataStructure::MakeImportPublicationCopy(*sourcePtr);
  REQUIRE(occupied != nullptr);
  // The child map owns ID 7, but the weak registry and the destination root do not.
  REQUIRE(parentPtr->getDataMap().insert(occupied));
  REQUIRE(destination.getData(DataObject::IdType{7}) == nullptr);
  REQUIRE(destination.getRootGroup().find(7) == destination.getRootGroup().end());
  REQUIRE(destination.getData(DataPath({"Parent", "HierarchyOnly"})) == occupied.get());
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto candidate = CreateImportPublicationArray(prepared, "Candidate", 7, std::array<int32, 2>{31, -7});
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: duplicate selected paths reject before publication", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto* markerPtr = DataGroup::Import(destination, "Marker", 1);
  REQUIRE(markerPtr != nullptr);
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto candidate = CreateImportPublicationArray(prepared, "Candidate", 2, std::array<int32, 2>{31, -7});
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: distinct selected alias paths cannot publish the same ID twice", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
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
  REQUIRE(Int32ArrayType::Import(prepared, "Candidate", 3, store, DataObject::IdType{1}) != nullptr);
  auto candidate = std::dynamic_pointer_cast<Int32ArrayType>(prepared.getSharedData(DataObject::IdType{3}));
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: distinct parent aliases reject duplicate destination placements", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
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
  auto* firstPtr = Int32ArrayType::Import(prepared, "Candidate", 5, firstStore, DataObject::IdType{3});
  auto* secondPtr = Int32ArrayType::Import(prepared, "Candidate", 6, secondStore, DataObject::IdType{4});
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: checkpoint errors precede conflicting selection admission", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: empty and fully recovered selections preserve pending faults", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: missing recovered paths are skipped before admission", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  CAPTURE(instrumented);
  DataStructure destination;
  auto* markerPtr = DataGroup::Import(destination, "Marker", 1);
  REQUIRE(markerPtr != nullptr);
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto candidate = CreateImportPublicationArray(prepared, "Candidate", 2, std::array<int32, 2>{31, -7});
  const std::vector<DataPath> selected{DataPath({"Missing"}), DataPath({"Marker"}), DataPath({"Missing"}), DataPath({"Candidate"})};
  DataStructure::ImportPublicationTestControl control;
  usize notifications = 0;
  const nod::scoped_connection destinationConnection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const nod::scoped_connection preparedConnection(prepared.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&) { ++notifications; }));
  const auto result = instrumented ? control.publish(destination, prepared, selected, fs::path("recovered-paths.dream3d"), checkpoint) :
                                     destination.publishImportedObjects(prepared, selected, fs::path("recovered-paths.dream3d"), checkpoint);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.warnings().empty());
  auto published = std::dynamic_pointer_cast<Int32ArrayType>(destination.getSharedData(DataObject::IdType{2}));
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: foreign checkpoints reject before fault consumption", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
  const bool instrumented = GENERATE(false, true);
  DataStructure originalDestination;
  auto originalMarker = CreateImportPublicationArray(originalDestination, "OriginalMarker", 1, std::array<int32, 2>{41, 42});
  const DataStructure::ImportPublicationCheckpoint checkpoint(originalDestination);
  DataStructure destination;
  auto marker = CreateImportPublicationArray(destination, "Marker", 1, std::array<int32, 2>{51, 52});
  const auto* markerStorePtr = marker->getIDataStore();
  DataStructure prepared;
  auto candidate = CreateImportPublicationArray(prepared, "Candidate", 2, std::array<int32, 2>{31, -7});
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
  unit_test::CheckArraysInheritTupleDims(originalDestination);
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: stale checkpoints preserve successful imports and ordinary insertions", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
  const bool previousPublication = GENERATE(false, true);
  const bool instrumented = GENERATE(false, true);
  CAPTURE(previousPublication, instrumented);
  DataStructure destination;
  auto marker = CreateImportPublicationArray(destination, "Marker", 1, std::array<int32, 2>{51, 52});
  const auto* markerStorePtr = marker->getIDataStore();
  const DataStructure::ImportPublicationCheckpoint originalCheckpoint(destination);
  const auto checkpoint = originalCheckpoint;
  const std::string retainedName = previousPublication ? "ImportedA" : "Unrelated";
  if(previousPublication)
  {
    DataStructure firstPrepared;
    CreateImportPublicationArray(firstPrepared, retainedName, 2, std::array<int32, 2>{61, 62});
    const auto first = destination.publishImportedObjects(firstPrepared, {DataPath({retainedName})}, fs::path("first-publication.dream3d"), originalCheckpoint);
    SIMPLNX_RESULT_REQUIRE_VALID(first);
  }
  else
  {
    auto store = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0);
    (*store)[0] = 61;
    (*store)[1] = 62;
    REQUIRE(Int32ArrayType::Create(destination, retainedName, store) != nullptr);
  }
  auto retained = std::dynamic_pointer_cast<Int32ArrayType>(destination.getSharedData(DataObject::IdType{2}));
  REQUIRE(retained != nullptr);
  const auto* retainedStorePtr = retained->getIDataStore();
  REQUIRE(destination.getNextId() == 3);
  DataStructure prepared;
  auto candidate = CreateImportPublicationArray(prepared, "ImportedB", 3, std::array<int32, 2>{31, -7});
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}

TEST_CASE("Import publication: complete rollback permits checkpoint reuse without stale-owner deletion", "[ImportPublication][DataStructure]")
{
  unit_test::LoadPlugins();
  DataStructure destination;
  auto marker = CreateImportPublicationArray(destination, "Marker", 1, std::array<int32, 2>{51, 52});
  const DataStructure::ImportPublicationCheckpoint checkpoint(destination);
  DataStructure prepared;
  auto firstSource = CreateImportPublicationArray(prepared, "First", 2, std::array<int32, 2>{31, -7});
  auto secondSource = CreateImportPublicationArray(prepared, "Second", 3, std::array<int32, 2>{81, 82});
  const std::vector<DataPath> selected{DataPath({"First"}), DataPath({"Second"})};
  std::vector<DataObject::IdType> removals;
  usize additions = 0;
  const nod::scoped_connection connection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>& message) {
    if(message->getMsgType() == DataRemovedMessage::k_MsgType)
    {
      removals.push_back(std::static_pointer_cast<DataRemovedMessage>(message)->getId());
    }
    else if(message->getMsgType() == DataAddedMessage::k_MsgType)
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
  auto first = std::dynamic_pointer_cast<Int32ArrayType>(destination.getSharedData(DataObject::IdType{2}));
  auto second = std::dynamic_pointer_cast<Int32ArrayType>(destination.getSharedData(DataObject::IdType{3}));
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
  unit_test::CheckArraysInheritTupleDims(destination);
  unit_test::CheckArraysInheritTupleDims(prepared);
}
#endif

namespace
{
/**
 * @brief Builds Geom/{Feature, Misc/{Keep, Feature, Sub/{Feature}}} and Outside/Feature, where every Feature is one linked AttributeMatrix.
 */
DataStructure CreateExcludedLinkStructure()
{
  DataStructure dataStructure;
  auto* geomPtr = DataGroup::Create(dataStructure, "Geom");
  auto* featurePtr = AttributeMatrix::Create(dataStructure, "Feature", {3}, geomPtr->getId());
  auto* countsPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Counts", {3}, {1}, featurePtr->getId());
  (*countsPtr)[0] = 5;
  (*countsPtr)[1] = 6;
  (*countsPtr)[2] = 7;
  auto* miscPtr = DataGroup::Create(dataStructure, "Misc", geomPtr->getId());
  auto* keepPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Keep", {1}, {1}, miscPtr->getId());
  (*keepPtr)[0] = 123;
  auto* subPtr = DataGroup::Create(dataStructure, "Sub", miscPtr->getId());
  auto* outsidePtr = DataGroup::Create(dataStructure, "Outside");
  dataStructure.setAdditionalParent(featurePtr->getId(), miscPtr->getId());
  dataStructure.setAdditionalParent(featurePtr->getId(), subPtr->getId());
  dataStructure.setAdditionalParent(featurePtr->getId(), outsidePtr->getId());
  DataGroup::Create(dataStructure, "Copy");
  return dataStructure;
}
} // namespace

TEST_CASE("CopyDataObjectExcludingAction: excludes every link and restores the source", "[simplnx][DataStructure][CopyDataObjectExcludingAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DataStructure dataStructure = CreateExcludedLinkStructure();
  const DataPath featurePath({"Geom", "Feature"});
  const auto featureId = dataStructure.getData(featurePath)->getId();
  const auto parents = dataStructure.getData(featurePath)->getParentIds();
  REQUIRE(parents.size() == 4);

  OutputActions actions;
  AppendCopyDataObjectAction(dataStructure, actions, DataPath({"Geom", "Misc"}), DataPath({"Copy", "Misc"}), featurePath);
  REQUIRE(actions.actions.size() == 1);
  const auto* copyPtr = dynamic_cast<const CopyDataObjectExcludingAction*>(actions.actions[0].get());
  REQUIRE(copyPtr != nullptr);
  auto created = copyPtr->getAllCreatedPaths();
  std::sort(created.begin(), created.end());
  std::vector<DataPath> expected = {DataPath({"Copy", "Misc"}), DataPath({"Copy", "Misc", "Keep"}), DataPath({"Copy", "Misc", "Sub"})};
  std::sort(expected.begin(), expected.end());
  CHECK(created == expected);

  const Result<> applyResult = copyPtr->apply(dataStructure, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);

  // Copy: unrelated data copied, no link to the excluded object at any depth.
  CHECK(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Copy", "Misc", "Keep"}))[0] == 123);
  CHECK(dataStructure.getData(DataPath({"Copy", "Misc", "Sub"})) != nullptr);
  CHECK(dataStructure.getData(DataPath({"Copy", "Misc", "Feature"})) == nullptr);
  CHECK(dataStructure.getData(DataPath({"Copy", "Misc", "Sub", "Feature"})) == nullptr);
  CHECK(dataStructure.getDataRefAs<BaseGroup>(DataPath({"Copy", "Misc"})).getSize() == 2);
  CHECK(dataStructure.getDataRefAs<BaseGroup>(DataPath({"Copy", "Misc", "Sub"})).getSize() == 0);

  // Source: all four links restored, same object, same parents, same values.
  for(const auto& path : {featurePath, DataPath({"Geom", "Misc", "Feature"}), DataPath({"Geom", "Misc", "Sub", "Feature"}), DataPath({"Outside", "Feature"})})
  {
    const auto* objectPtr = dataStructure.getData(path);
    REQUIRE(objectPtr != nullptr);
    CHECK(objectPtr->getId() == featureId);
  }
  CHECK(dataStructure.getData(featurePath)->getParentIds() == parents);
  CHECK(dataStructure.getDataRefAs<BaseGroup>(DataPath({"Geom", "Misc"})).getSize() == 3);
  const auto& counts = dataStructure.getDataRefAs<Int32ArrayType>(featurePath.createChildPath("Counts"));
  CHECK(counts[0] == 5);
  CHECK(counts[1] == 6);
  CHECK(counts[2] == 7);
}

TEST_CASE("CopyDataObjectExcludingAction: helper with a missing excluded object copies everything", "[simplnx][DataStructure][CopyDataObjectExcludingAction]")
{
  DataStructure dataStructure = CreateExcludedLinkStructure();
  OutputActions actions;
  REQUIRE_NOTHROW(AppendCopyDataObjectAction(dataStructure, actions, DataPath({"Geom", "Misc"}), DataPath({"Copy", "Misc"}), DataPath({"Geom", "Missing"})));
  REQUIRE(actions.actions.size() == 1);
  const auto* copyPtr = dynamic_cast<const CopyDataObjectAction*>(actions.actions[0].get());
  REQUIRE(copyPtr != nullptr);
  // Copy/Misc, Keep, Feature, Feature/Counts, Sub, Sub/Feature, Sub/Feature/Counts
  CHECK(copyPtr->getAllCreatedPaths().size() == 7);
  const Result<> applyResult = copyPtr->apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  CHECK(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Copy", "Misc", "Feature", "Counts"}))[2] == 7);
}

TEST_CASE("CopyDataObjectExcludingAction: cloned action excludes links below a root placement", "[simplnx][DataStructure][CopyDataObjectExcludingAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  auto ds = CreateExcludedLinkStructure();
  const auto* geomPtr = ds.getData(DataPath({"Geom"}));
  const auto* outsidePtr = ds.getData(DataPath({"Outside"}));
  REQUIRE(ds.setAdditionalParent(geomPtr->getId(), outsidePtr->getId()));
  const DataPath featurePath({"Geom", "Feature"});
  const auto featureId = ds.getData(featurePath)->getId();
  const DataPath sourcePath({"Geom", "Misc"});
  const DataPath copyPath({"Copy", "Misc"});
  const std::array placements = {featurePath, sourcePath.createChildPath("Feature"), sourcePath.createChildPath("Sub").createChildPath("Feature"),
                                 DataPath({"Outside", "Geom", "Misc", "Sub", "Feature"}), DataPath({"Outside", "Feature"})};

  OutputActions actions;
  AppendCopyDataObjectAction(ds, actions, sourcePath, copyPath, featurePath);
  REQUIRE(actions.actions.size() == 1);
  const auto clonedAction = actions.actions.front()->clone();
  const auto* copyPtr = dynamic_cast<const CopyDataObjectExcludingAction*>(clonedAction.get());
  REQUIRE(copyPtr != nullptr);
  auto created = copyPtr->getAllCreatedPaths();
  std::sort(created.begin(), created.end());
  CHECK(created == std::vector<DataPath>{copyPath, copyPath.createChildPath("Keep"), copyPath.createChildPath("Sub")});

  OutputActions excludedCopies;
  AppendCopyDataObjectAction(ds, excludedCopies, sourcePath.createChildPath("Feature").createChildPath("Counts"), DataPath({"Copied Counts"}), featurePath);
  CHECK(excludedCopies.actions.empty());

  std::vector<DataObject::IdType> prunedFeatureIds;
  const nod::scoped_connection connection(ds.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>& message) {
    if(message->getMsgType() == DataRemovedMessage::k_MsgType)
    {
      const auto removed = std::static_pointer_cast<DataRemovedMessage>(message);
      if(removed->getName() == "Feature")
      {
        prunedFeatureIds.push_back(removed->getId());
      }
    }
  }));
  const auto result = copyPtr->apply(ds, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(prunedFeatureIds.size() == 2);
  for(const auto prunedId : prunedFeatureIds)
  {
    CHECK(prunedId != featureId);
    CHECK(ds.getData(prunedId) == nullptr);
  }
  CHECK(ds.getData(copyPath.createChildPath("Feature")) == nullptr);
  CHECK(ds.getData(copyPath.createChildPath("Sub").createChildPath("Feature")) == nullptr);
  const auto* countsPtr = ds.getDataAs<Int32ArrayType>(featurePath.createChildPath("Counts"));
  REQUIRE(countsPtr != nullptr);
  CHECK((*countsPtr)[2] == 7);
  for(const auto& placement : placements)
  {
    CHECK(ds.getId(placement) == featureId);
    CHECK(ds.getData(placement.createChildPath("Counts")) == countsPtr);
  }
}

TEST_CASE("CopyDataObjectExcludingAction: missing excluded object copies everything", "[simplnx][DataStructure][CopyDataObjectExcludingAction]")
{
  DataStructure dataStructure = CreateExcludedLinkStructure();
  const CopyDataObjectExcludingAction copy(DataPath({"Geom", "Misc"}), DataPath({"Copy", "Misc"}), {DataPath({"Copy", "Misc"})}, DataPath({"Geom", "Missing"}));
  const Result<> applyResult = copy.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  CHECK(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Copy", "Misc", "Keep"}))[0] == 123);
  CHECK(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Copy", "Misc", "Feature", "Counts"}))[2] == 7);
}

TEST_CASE("CopyDataObjectExcludingAction: preserves destination under an excluded parent", "[simplnx][DataStructure][CopyDataObjectExcludingAction]")
{
  unit_test::LoadPlugins();
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DataStructure dataStructure;
  auto* sourcePtr = DataGroup::Create(dataStructure, "Source");
  auto* targetPtr = DataGroup::Create(dataStructure, "Target");
  REQUIRE(sourcePtr != nullptr);
  REQUIRE(targetPtr != nullptr);
  auto* excludedPtr = AttributeMatrix::Create(dataStructure, "Feature", {3}, sourcePtr->getId());
  REQUIRE(excludedPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(excludedPtr->getId(), targetPtr->getId()));
  const auto parents = excludedPtr->getParentIds();
  OutputActions actions;
  AppendCopyDataObjectAction(dataStructure, actions, DataPath({"Source"}), DataPath({"Target", "Copy"}), DataPath({"Source", "Feature"}));
  REQUIRE(actions.actions.size() == 1);
  auto result = actions.actions[0]->apply(dataStructure, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(dataStructure.getData(DataPath({"Target", "Copy"})) != nullptr);
  CHECK(dataStructure.getData(DataPath({"Target", "Copy", "Feature"})) == nullptr);
  CHECK(dataStructure.getData(DataPath({"Source", "Feature"})) == excludedPtr);
  CHECK(dataStructure.getData(DataPath({"Target", "Feature"})) == excludedPtr);
  CHECK(excludedPtr->getParentIds() == parents);
}
