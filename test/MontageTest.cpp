#include <catch2/catch.hpp>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"

using namespace complex;

TEST_CASE("GridMontage")
{
  DataStructure ds;
  auto montage = dynamic_cast<GridMontage*>(ds.createMontage<GridMontage>("Grid"));
  REQUIRE(montage != nullptr);

  const size_t rowCount = 3;
  const size_t colCount = 2;
  const size_t depthCount = 1;
  montage->resizeTileDims(rowCount, colCount, depthCount);

  REQUIRE(montage->getRowCount() == rowCount);
  REQUIRE(montage->getColumnCount() == colCount);
  REQUIRE(montage->getDepth() == depthCount);

  auto tile1 = ds.createGeometry<ImageGeom>("Tile 1", montage->getId());
  auto tile2 = ds.createGeometry<ImageGeom>("Tile 2", montage->getId());
  auto tile3 = ds.createGeometry<ImageGeom>("Tile 3", montage->getId());
  auto tile4 = ds.createGeometry<ImageGeom>("Tile 4", montage->getId());
  auto tile5 = ds.createGeometry<ImageGeom>("Tile 5");

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
