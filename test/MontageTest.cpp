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
void CheckMontageReverseLookup(usize rows, usize columns, usize depth, const SizeVec3& position)
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
  auto montage = GridMontage::Create(dataStructure, "Grid");
  REQUIRE(montage != nullptr);

  const usize rowCount = 3;
  const usize colCount = 2;
  const usize depthCount = 1;
  montage->resizeTileDims(rowCount, colCount, depthCount);

  REQUIRE(montage->getRowCount() == rowCount);
  REQUIRE(montage->getColumnCount() == colCount);
  REQUIRE(montage->getDepth() == depthCount);

  auto tile1 = ImageGeom::Create(dataStructure, "Tile 1", montage->getId());
  auto tile2 = ImageGeom::Create(dataStructure, "Tile 2", montage->getId());
  auto tile3 = ImageGeom::Create(dataStructure, "Tile 3", montage->getId());
  auto tile4 = ImageGeom::Create(dataStructure, "Tile 4", montage->getId());
  auto tile5 = ImageGeom::Create(dataStructure, "Tile 5");

  auto index00 = montage->getTileIndex(0, 0, 0);
  auto index10 = montage->getTileIndex(1, 0, 0);
  montage->setGeometry(&index00, tile1);
  montage->setGeometry(&index10, tile2);
  montage->setGeometry({0, 1, 0}, tile3);
  montage->setGeometry({1, 1, 0}, tile4);
  montage->setGeometry({0, 2, 0}, tile5);

  REQUIRE(montage->getGeometries().size() == 5);
  REQUIRE(montage->getGeometryNames().size() == 5);
  REQUIRE(montage->getGeometry(&index00) == tile1);
  REQUIRE(montage->getGeometry(&index10) == tile2);
  REQUIRE(montage->getGeometry({0, 1, 0}) == tile3);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
