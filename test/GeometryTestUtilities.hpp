#pragma once

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"

#include <catch2/catch.hpp>

namespace complex
{

template <typename T>
T* createGeom(DataStructure& ds)
{
  auto geom = T::Create(ds, "Geom");
  T* output = dynamic_cast<T*>(geom);
  REQUIRE(output != nullptr);
  return output;
}

static const AbstractGeometry::SharedVertexList* createVertexList(AbstractGeometry* geom)
{
  auto ds = geom->getDataStructure();
  auto dataStore = new DataStore<float>({0}, {3});
  auto dataArr = AbstractGeometry::SharedVertexList::Create(*ds, "Vertices", dataStore, geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const AbstractGeometry::SharedVertexList*>(dataArr);
}

static const AbstractGeometry::SharedEdgeList* createEdgeList(AbstractGeometry* geom)
{
  auto ds = geom->getDataStructure();
  auto dataStore = new DataStore<AbstractGeometry::MeshIndexType>({0}, {2});
  auto dataArr = AbstractGeometry::SharedEdgeList::Create(*ds, "Edges", dataStore, geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const AbstractGeometry::SharedEdgeList*>(dataArr);
}

static const AbstractGeometry::SharedFaceList* createFaceList(AbstractGeometry* geom)
{
  auto ds = geom->getDataStructure();
  auto dataStore = new DataStore<AbstractGeometry::MeshIndexType>({0}, {4});
  auto dataArr = AbstractGeometry::SharedFaceList ::Create(*ds, "Faces", dataStore, geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const AbstractGeometry::SharedFaceList*>(dataArr);
}

} // namespace complex
