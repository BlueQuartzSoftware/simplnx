#include "GeometryTestUtilities.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateRectGridGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <limits>
#include <memory>
#include <string>

using namespace nx::core;

/**
 * @brief Verifies common node geometry operations.
 * @param geom Geometry under test.
 */
static void TestAbstractGeometry(IGeometry* geom)
{
  SECTION("abstract geometry")
  {
    SECTION("units")
    {
      const auto units = IGeometry::LengthUnit::Fathom;
      geom->setUnits(units);
      REQUIRE(geom->getUnits() == units);
    }
    SECTION("dimensionality")
    {
      const uint32 uDims = 20;
      geom->setUnitDimensionality(uDims);
      REQUIRE(geom->getUnitDimensionality() == uDims);

      const uint32 sDims = 14;
      geom->setSpatialDimensionality(sDims);
      REQUIRE(geom->getSpatialDimensionality() == sDims);
    }
  }
}

/**
 * @brief Verifies vertex and edge operations on a two-dimensional geometry.
 * @param geom Geometry under test.
 */
static void TestGeom2D(INodeGeometry2D* geom)
{
  SECTION("abstract geometry 2D")
  {
    const usize vertId = 2;
    const Point3D<float32> coord = {0.5f, 0.0f, 2.0f};

    // Vertices
    {
      const auto* verticesPtr = createVertexList(geom);
      REQUIRE(verticesPtr != nullptr);
      geom->setVertices(*verticesPtr);
      REQUIRE(geom->getVertices() == verticesPtr);
      const usize numVertices = 10;
      REQUIRE(geom->resizeVertexList(numVertices).valid());
      REQUIRE(geom->getNumberOfVertices() == numVertices);

      geom->setVertexCoordinate(vertId, coord);
      REQUIRE(geom->getVertexCoordinate(vertId) == coord);
    }

    // edges
    {
      const auto* edgesPtr = createEdgeList(geom);
      geom->setEdgeList(*edgesPtr);
      REQUIRE(geom->getEdges() == edgesPtr);
      const usize numEdges = 5;
      REQUIRE(geom->resizeEdgeList(numEdges).valid());
      REQUIRE(geom->getNumberOfEdges() == numEdges);
      const usize edgeId = 3;
      std::array<usize, 2> verts = {vertId, vertId + 1};
      geom->setEdgePointIds(edgeId, verts);

      std::array<Point3Df, 2> edgeVerts;
      std::array<usize, 2> vertsOut = {0, 0};
      geom->getEdgePointIds(edgeId, vertsOut);
      for(usize i = 0; i < 2; i++)
      {
        REQUIRE(verts[i] == vertsOut[i]);
      }
      geom->getEdgeCoordinates(edgeId, edgeVerts);
      REQUIRE(edgeVerts[0] == coord);
    }
  }
}

/**
 * @brief Verifies vertex, edge, and face operations on a three-dimensional geometry.
 * @param geom Geometry under test.
 */
static void TestGeom3D(INodeGeometry3D* geom)
{
  SECTION("abstract geometry 3D")
  {
    const usize vertId = 2;
    const Point3D<float32> coord = {0.5f, 0.0f, 2.0f};

    // vertices
    {
      const auto* verticesPtr = createVertexList(geom);
      geom->setVertices(*verticesPtr);
      REQUIRE(geom->getVertices() == verticesPtr);
      const usize numVertices = 10;
      REQUIRE(geom->resizeVertexList(numVertices).valid());
      REQUIRE(geom->getNumberOfVertices() == numVertices);

      geom->setVertexCoordinate(vertId, coord);
      REQUIRE(geom->getVertexCoordinate(vertId) == coord);
    }
    // edges
    {
      const auto* edgesPtr = createEdgeList(geom);
      geom->setEdgeList(*edgesPtr);
      REQUIRE(geom->getEdges() == edgesPtr);
      const usize numEdges = 5;
      REQUIRE(geom->resizeEdgeList(numEdges).valid());
      REQUIRE(geom->getNumberOfEdges() == numEdges);
      const usize edgeId = 3;
      std::array<usize, 2> verts = {vertId, vertId + 1};
      geom->setEdgePointIds(edgeId, verts);
      usize vertsOut[2];
      geom->getEdgePointIds(edgeId, vertsOut);
      for(usize i = 0; i < 2; i++)
      {
        REQUIRE(verts[i] == vertsOut[i]);
      }
      std::array<Point3Df, 2> edgeVerts;

      geom->getEdgeCoordinates(edgeId, edgeVerts);
      REQUIRE(edgeVerts[0] == coord);
    }
    // faces
    {
    }
  }
}

/**
 * @brief Verifies dimensions and cell counts on a grid geometry.
 * @param geom Geometry under test.
 */
static void TestGeomGrid(IGridGeometry* geom)
{
  SECTION("abstract geometry grid")
  {
    const usize xDim = 10;
    const usize yDim = 150;
    const usize zDim = 50;
    const SizeVec3 dims = {xDim, yDim, zDim};
    geom->setDimensions(dims);
    REQUIRE(geom->getDimensions() == dims);

    REQUIRE(geom->getNumXCells() == xDim);
    REQUIRE(geom->getNumYCells() == yDim);
    REQUIRE(geom->getNumZCells() == zDim);
  }
}

// Bounding-box tests cover negative, mixed-sign, and positive coordinates.
// These ranges verify that corner initialization handles every node geometry.
/**
 * @brief Requires a bounding box to match the expected minimum and maximum points.
 * @tparam T Specifies the coordinate type.
 * @param bbox Bounding box to inspect.
 * @param expectedMin Expected minimum point.
 * @param expectedMax Expected maximum point.
 */
