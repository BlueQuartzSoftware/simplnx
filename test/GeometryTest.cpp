#include "GeometryTestUtilities.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

////////////////////////////////////
// Begin generic geometry testing //
////////////////////////////////////
void testAbstractGeometry(IGeometry* geom)
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

void testGeom2D(INodeGeometry2D* geom)
{
  SECTION("abstract geometry 2D")
  {
    const usize vertId = 2;
    const Point3D<float32> coord = {0.5f, 0.0f, 2.0f};

    // Vertices
    {
      auto vertices = createVertexList(geom);
      REQUIRE(vertices != nullptr);
      geom->setVertices(*vertices);
      REQUIRE(geom->getVertices() == vertices);
      const usize numVertices = 10;
      geom->resizeVertexList(numVertices);
      REQUIRE(geom->getNumberOfVertices() == numVertices);

      geom->setVertexCoordinate(vertId, coord);
      REQUIRE(geom->getVertexCoordinate(vertId) == coord);
    }

    // edges
    {
      auto edges = createEdgeList(geom);
      geom->setEdgeList(*edges);
      REQUIRE(geom->getEdges() == edges);
      const usize numEdges = 5;
      geom->resizeEdgeList(numEdges);
      REQUIRE(geom->getNumberOfEdges() == numEdges);
      const usize edgeId = 3;
      std::array<usize, 2> verts = {vertId, vertId + 1};
      geom->setEdgePointIds(edgeId, verts);

      std::array<Point3Df, 2> edge_verts;
      std::array<usize, 2> vertsOut = {0, 0};
      geom->getEdgePointIds(edgeId, vertsOut);
      for(usize i = 0; i < 2; i++)
      {
        REQUIRE(verts[i] == vertsOut[i]);
      }
      geom->getEdgeCoordinates(edgeId, edge_verts);
      REQUIRE(edge_verts[0] == coord);
    }
  }
}

void testGeom3D(INodeGeometry3D* geom)
{
  SECTION("abstract geometry 3D")
  {
    const usize vertId = 2;
    const Point3D<float32> coord = {0.5f, 0.0f, 2.0f};

    // vertices
    {
      auto vertices = createVertexList(geom);
      geom->setVertices(*vertices);
      REQUIRE(geom->getVertices() == vertices);
      const usize numVertices = 10;
      geom->resizeVertexList(numVertices);
      REQUIRE(geom->getNumberOfVertices() == numVertices);

      geom->setVertexCoordinate(vertId, coord);
      REQUIRE(geom->getVertexCoordinate(vertId) == coord);
    }
    // edges
    {
      auto edges = createEdgeList(geom);
      geom->setEdgeList(*edges);
      REQUIRE(geom->getEdges() == edges);
      const usize numEdges = 5;
      geom->resizeEdgeList(numEdges);
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
      std::array<Point3Df, 2> edge_verts;

      geom->getEdgeCoordinates(edgeId, edge_verts);
      REQUIRE(edge_verts[0] == coord);
    }
    // faces
    {
    }
  }
}

void testGeomGrid(IGridGeometry* geom)
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

