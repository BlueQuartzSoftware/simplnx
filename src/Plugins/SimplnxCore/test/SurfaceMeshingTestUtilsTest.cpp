#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("SimplnxCore::SurfaceMeshingTestUtils", "[SimplnxCore][SurfaceMeshingTestUtils]")
{
  SECTION("CreateCylinderInBox inset builds the expected volume")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateCylinderInBox(false);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({"ImageGeom"})));
    const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({"ImageGeom"}));
    REQUIRE(imageGeom.getDimensions() == SizeVec3{12, 12, 12});

    const DataPath featureIdsPath({"ImageGeom", "CellData", "FeatureIds"});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(featureIdsPath));
    const auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(featureIdsPath).getDataStoreRef();

    // Every voxel in the z == 0 plane is background when inset.
    for(usize y = 0; y < 12; y++)
    {
      for(usize x = 0; x < 12; x++)
      {
        REQUIRE(featureIdsRef[(0 * 12 * 12) + (y * 12) + x] == 0);
      }
    }
    // The cylinder axis voxel is Feature 1 at z == 4.
    REQUIRE(featureIdsRef[(4 * 12 * 12) + (6 * 12) + 6] == 1);
  }

  SECTION("CreateCylinderInBox flush touches the z == 0 plane")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateCylinderInBox(true);
    const DataPath featureIdsPath({"ImageGeom", "CellData", "FeatureIds"});
    const auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(featureIdsPath).getDataStoreRef();

    REQUIRE(featureIdsRef[(0 * 12 * 12) + (6 * 12) + 6] == 1);
    // Still inset from the x/y walls.
    REQUIRE(featureIdsRef[(0 * 12 * 12) + (6 * 12) + 0] == 0);
  }

  SECTION("IsWatertight rejects an open mesh and accepts a closed one")
  {
    // A single triangle: 3 edges, each used once -> not watertight.
    DataStructure dataStructure;
    auto* openGeomPtr = TriangleGeom::Create(dataStructure, "OpenTri");
    auto* openVertsPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {3}, {3}, openGeomPtr->getId());
    openGeomPtr->setVertices(*openVertsPtr);
    auto* openFacesPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "Faces", {1}, {3}, openGeomPtr->getId());
    openGeomPtr->setFaceList(*openFacesPtr);

    auto& openVertsRef = openGeomPtr->getVertices()->getDataStoreRef();
    const std::array<float32, 9> openCoords = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    for(usize i = 0; i < 9; i++)
    {
      openVertsRef[i] = openCoords[i];
    }
    auto& openFacesRef = openGeomPtr->getFaces()->getDataStoreRef();
    openFacesRef[0] = 0;
    openFacesRef[1] = 1;
    openFacesRef[2] = 2;

    REQUIRE_FALSE(SurfaceMeshingTest::IsWatertight(*openGeomPtr));

    // A tetrahedron: 4 triangles, 6 edges, each used twice -> watertight.
    auto* closedGeomPtr = TriangleGeom::Create(dataStructure, "ClosedTet");
    auto* closedVertsPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {4}, {3}, closedGeomPtr->getId());
    closedGeomPtr->setVertices(*closedVertsPtr);
    auto* closedFacesPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "Faces", {4}, {3}, closedGeomPtr->getId());
    closedGeomPtr->setFaceList(*closedFacesPtr);

    auto& closedVertsRef = closedGeomPtr->getVertices()->getDataStoreRef();
    const std::array<float32, 12> tetCoords = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F};
    for(usize i = 0; i < 12; i++)
    {
      closedVertsRef[i] = tetCoords[i];
    }
    auto& closedFacesRef = closedGeomPtr->getFaces()->getDataStoreRef();
    const std::array<usize, 12> tetFaces = {0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3};
    for(usize i = 0; i < 12; i++)
    {
      closedFacesRef[i] = tetFaces[i];
    }

    REQUIRE(SurfaceMeshingTest::IsWatertight(*closedGeomPtr));

    const auto counts = SurfaceMeshingTest::CountEdgeUses(*closedGeomPtr);
    REQUIRE(counts.TotalEdges == 6);
    REQUIRE(counts.EdgesUsedTwice == 6);
    REQUIRE(counts.EdgesUsedOnce == 0);
  }

  UnitTest::CheckArraysInheritTupleDims(DataStructure{});
}