template <typename T>
static void CheckBoundingBox(const BoundingBox3D<T>& bbox, const Point3D<T>& expectedMin, const Point3D<T>& expectedMax)
{
  REQUIRE(bbox.isValid());
  REQUIRE(bbox.getMinPoint()[0] == Approx(expectedMin[0]));
  REQUIRE(bbox.getMinPoint()[1] == Approx(expectedMin[1]));
  REQUIRE(bbox.getMinPoint()[2] == Approx(expectedMin[2]));
  REQUIRE(bbox.getMaxPoint()[0] == Approx(expectedMax[0]));
  REQUIRE(bbox.getMaxPoint()[1] == Approx(expectedMax[1]));
  REQUIRE(bbox.getMaxPoint()[2] == Approx(expectedMax[2]));
}

/**
 * @brief Tests bounding boxes for one node geometry type.
 * @tparam GeomType Specifies the geometry type.
 */
template <class GeomType>
static void TestNodeGeometryBoundingBox()
{
  DataStructure dataStructure;
  auto* geomPtr = createGeom<GeomType>(dataStructure);

  const auto* verticesPtr = createVertexList(geomPtr);
  geomPtr->setVertices(*verticesPtr);

  // Negative coordinates require the upper corner to start below every input value.
  {
    REQUIRE(geomPtr->resizeVertexList(4).valid());
    geomPtr->setVertexCoordinate(0, Point3Df{-3.0f, -4.0f, -5.0f});
    geomPtr->setVertexCoordinate(1, Point3Df{-1.0f, -8.0f, -2.0f});
    geomPtr->setVertexCoordinate(2, Point3Df{-6.0f, -2.0f, -9.0f});
    geomPtr->setVertexCoordinate(3, Point3Df{-4.0f, -5.0f, -7.0f});
    CheckBoundingBox<float32>(geomPtr->getBoundingBox(), Point3Df{-6.0f, -8.0f, -9.0f}, Point3Df{-1.0f, -2.0f, -2.0f});
  }

  // Mixed-sign coordinates include zero on every axis.
  {
    REQUIRE(geomPtr->resizeVertexList(3).valid());
    geomPtr->setVertexCoordinate(0, Point3Df{-2.0f, 3.0f, -4.0f});
    geomPtr->setVertexCoordinate(1, Point3Df{5.0f, -6.0f, 1.0f});
    geomPtr->setVertexCoordinate(2, Point3Df{0.0f, 0.0f, 0.0f});
    CheckBoundingBox<float32>(geomPtr->getBoundingBox(), Point3Df{-2.0f, -6.0f, -4.0f}, Point3Df{5.0f, 3.0f, 1.0f});
  }

  // Positive coordinates verify the common case remains correct.
  {
    REQUIRE(geomPtr->resizeVertexList(2).valid());
    geomPtr->setVertexCoordinate(0, Point3Df{2.0f, 4.0f, 6.0f});
    geomPtr->setVertexCoordinate(1, Point3Df{8.0f, 1.0f, 3.0f});
    CheckBoundingBox<float32>(geomPtr->getBoundingBox(), Point3Df{2.0f, 1.0f, 3.0f}, Point3Df{8.0f, 4.0f, 6.0f});
  }
}