/////////////////////////////////////
// Begin geometry-specific testing //
/////////////////////////////////////
TEST_CASE("EdgeGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<EdgeGeom>(dataStructure);

  testAbstractGeometry(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "EdgeGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("HexahedralGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<HexahedralGeom>(dataStructure);

  testGeom3D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "HexahedralGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<ImageGeom>(dataStructure);

  testGeomGrid(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "ImageGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// ImageGeom::findElementSizes was rewritten in the 2D-handling refactor. It no
// longer silently coerces an empty-dim spacing of 0 to 1 — it now errors on any
// non-positive spacing. These tests lock in the new contract so future
// regressions are caught without relying on a pipeline-level test.
// -----------------------------------------------------------------------------
TEST_CASE("ImageGeom::findElementSizes 3D valid spacing")
{
  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, "Image");
  geom->setDimensions(SizeVec3{2, 3, 4});
  geom->setSpacing(FloatVec3{2.0f, 3.0f, 5.0f});

  REQUIRE(geom->findElementSizes(false).valid());
  const auto* voxelSizes = geom->getElementSizes();
  REQUIRE(voxelSizes != nullptr);
  REQUIRE(voxelSizes->getNumberOfTuples() == 2 * 3 * 4);
  // Every voxel has the same volume: spacing[0] * spacing[1] * spacing[2].
  REQUIRE((*voxelSizes)[0] == Approx(2.0f * 3.0f * 5.0f));
  REQUIRE((*voxelSizes)[voxelSizes->getNumberOfTuples() - 1] == Approx(2.0f * 3.0f * 5.0f));
}

TEST_CASE("ImageGeom::findElementSizes 2D uses spacing product verbatim (paper example)")
{
  // This mirrors the 'piece of paper' example in Geometry.rst: dims 17x22x1,
  // spacing {0.5, 0.5, 0.004}. The element size is the product of all three
  // spacings — the filter treats the empty-axis spacing as real thickness.
  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, "Paper");
  geom->setDimensions(SizeVec3{17, 22, 1});
  geom->setSpacing(FloatVec3{0.5f, 0.5f, 0.004f});

  REQUIRE(geom->findElementSizes(false).valid());
  const auto* voxelSizes = geom->getElementSizes();
  REQUIRE(voxelSizes != nullptr);
  REQUIRE((*voxelSizes)[0] == Approx(0.5f * 0.5f * 0.004f));

  // If the user wants to ignore the empty axis (treat z-spacing as unit
  // thickness), they must set spacing[2] = 1.0 explicitly. The old code
  // coerced this automatically; the new code requires it to be explicit.
  geom->setSpacing(FloatVec3{0.5f, 0.5f, 1.0f});
  REQUIRE(geom->findElementSizes(true).valid());
  const auto* voxelSizesFlat = geom->getElementSizes();
  REQUIRE((*voxelSizesFlat)[0] == Approx(0.25f));
}

TEST_CASE("ImageGeom::findElementSizes rejects non-positive spacing")
{
  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, "Image");
  geom->setDimensions(SizeVec3{3, 3, 1});

  SECTION("spacing[2] == 0 (common pitfall for 2D after the refactor)")
  {
    geom->setSpacing(FloatVec3{1.0f, 1.0f, 0.0f});
    const auto result = geom->findElementSizes(false);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -1530);
    REQUIRE(geom->getElementSizes() == nullptr);
  }

  SECTION("negative spacing on any axis")
  {
    geom->setSpacing(FloatVec3{1.0f, -1.0f, 1.0f});
    REQUIRE(geom->findElementSizes(false).invalid());
  }
}

TEST_CASE("ImageGeom::findElementSizes 1D image computes length correctly")
{
  // 1D images (two empty axes) now work as long as the user supplies unit
  // spacing for the empty axes. Element size is a line length in the
  // remaining axis, scaled by the other two spacings.
  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, "Line");
  geom->setDimensions(SizeVec3{5, 1, 1});
  geom->setSpacing(FloatVec3{2.0f, 1.0f, 1.0f});

  REQUIRE(geom->findElementSizes(false).valid());
  const auto* voxelSizes = geom->getElementSizes();
  REQUIRE(voxelSizes != nullptr);
  REQUIRE(voxelSizes->getNumberOfTuples() == 5);
  REQUIRE((*voxelSizes)[0] == Approx(2.0f));
}

TEST_CASE("QuadGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<QuadGeom>(dataStructure);

  testGeom2D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "QuadGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("RectGridGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<RectGridGeom>(dataStructure);

  testGeomGrid(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "RectGridGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("TetrahedralGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<TetrahedralGeom>(dataStructure);

  testGeom3D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "TetrahedralGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("TriangleGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<TriangleGeom>(dataStructure);

  testGeom2D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "TriangleGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("VertexGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<VertexGeom>(dataStructure);

  testAbstractGeometry(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "VertexGeom");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
