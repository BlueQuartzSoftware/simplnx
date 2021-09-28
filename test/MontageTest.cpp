#include <catch2/catch.hpp>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"

using namespace complex;

TEST_CASE("GridMontage")
{
  DataStructure ds;
  auto montage = GridMontage::Create(ds, "Grid");
  REQUIRE(montage != nullptr);

  const usize rowCount = 3;
  const usize colCount = 2;
  const usize depthCount = 1;
  montage->resizeTileDims(rowCount, colCount, depthCount);

  REQUIRE(montage->getRowCount() == rowCount);
  REQUIRE(montage->getColumnCount() == colCount);
  REQUIRE(montage->getDepth() == depthCount);

  auto tile1 = ImageGeom::Create(ds, "Tile 1", montage->getId());
  auto tile2 = ImageGeom::Create(ds, "Tile 2", montage->getId());
  auto tile3 = ImageGeom::Create(ds, "Tile 3", montage->getId());
  auto tile4 = ImageGeom::Create(ds, "Tile 4", montage->getId());
  auto tile5 = ImageGeom::Create(ds, "Tile 5");

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
}