// Geometry-specific cases verify each concrete geometry type.
TEST_CASE("EdgeGeomTest")
{
  DataStructure dataStructure;
  auto* geomPtr = createGeom<EdgeGeom>(dataStructure);

  TestAbstractGeometry(geomPtr);

  SECTION("type as string")
  {
    REQUIRE(geomPtr->getTypeName() == "EdgeGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("HexahedralGeomTest")
{
  DataStructure dataStructure;
  auto* geomPtr = createGeom<HexahedralGeom>(dataStructure);

  TestGeom3D(geomPtr);

  SECTION("type as string")
  {
    REQUIRE(geomPtr->getTypeName() == "HexahedralGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageGeomTest")
{
  DataStructure dataStructure;
  auto* geomPtr = createGeom<ImageGeom>(dataStructure);

  TestGeomGrid(geomPtr);

  SECTION("type as string")
  {
    REQUIRE(geomPtr->getTypeName() == "ImageGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// ImageGeom::findElementSizes rejects non-positive spacing and uses each spacing value.
// These cases verify the contract directly.
TEST_CASE("ImageGeom::findElementSizes 3D valid spacing")
{
  DataStructure dataStructure;
  auto* geomPtr = ImageGeom::Create(dataStructure, "Image");
  geomPtr->setDimensions(SizeVec3{2, 3, 4});
  REQUIRE(geomPtr->setSpacing(FloatVec3{2.0f, 3.0f, 5.0f}).valid());

  REQUIRE(geomPtr->findElementSizes(false).valid());
  const auto* voxelSizesPtr = geomPtr->getElementSizes();
  REQUIRE(voxelSizesPtr != nullptr);
  REQUIRE(voxelSizesPtr->getNumberOfTuples() == 2 * 3 * 4);
  // Every voxel has the same volume: spacing[0] * spacing[1] * spacing[2].
  REQUIRE((*voxelSizesPtr)[0] == Approx(2.0f * 3.0f * 5.0f));
  REQUIRE((*voxelSizesPtr)[voxelSizesPtr->getNumberOfTuples() - 1] == Approx(2.0f * 3.0f * 5.0f));
}

TEST_CASE("ImageGeom::findElementSizes 2D uses spacing product verbatim (paper example)")
{
  // This mirrors the 'piece of paper' example in Geometry.rst: dims 17x22x1,
  // spacing {0.5, 0.5, 0.004}. The element size is the product of all three
  // spacings — the filter treats the empty-axis spacing as real thickness.
  DataStructure dataStructure;
  auto* geomPtr = ImageGeom::Create(dataStructure, "Paper");
  geomPtr->setDimensions(SizeVec3{17, 22, 1});
  REQUIRE(geomPtr->setSpacing(FloatVec3{0.5f, 0.5f, 0.004f}).valid());

  REQUIRE(geomPtr->findElementSizes(false).valid());
  const auto* voxelSizesPtr = geomPtr->getElementSizes();
  REQUIRE(voxelSizesPtr != nullptr);
  REQUIRE((*voxelSizesPtr)[0] == Approx(0.5f * 0.5f * 0.004f));

  // Unit spacing explicitly represents a zero-thickness axis.
  REQUIRE(geomPtr->setSpacing(FloatVec3{0.5f, 0.5f, 1.0f}).valid());
  REQUIRE(geomPtr->findElementSizes(true).valid());
  const auto* voxelSizesFlatPtr = geomPtr->getElementSizes();
  REQUIRE((*voxelSizesFlatPtr)[0] == Approx(0.25f));
}

TEST_CASE("ImageGeom rejects invalid spacing atomically", "[simplnx][ImageGeom]")
{
  const usize axis = GENERATE(usize{0}, usize{1}, usize{2});
  const float32 value = GENERATE(std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity(), 0.0f, -2.0f);
  CAPTURE(axis, value);
  const std::array<std::string, 3> axes = {"X", "Y", "Z"};
  DataStructure ds;
  auto* geomPtr = ImageGeom::Create(ds, "Image");
  REQUIRE(geomPtr != nullptr);
  CHECK(geomPtr->getSpacing() == FloatVec3{1.0f, 1.0f, 1.0f});
  const FloatVec3 original{0.5f, 2.0f, 4.0f};
  const auto initialResult = geomPtr->setSpacing(original);
  SIMPLNX_RESULT_REQUIRE_VALID(initialResult);

  FloatVec3 candidate{3.0f, 5.0f, 7.0f};
  candidate[axis] = value;
  const auto result = geomPtr->setSpacing(candidate[0], candidate[1], candidate[2]);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  CHECK(result.errors().front().code == ImageGeom::k_InvalidSpacing);
  CHECK(result.errors().front().message.find(axes[axis]) != std::string::npos);
  CHECK(result.errors().front().message.find(fmt::format("{}", value)) != std::string::npos);
  CHECK(geomPtr->getSpacing() == original);
}

TEST_CASE("ImageGeom rejects nonfinite origin atomically", "[simplnx][ImageGeom]")
{
  const usize axis = GENERATE(usize{0}, usize{1}, usize{2});
  const float32 value = GENERATE(std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
  CAPTURE(axis, value);
  const std::array<std::string, 3> axes = {"X", "Y", "Z"};
  DataStructure ds;
  auto* geomPtr = ImageGeom::Create(ds, "Image");
  REQUIRE(geomPtr != nullptr);
  CHECK(geomPtr->getOrigin() == FloatVec3{0.0f, 0.0f, 0.0f});
  const FloatVec3 original{-3.0f, 0.0f, 5.0f};
  const auto initialResult = geomPtr->setOrigin(original[0], original[1], original[2]);
  SIMPLNX_RESULT_REQUIRE_VALID(initialResult);

  FloatVec3 candidate{1.0f, 2.0f, 3.0f};
  candidate[axis] = value;
  const auto result = geomPtr->setOrigin(candidate);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  CHECK(result.errors().front().code == ImageGeom::k_InvalidOrigin);
  CHECK(result.errors().front().message.find(axes[axis]) != std::string::npos);
  CHECK(result.errors().front().message.find(fmt::format("{}", value)) != std::string::npos);
  CHECK(geomPtr->getOrigin() == original);
}

TEST_CASE("Image geometry actions reject invalid metadata", "[simplnx][CreateImageGeometryAction][UpdateImageGeomAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool badSpacing = GENERATE(false, true);
  const float32 infinity = std::numeric_limits<float32>::infinity();
  const FloatVec3 origin{-2.0f, 5.0f, badSpacing ? 0.0f : infinity};
  const FloatVec3 spacing{2.0f, 3.0f, badSpacing ? 0.0f : 4.0f};
  const auto expectedCode = badSpacing ? ImageGeom::k_InvalidSpacing : ImageGeom::k_InvalidOrigin;
  DataStructure ds;
  const DataPath path({"Parent", "Image"});
  SECTION("create")
  {
    const CreateImageGeometryAction action(path, {2, 3, 4}, origin.toContainer<std::vector<float32>>(), spacing.toContainer<std::vector<float32>>(), "Cell Data");
    const auto result = action.apply(ds, mode);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors().front().code == expectedCode);
    CHECK(result.errors().front().message.find(path.toString()) != std::string::npos);
    CHECK_FALSE(ds.containsData(path));
  }
  SECTION("update")
  {
    REQUIRE(DataGroup::Create(ds, "Parent") != nullptr);
    auto* geomPtr = ImageGeom::Create(ds, "Image", ds.getId(path.getParent()));
    REQUIRE(geomPtr != nullptr);
    const UpdateImageGeomAction action(origin, spacing, path);
    const auto result = action.apply(ds, mode);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors().front().code == expectedCode);
    CHECK(result.errors().front().message.find(path.toString()) != std::string::npos);
    CHECK(geomPtr->getSpacing() == FloatVec3{1.0f, 1.0f, 1.0f});
    CHECK(geomPtr->getOrigin() == FloatVec3{0.0f, 0.0f, 0.0f});
  }
}

TEST_CASE("ImageGeom::findElementSizes 1D image computes length correctly")
{
  // One-dimensional images require unit spacing on their empty axes.
  // Element size is the remaining-axis length scaled by the other spacings.
  DataStructure dataStructure;
  auto* geomPtr = ImageGeom::Create(dataStructure, "Line");
  geomPtr->setDimensions(SizeVec3{5, 1, 1});
  REQUIRE(geomPtr->setSpacing(FloatVec3{2.0f, 1.0f, 1.0f}).valid());

  REQUIRE(geomPtr->findElementSizes(false).valid());
  const auto* voxelSizesPtr = geomPtr->getElementSizes();
  REQUIRE(voxelSizesPtr != nullptr);
  REQUIRE(voxelSizesPtr->getNumberOfTuples() == 5);
  REQUIRE((*voxelSizesPtr)[0] == Approx(2.0f));
}

TEST_CASE("QuadGeomTest")
{
  DataStructure dataStructure;
  auto* geomPtr = createGeom<QuadGeom>(dataStructure);

  TestGeom2D(geomPtr);

  SECTION("type as string")
  {
    REQUIRE(geomPtr->getTypeName() == "QuadGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CreateRectGridGeometryAction creates bounds in preflight and execute", "[simplnx][CreateRectGridGeometryAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  INFO("mode = " << (mode == IDataAction::Mode::Preflight ? "Preflight" : "Execute"));
  DataStructure dataStructure;
  const DataPath path({"RectGrid"});
  constexpr usize k_XTuples = 4;
  constexpr usize k_YTuples = 5;
  constexpr usize k_ZTuples = 6;
  const CreateRectGridGeometryAction action(path, k_XTuples, k_YTuples, k_ZTuples, "Cell Data", "X Bounds", "Y Bounds", "Z Bounds");

  const Result<> result = action.apply(dataStructure, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* geomPtr = dataStructure.getDataAs<RectGridGeom>(path);
  REQUIRE(geomPtr != nullptr);
  REQUIRE(geomPtr->getDimensions() == SizeVec3{k_XTuples - 1, k_YTuples - 1, k_ZTuples - 1});

  const auto* xBoundsPtr = dataStructure.getDataAs<Float32Array>(path.createChildPath("X Bounds"));
  const auto* yBoundsPtr = dataStructure.getDataAs<Float32Array>(path.createChildPath("Y Bounds"));
  const auto* zBoundsPtr = dataStructure.getDataAs<Float32Array>(path.createChildPath("Z Bounds"));
  REQUIRE(xBoundsPtr != nullptr);
  REQUIRE(yBoundsPtr != nullptr);
  REQUIRE(zBoundsPtr != nullptr);
  REQUIRE(xBoundsPtr->getNumberOfTuples() == k_XTuples);
  REQUIRE(yBoundsPtr->getNumberOfTuples() == k_YTuples);
  REQUIRE(zBoundsPtr->getNumberOfTuples() == k_ZTuples);
  REQUIRE(geomPtr->getXBounds() == xBoundsPtr);
  REQUIRE(geomPtr->getYBounds() == yBoundsPtr);
  REQUIRE(geomPtr->getZBounds() == zBoundsPtr);

  if(mode == IDataAction::Mode::Execute)
  {
    CHECK(xBoundsPtr->getValue(0) == 0.0f);
    CHECK(xBoundsPtr->getValue(k_XTuples - 1) == 3.0f);
    CHECK(yBoundsPtr->getValue(k_YTuples - 1) == 4.0f);
    CHECK(zBoundsPtr->getValue(k_ZTuples - 1) == 5.0f);
  }

  const auto* cellDataPtr = dataStructure.getDataAs<AttributeMatrix>(path.createChildPath("Cell Data"));
  REQUIRE(cellDataPtr != nullptr);
  REQUIRE(geomPtr->getCellData() == cellDataPtr);
  REQUIRE(cellDataPtr->getShape() == ShapeType{k_ZTuples - 1, k_YTuples - 1, k_XTuples - 1});
}

TEST_CASE("CreateRectGridGeometryAction reports a bound array that cannot be created", "[simplnx][CreateRectGridGeometryAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  INFO("mode = " << (mode == IDataAction::Mode::Preflight ? "Preflight" : "Execute"));
  DataStructure dataStructure;
  const DataPath path({"RectGrid"});
  const CreateRectGridGeometryAction action(path, 4, 5, 6, "Cell Data", "Cell Data", "Y Bounds", "Z Bounds");

  const Result<> result = action.apply(dataStructure, mode);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  REQUIRE(dataStructure.getDataAs<Float32Array>(path.createChildPath("Cell Data")) == nullptr);
}

TEST_CASE("CreateRectGridGeometryAction rejects a count below two", "[simplnx][CreateRectGridGeometryAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DataStructure ds;
  const DataPath path({"RectGrid"});
  const CreateRectGridGeometryAction action(path, 3, 1, 3, "Cell Data", "X Bounds", "Y Bounds", "Z Bounds");
  const auto result = action.apply(ds, mode);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  CHECK(result.errors().front().code == -5914);
  CHECK(result.errors().front().message.find("Y bounds") != std::string::npos);
  CHECK_FALSE(ds.containsData(path));
}

TEST_CASE("RectGridGeomTest")
{
  DataStructure dataStructure;
  auto* geomPtr = createGeom<RectGridGeom>(dataStructure);

  TestGeomGrid(geomPtr);

  SECTION("type as string")
  {
    REQUIRE(geomPtr->getTypeName() == "RectGridGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("TetrahedralGeomTest")
{
  DataStructure dataStructure;
  auto* geomPtr = createGeom<TetrahedralGeom>(dataStructure);

  TestGeom3D(geomPtr);

  SECTION("type as string")
  {
    REQUIRE(geomPtr->getTypeName() == "TetrahedralGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("TriangleGeomTest")
{
  DataStructure dataStructure;
  auto* geomPtr = createGeom<TriangleGeom>(dataStructure);

  TestGeom2D(geomPtr);

  SECTION("type as string")
  {
    REQUIRE(geomPtr->getTypeName() == "TriangleGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("VertexGeomTest")
{
  DataStructure dataStructure;
  auto* geomPtr = createGeom<VertexGeom>(dataStructure);

  TestAbstractGeometry(geomPtr);

  SECTION("type as string")
  {
    REQUIRE(geomPtr->getTypeName() == "VertexGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CreateVertexGeometryAction: units and cloning", "[simplnx][CreateVertexGeometryAction]")
{
  const auto handling = GENERATE(ArrayHandlingType::Create, ArrayHandlingType::Copy, ArrayHandlingType::Move, ArrayHandlingType::Reference);
  const auto units = GENERATE(IGeometry::LengthUnit::Meter, IGeometry::LengthUnit::Unspecified, IGeometry::LengthUnit::Millimeter);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool shouldCloneAction = GENERATE(false, true);
  CAPTURE(handling, units, mode, shouldCloneAction);

  DataStructure dataStructure;
  const DataPath kGeometryPath({"Vertices"});
  const std::string kInputGroupName = "Input";
  const std::string kCoordinatesName = "Coordinates";
  const std::string kVertexDataName = "Vertex Data";
  constexpr usize k_NumVertices = 2;
  constexpr usize k_NumComponents = 3;
  const DataPath kInputPath({kInputGroupName, kCoordinatesName});
  if(handling != ArrayHandlingType::Create)
  {
    REQUIRE(DataGroup::Create(dataStructure, kInputGroupName) != nullptr);
    const auto result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, {k_NumVertices}, {k_NumComponents}, kInputPath, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }

  std::unique_ptr<CreateVertexGeometryAction> actionPtr;
  if(handling == ArrayHandlingType::Create)
  {
    if(units == IGeometry::LengthUnit::Meter)
    {
      actionPtr = std::make_unique<CreateVertexGeometryAction>(kGeometryPath, k_NumVertices, kVertexDataName, kCoordinatesName);
    }
    else
    {
      actionPtr = std::make_unique<CreateVertexGeometryAction>(kGeometryPath, k_NumVertices, kVertexDataName, kCoordinatesName, units);
    }
  }
  else
  {
    if(units == IGeometry::LengthUnit::Meter)
    {
      actionPtr = std::make_unique<CreateVertexGeometryAction>(kGeometryPath, kInputPath, kVertexDataName, handling);
    }
    else
    {
      actionPtr = std::make_unique<CreateVertexGeometryAction>(kGeometryPath, kInputPath, kVertexDataName, handling, units);
    }
  }
  CHECK(actionPtr->units() == units);
  IDataAction::UniquePointer clonedActionPtr;
  if(shouldCloneAction)
  {
    clonedActionPtr = actionPtr->clone();
    REQUIRE(clonedActionPtr != nullptr);
    const auto* vertexActionPtr = dynamic_cast<const CreateVertexGeometryAction*>(clonedActionPtr.get());
    REQUIRE(vertexActionPtr != nullptr);
    CHECK(vertexActionPtr->units() == units);
  }
  const auto result = shouldCloneAction ? clonedActionPtr->apply(dataStructure, mode) : actionPtr->apply(dataStructure, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* vertexGeomPtr = dataStructure.getDataAs<VertexGeom>(kGeometryPath);
  REQUIRE(vertexGeomPtr != nullptr);
  CHECK(vertexGeomPtr->getUnits() == units);
  CHECK(vertexGeomPtr->getNumberOfVertices() == k_NumVertices);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CreateImageGeometryAction: units and cloning", "[simplnx][CreateImageGeometryAction]")
{
  const auto units = GENERATE(IGeometry::LengthUnit::Unspecified, IGeometry::LengthUnit::Millimeter, IGeometry::LengthUnit::Micrometer);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  CAPTURE(units, mode);

  const DataPath kGeometryPath({"Image"});
  const CreateImageGeometryAction::DimensionType kDimensions = {2, 3, 4};
  const CreateImageGeometryAction::OriginType kOrigin = {0.0F, 0.0F, 0.0F};
  const CreateImageGeometryAction::SpacingType kSpacing = {1.0F, 1.0F, 1.0F};
  const std::string kCellDataName = "Cell Data";

  std::unique_ptr<CreateImageGeometryAction> actionPtr;
  if(units == IGeometry::LengthUnit::Micrometer)
  {
    actionPtr = std::make_unique<CreateImageGeometryAction>(kGeometryPath, kDimensions, kOrigin, kSpacing, kCellDataName);
  }
  else
  {
    actionPtr = std::make_unique<CreateImageGeometryAction>(kGeometryPath, kDimensions, kOrigin, kSpacing, kCellDataName, units);
  }
  CHECK(actionPtr->units() == units);

  const auto clonedActionPtr = actionPtr->clone();
  REQUIRE(clonedActionPtr != nullptr);
  const auto* imageActionPtr = dynamic_cast<const CreateImageGeometryAction*>(clonedActionPtr.get());
  REQUIRE(imageActionPtr != nullptr);
  CHECK(imageActionPtr->units() == units);

  DataStructure dataStructure;
  const auto result = clonedActionPtr->apply(dataStructure, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(kGeometryPath);
  REQUIRE(imageGeomPtr != nullptr);
  CHECK(imageGeomPtr->getUnits() == units);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CreateVertexGeometryAction: existing DataArray at the geometry path", "[simplnx][CreateVertexGeometryAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  CAPTURE(mode);

  DataStructure dataStructure;
  const DataPath kGeometryPath({"Vertices"});
  const auto createResult = ArrayCreationUtilities::CreateArray<float32>(dataStructure, {2}, {3}, kGeometryPath, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(createResult);
  const auto existingId = dataStructure.getId(kGeometryPath);
  REQUIRE(existingId.has_value());

  // VertexGeom::Create fails because a DataArray already uses the name. The action must return an error, not use the missing geometry.
  const CreateVertexGeometryAction action(kGeometryPath, 2, "Vertex Data", "Coordinates", IGeometry::LengthUnit::Unspecified);
  const auto result = action.apply(dataStructure, mode);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == -6108);
  CHECK(result.errors().front().message.find(kGeometryPath.toString()) != std::string::npos);
  CHECK(dataStructure.getDataAs<Float32Array>(kGeometryPath) != nullptr);
  CHECK(dataStructure.getId(kGeometryPath) == existingId);
}

// Each node geometry inherits the same bounding-box implementation.
// The cases verify it for every concrete type and coordinate range.
TEST_CASE("Node geometry getBoundingBox handles negative coordinates")
{
  SECTION("VertexGeom")
  {
    TestNodeGeometryBoundingBox<VertexGeom>();
  }
  SECTION("EdgeGeom")
  {
    TestNodeGeometryBoundingBox<EdgeGeom>();
  }
  SECTION("TriangleGeom")
  {
    TestNodeGeometryBoundingBox<TriangleGeom>();
  }
  SECTION("QuadGeom")
  {
    TestNodeGeometryBoundingBox<QuadGeom>();
  }
  SECTION("TetrahedralGeom")
  {
    TestNodeGeometryBoundingBox<TetrahedralGeom>();
  }
  SECTION("HexahedralGeom")
  {
    TestNodeGeometryBoundingBox<HexahedralGeom>();
  }
}

// -----------------------------------------------------------------------------
// RectGridGeom::setBounds validation
// Covers: valid inputs (positive, negative, zero-straddling, minimum size,
// null-pointer clear), size rejection (-4006) on each axis, monotonicity
// rejection (-4007) on each axis, and atomicity (geometry is not modified
// when any axis fails validation).
// -----------------------------------------------------------------------------
namespace
{
Float32Array* MakeBoundsAxis(DataStructure& ds, RectGridGeom* geom, const std::string& name, std::vector<float32> values)
{
  auto* arrPtr = Float32Array::CreateWithStore<DataStore<float32>>(ds, name, ShapeType{values.size()}, ShapeType{1}, geom->getId());
  for(usize i = 0; i < values.size(); ++i)
  {
    arrPtr->setValue(i, values[i]);
  }
  return arrPtr;
}
} // namespace

TEST_CASE("RectGridGeom::setBounds accepts valid arrays")
{
  DataStructure ds;
  auto* geomPtr = RectGridGeom::Create(ds, "RectGrid");

  SECTION("positive monotonically increasing")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {0.0f, 1.0f, 2.5f, 5.0f});
    auto* yPtr = MakeBoundsAxis(ds, geomPtr, "Y", {0.0f, 3.0f});
    auto* zPtr = MakeBoundsAxis(ds, geomPtr, "Z", {0.0f, 1.0f, 2.0f});
    auto result = geomPtr->setBounds(xPtr, yPtr, zPtr);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(geomPtr->getXBounds() == xPtr);
    REQUIRE(geomPtr->getYBounds() == yPtr);
    REQUIRE(geomPtr->getZBounds() == zPtr);
  }

  SECTION("all-negative monotonically increasing")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {-10.0f, -5.0f, -1.0f});
    auto* yPtr = MakeBoundsAxis(ds, geomPtr, "Y", {-20.0f, -3.0f});
    auto* zPtr = MakeBoundsAxis(ds, geomPtr, "Z", {-7.0f, -0.5f});
    auto result = geomPtr->setBounds(xPtr, yPtr, zPtr);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(geomPtr->getXBounds() == xPtr);
    REQUIRE(geomPtr->getYBounds() == yPtr);
    REQUIRE(geomPtr->getZBounds() == zPtr);
  }

  SECTION("straddling zero")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {-2.0f, 0.0f, 3.0f});
    auto* yPtr = MakeBoundsAxis(ds, geomPtr, "Y", {-5.0f, 1.0f});
    auto* zPtr = MakeBoundsAxis(ds, geomPtr, "Z", {-1.0f, 0.0f, 1.0f});
    auto result = geomPtr->setBounds(xPtr, yPtr, zPtr);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }

  SECTION("minimum valid size — 2 elements each")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {0.0f, 1.0f});
    auto* yPtr = MakeBoundsAxis(ds, geomPtr, "Y", {0.0f, 1.0f});
    auto* zPtr = MakeBoundsAxis(ds, geomPtr, "Z", {0.0f, 1.0f});
    auto result = geomPtr->setBounds(xPtr, yPtr, zPtr);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }

  SECTION("nullptr pointers clear bounds IDs")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {0.0f, 1.0f});
    auto* yPtr = MakeBoundsAxis(ds, geomPtr, "Y", {0.0f, 1.0f});
    auto* zPtr = MakeBoundsAxis(ds, geomPtr, "Z", {0.0f, 1.0f});
    auto setResult = geomPtr->setBounds(xPtr, yPtr, zPtr);
    SIMPLNX_RESULT_REQUIRE_VALID(setResult);

    auto clearResult = geomPtr->setBounds(nullptr, nullptr, nullptr);
    SIMPLNX_RESULT_REQUIRE_VALID(clearResult);
    REQUIRE(geomPtr->getXBounds() == nullptr);
    REQUIRE(geomPtr->getYBounds() == nullptr);
    REQUIRE(geomPtr->getZBounds() == nullptr);
  }
}

TEST_CASE("RectGridGeom::setBounds validates EmptyDataStore metadata")
{
  const usize size = GENERATE(usize{0}, usize{1}, usize{2}, usize{3});
  const usize axis = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(size, axis);
  DataStructure ds;
  auto* geomPtr = RectGridGeom::Create(ds, "RectGrid");
  std::array<Float32Array*, 3> bounds = {};
  const std::array<std::string, 3> names = {"X", "Y", "Z"};
  for(usize i = 0; i < bounds.size(); ++i)
  {
    auto storeResult = EmptyDataStore<float32>::Create(ShapeType{i == axis ? size : 3}, ShapeType{1}, "");
    SIMPLNX_RESULT_REQUIRE_VALID(storeResult);
    bounds[i] = Float32Array::Create(ds, names[i], std::move(storeResult.value()), geomPtr->getId());
    REQUIRE(bounds[i] != nullptr);
    REQUIRE(bounds[i]->getDataStoreRef().getStoreType() == IDataStore::StoreType::Empty);
  }
  const auto result = geomPtr->setBounds(bounds[0], bounds[1], bounds[2]);
  if(size < 2)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4006);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(geomPtr->getXBounds() == bounds[0]);
    REQUIRE(geomPtr->getYBounds() == bounds[1]);
    REQUIRE(geomPtr->getZBounds() == bounds[2]);
  }
}

TEST_CASE("RectGridGeom::setBounds rejects nonscalar bounds", "[simplnx][RectGridGeom]")
{
  DataStructure ds;
  auto* geomPtr = RectGridGeom::Create(ds, "RectGrid");
  REQUIRE(geomPtr != nullptr);
  auto* boundsPtr = Float32Array::CreateWithStore<DataStore<float32>>(ds, "Y", ShapeType{3}, ShapeType{2}, geomPtr->getId());
  REQUIRE(boundsPtr != nullptr);
  const auto result = geomPtr->setBounds(nullptr, boundsPtr, nullptr);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  CHECK(result.errors().front().code == RectGridGeom::k_NonscalarBounds);
  CHECK(geomPtr->getYBounds() == nullptr);
}

TEST_CASE("RectGridGeom::setBounds rejects nonfinite bounds", "[simplnx][RectGridGeom]")
{
  const usize axis = GENERATE(usize{0}, usize{1}, usize{2});
  const float32 value = GENERATE(std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
  CAPTURE(axis, value);
  DataStructure ds;
  auto* geomPtr = RectGridGeom::Create(ds, "RectGrid");
  REQUIRE(geomPtr != nullptr);
  const std::array<std::string, 3> axes = {"X", "Y", "Z"};
  std::array<Float32Array*, 3> bounds = {};
  for(usize i = 0; i < bounds.size(); ++i)
  {
    bounds[i] = MakeBoundsAxis(ds, geomPtr, axes[i], {0.0f, 1.0f, 2.0f});
  }
  bounds[axis]->setValue(1, value);
  const auto result = geomPtr->setBounds(bounds[0], bounds[1], bounds[2]);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  CHECK(result.errors().front().code == RectGridGeom::k_NonfiniteBounds);
  CHECK(result.errors().front().message.find(axes[axis] + " bounds") != std::string::npos);
  CHECK(result.errors().front().message.find("index 1") != std::string::npos);
  CHECK(result.errors().front().message.find(fmt::format("{}", value)) != std::string::npos);
  CHECK(geomPtr->getXBounds() == nullptr);
  CHECK(geomPtr->getYBounds() == nullptr);
  CHECK(geomPtr->getZBounds() == nullptr);
}

TEST_CASE("CreateRectGridGeometryAction reports invalid input bounds before moving arrays", "[simplnx][CreateRectGridGeometryAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const auto handling = GENERATE(ArrayHandlingType::Copy, ArrayHandlingType::Move, ArrayHandlingType::Reference);
  DataStructure ds;
  auto* sourcePtr = RectGridGeom::Create(ds, "Source");
  REQUIRE(sourcePtr != nullptr);
  MakeBoundsAxis(ds, sourcePtr, "X", {0.0f, 1.0f});
  MakeBoundsAxis(ds, sourcePtr, "Y", {0.0f, 1.0f});
  MakeBoundsAxis(ds, sourcePtr, "Z", {0.0f, 0.0f});
  const DataPath path({"Parent", "RectGrid"});
  const DataPath xPath({"Source", "X"});
  const DataPath yPath({"Source", "Y"});
  const DataPath zPath({"Source", "Z"});
  const CreateRectGridGeometryAction action(path, xPath, yPath, zPath, "Cell Data", handling);
  const auto result = action.apply(ds, mode);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  CHECK(result.errors().front().code == -4007);
  CHECK(result.errors().front().message.find(path.toString()) != std::string::npos);
  CHECK(ds.containsData(xPath));
  CHECK(ds.containsData(yPath));
  CHECK(ds.containsData(zPath));
  CHECK_FALSE(ds.containsData(path));
}

TEST_CASE("RectGridGeom::setBounds validates across bulk read boundaries")
{
  DataStructure ds;
  auto* geomPtr = RectGridGeom::Create(ds, "RectGrid");
  std::vector<float32> values(8193);
  for(usize i = 0; i < values.size(); ++i)
  {
    values[i] = static_cast<float32>(i);
  }
  auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", values);
  auto* yPtr = MakeBoundsAxis(ds, geomPtr, "Y", {0.0f, 1.0f});
  auto* zPtr = MakeBoundsAxis(ds, geomPtr, "Z", {0.0f, 1.0f});
  const auto validResult = geomPtr->setBounds(xPtr, yPtr, zPtr);
  SIMPLNX_RESULT_REQUIRE_VALID(validResult);

  // The first value of the second 4096-value chunk decreases from the preceding chunk.
  xPtr->setValue(4096, 4094.0f);
  const auto result = geomPtr->setBounds(xPtr, yPtr, zPtr);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == -4007);
  REQUIRE(result.errors().front().message == "RectGridGeom: X bounds array is not strictly monotonically increasing at index 4096: bounds[4095]=4095 >= bounds[4096]=4094");
}

TEST_CASE("RectGridGeom::setBounds rejects too-small arrays")
{
  DataStructure ds;
  auto* geomPtr = RectGridGeom::Create(ds, "RectGrid");
  auto* goodYPtr = MakeBoundsAxis(ds, geomPtr, "GoodY", {0.0f, 1.0f});
  auto* goodZPtr = MakeBoundsAxis(ds, geomPtr, "GoodZ", {0.0f, 1.0f});

  SECTION("empty X array")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {});
    auto result = geomPtr->setBounds(xPtr, goodYPtr, goodZPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4006);
  }

  SECTION("single-element X array")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {5.0f});
    auto result = geomPtr->setBounds(xPtr, goodYPtr, goodZPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4006);
  }

  SECTION("single-element Y array")
  {
    auto* goodXPtr = MakeBoundsAxis(ds, geomPtr, "GoodX", {0.0f, 1.0f});
    auto* yPtr = MakeBoundsAxis(ds, geomPtr, "Y", {3.0f});
    auto result = geomPtr->setBounds(goodXPtr, yPtr, goodZPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4006);
  }

  SECTION("single-element Z array")
  {
    auto* goodXPtr = MakeBoundsAxis(ds, geomPtr, "GoodX", {0.0f, 1.0f});
    auto* zPtr = MakeBoundsAxis(ds, geomPtr, "Z", {7.0f});
    auto result = geomPtr->setBounds(goodXPtr, goodYPtr, zPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4006);
  }
}

TEST_CASE("RectGridGeom::setBounds rejects non-monotonic arrays")
{
  DataStructure ds;
  auto* geomPtr = RectGridGeom::Create(ds, "RectGrid");
  auto* goodYPtr = MakeBoundsAxis(ds, geomPtr, "GoodY", {0.0f, 1.0f});
  auto* goodZPtr = MakeBoundsAxis(ds, geomPtr, "GoodZ", {0.0f, 1.0f});

  SECTION("equal adjacent values on X")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {0.0f, 1.0f, 1.0f, 2.0f});
    auto result = geomPtr->setBounds(xPtr, goodYPtr, goodZPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4007);
  }

  SECTION("strictly decreasing on X")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {5.0f, 3.0f, 1.0f});
    auto result = geomPtr->setBounds(xPtr, goodYPtr, goodZPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4007);
  }

  SECTION("reversal in the middle of X")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {0.0f, 2.0f, 1.0f, 3.0f});
    auto result = geomPtr->setBounds(xPtr, goodYPtr, goodZPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4007);
  }

  SECTION("non-monotonic Y axis")
  {
    auto* goodXPtr = MakeBoundsAxis(ds, geomPtr, "GoodX", {0.0f, 1.0f, 2.0f});
    auto* yPtr = MakeBoundsAxis(ds, geomPtr, "Y", {3.0f, 1.0f});
    auto result = geomPtr->setBounds(goodXPtr, yPtr, goodZPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4007);
  }

  SECTION("non-monotonic Z axis")
  {
    auto* goodXPtr = MakeBoundsAxis(ds, geomPtr, "GoodX", {0.0f, 1.0f});
    auto* zPtr = MakeBoundsAxis(ds, geomPtr, "Z", {2.0f, 1.0f});
    auto result = geomPtr->setBounds(goodXPtr, goodYPtr, zPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4007);
  }

  SECTION("equal adjacent negative values")
  {
    auto* xPtr = MakeBoundsAxis(ds, geomPtr, "X", {-5.0f, -3.0f, -3.0f, -1.0f});
    auto result = geomPtr->setBounds(xPtr, goodYPtr, goodZPtr);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -4007);
  }
}

TEST_CASE("RectGridGeom::setBounds is atomic — geometry unchanged on failure")
{
  DataStructure ds;
  auto* geomPtr = RectGridGeom::Create(ds, "RectGrid");

  auto* x0Ptr = MakeBoundsAxis(ds, geomPtr, "X0", {0.0f, 1.0f, 2.0f});
  auto* y0Ptr = MakeBoundsAxis(ds, geomPtr, "Y0", {0.0f, 5.0f});
  auto* z0Ptr = MakeBoundsAxis(ds, geomPtr, "Z0", {0.0f, 3.0f});
  auto initialSet = geomPtr->setBounds(x0Ptr, y0Ptr, z0Ptr);
  SIMPLNX_RESULT_REQUIRE_VALID(initialSet);

  SECTION("X fails — Y and Z original IDs preserved")
  {
    auto* x1Ptr = MakeBoundsAxis(ds, geomPtr, "X1", {9.0f}); // too small
    auto* y1Ptr = MakeBoundsAxis(ds, geomPtr, "Y1", {0.0f, 4.0f});
    auto* z1Ptr = MakeBoundsAxis(ds, geomPtr, "Z1", {0.0f, 2.0f});
    auto badResult = geomPtr->setBounds(x1Ptr, y1Ptr, z1Ptr);
    SIMPLNX_RESULT_REQUIRE_INVALID(badResult);
    REQUIRE(geomPtr->getXBounds() == x0Ptr);
    REQUIRE(geomPtr->getYBounds() == y0Ptr);
    REQUIRE(geomPtr->getZBounds() == z0Ptr);
  }

  SECTION("Y fails — X and Z original IDs preserved")
  {
    auto* x1Ptr = MakeBoundsAxis(ds, geomPtr, "X1", {0.0f, 4.0f});
    auto* y1Ptr = MakeBoundsAxis(ds, geomPtr, "Y1", {99.0f}); // too small
    auto* z1Ptr = MakeBoundsAxis(ds, geomPtr, "Z1", {0.0f, 2.0f});
    auto badResult = geomPtr->setBounds(x1Ptr, y1Ptr, z1Ptr);
    SIMPLNX_RESULT_REQUIRE_INVALID(badResult);
    REQUIRE(geomPtr->getXBounds() == x0Ptr);
    REQUIRE(geomPtr->getYBounds() == y0Ptr);
    REQUIRE(geomPtr->getZBounds() == z0Ptr);
  }

  SECTION("Z fails — X and Y original IDs preserved")
  {
    auto* x1Ptr = MakeBoundsAxis(ds, geomPtr, "X1", {0.0f, 4.0f});
    auto* y1Ptr = MakeBoundsAxis(ds, geomPtr, "Y1", {0.0f, 2.0f});
    auto* z1Ptr = MakeBoundsAxis(ds, geomPtr, "Z1", {5.0f, 2.0f}); // decreasing
    auto badResult = geomPtr->setBounds(x1Ptr, y1Ptr, z1Ptr);
    SIMPLNX_RESULT_REQUIRE_INVALID(badResult);
    REQUIRE(geomPtr->getXBounds() == x0Ptr);
    REQUIRE(geomPtr->getYBounds() == y0Ptr);
    REQUIRE(geomPtr->getZBounds() == z0Ptr);
  }
}

TEST_CASE("ImageGeom getBoundingBox handles a negative origin")
{
  DataStructure dataStructure;
  auto* geomPtr = ImageGeom::Create(dataStructure, "Image");
  geomPtr->setDimensions(SizeVec3{4, 5, 6});
  REQUIRE(geomPtr->setSpacing(FloatVec3{0.5f, 2.0f, 1.5f}).valid());
  REQUIRE(geomPtr->setOrigin(FloatVec3{-3.0f, -10.0f, -1.0f}).valid());

  // Lower corner is the origin; upper corner is origin + dims * spacing:
  //   x: -3 + 4 * 0.5  = -1
  //   y: -10 + 5 * 2.0 =  0
  //   z: -1 + 6 * 1.5  =  8
  CheckBoundingBox<float64>(geomPtr->getBoundingBox(), Point3Dd{-3.0, -10.0, -1.0}, Point3Dd{-1.0, 0.0, 8.0});
  CheckBoundingBox<float32>(geomPtr->getBoundingBoxf(), Point3Df{-3.0f, -10.0f, -1.0f}, Point3Df{-1.0f, 0.0f, 8.0f});
}
