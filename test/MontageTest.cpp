#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("simplnx::GridMontage: resize stride", "[simplnx][GridMontage]")
{
  DataStructure dataStructure;
  auto* montagePtr = GridMontage::Create(dataStructure, "Grid");
  REQUIRE(montagePtr != nullptr);
  montagePtr->resizeTileDims(2, 1, 1);
  auto* tileAPtr = ImageGeom::Create(dataStructure, "A", montagePtr->getId());
  auto* tileBPtr = ImageGeom::Create(dataStructure, "B", montagePtr->getId());
  REQUIRE(tileAPtr != nullptr);
  REQUIRE(tileBPtr != nullptr);
  const auto tileAId = tileAPtr->getId();
  const auto tileBId = tileBPtr->getId();
  montagePtr->setGeometry({0, 1, 0}, tileAPtr);
  montagePtr->setGeometry({0, 0, 0}, tileBPtr);

  montagePtr->resizeTileDims(2, 3, 1);

  CHECK(montagePtr->getGeometry({0, 1, 0}) == tileAPtr);
  CHECK(montagePtr->getGeometry({0, 0, 0}) == tileBPtr);
  CHECK(dataStructure.getData(tileAId) == tileAPtr);
  CHECK(dataStructure.getData(tileBId) == tileBPtr);
  CHECK(tileAPtr->getParentIds() == DataObject::ParentCollectionType{montagePtr->getId()});
  CHECK(tileBPtr->getParentIds() == DataObject::ParentCollectionType{montagePtr->getId()});
}

namespace
{
void CheckMontageReverseLookup(usize rows, usize columns, usize depth, const SizeVec3Type& position)
{
  DataStructure dataStructure;
  auto* montagePtr = GridMontage::Create(dataStructure, "Grid");
  REQUIRE(montagePtr != nullptr);
  montagePtr->resizeTileDims(rows, columns, depth);
  auto* tileAPtr = ImageGeom::Create(dataStructure, "A", montagePtr->getId());
  auto* tileBPtr = ImageGeom::Create(dataStructure, "B", montagePtr->getId());
  REQUIRE(tileAPtr != nullptr);
  REQUIRE(tileBPtr != nullptr);
  if(position[0] != 0)
  {
    montagePtr->setGeometry({0, 0, position[2]}, tileBPtr);
  }
  montagePtr->setGeometry(position, tileAPtr);

  const auto actualPosition = montagePtr->getTilePosOfGeometry(tileAPtr);
  REQUIRE(actualPosition.has_value());
  CHECK(*actualPosition == position);
  const auto tileIndexPtr = montagePtr->getTileIndex(tileAPtr);
  REQUIRE(tileIndexPtr != nullptr);
  CHECK(montagePtr->getGeometry(tileIndexPtr.get()) == tileAPtr);
}
} // namespace

TEST_CASE("simplnx::GridMontage: plane remainder", "[simplnx][GridMontage]")
{
  CheckMontageReverseLookup(2, 2, 2, {1, 0, 1});
}

TEST_CASE("simplnx::GridMontage: column remainder", "[simplnx][GridMontage]")
{
  CheckMontageReverseLookup(1, 2, 3, {1, 0, 2});
}

TEST_CASE("simplnx::GridMontage: first plane", "[simplnx][GridMontage]")
{
  CheckMontageReverseLookup(1, 1, 1, {0, 0, 0});
}

TEST_CASE("GridMontage")
{
  DataStructure dataStructure;
  auto* montagePtr = GridMontage::Create(dataStructure, "Grid");
  REQUIRE(montagePtr != nullptr);

  const usize rowCount = 3;
  const usize colCount = 2;
  const usize depthCount = 1;
  montagePtr->resizeTileDims(rowCount, colCount, depthCount);

  REQUIRE(montagePtr->getRowCount() == rowCount);
  REQUIRE(montagePtr->getColumnCount() == colCount);
  REQUIRE(montagePtr->getDepth() == depthCount);

  auto* tile1Ptr = ImageGeom::Create(dataStructure, "Tile 1", montagePtr->getId());
  auto* tile2Ptr = ImageGeom::Create(dataStructure, "Tile 2", montagePtr->getId());
  auto* tile3Ptr = ImageGeom::Create(dataStructure, "Tile 3", montagePtr->getId());
  auto* tile4Ptr = ImageGeom::Create(dataStructure, "Tile 4", montagePtr->getId());
  auto* tile5Ptr = ImageGeom::Create(dataStructure, "Tile 5");

  auto index00 = montagePtr->getTileIndex(0, 0, 0);
  auto index10 = montagePtr->getTileIndex(1, 0, 0);
  montagePtr->setGeometry(&index00, tile1Ptr);
  montagePtr->setGeometry(&index10, tile2Ptr);
  montagePtr->setGeometry({0, 1, 0}, tile3Ptr);
  montagePtr->setGeometry({1, 1, 0}, tile4Ptr);
  montagePtr->setGeometry({0, 2, 0}, tile5Ptr);

  REQUIRE(montagePtr->getGeometries().size() == 5);
  REQUIRE(montagePtr->getGeometryNames().size() == 5);
  REQUIRE(montagePtr->getGeometry(&index00) == tile1Ptr);
  REQUIRE(montagePtr->getGeometry(&index10) == tile2Ptr);
  REQUIRE(montagePtr->getGeometry({0, 1, 0}) == tile3Ptr);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
