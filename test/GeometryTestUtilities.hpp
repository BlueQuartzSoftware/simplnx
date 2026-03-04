#pragma once

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/AbstractGeometry.hpp"

#include <catch2/catch.hpp>

namespace nx::core
{

template <typename T>
T* createGeom(DataStructure& dataStructure)
{
  auto geom = T::Create(dataStructure, "Geom");
  T* output = dynamic_cast<T*>(geom);
  REQUIRE(output != nullptr);
  return output;
}

static const AbstractGeometry::SharedVertexList* createVertexList(AbstractGeometry* geom)
{
  auto dataStructure = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{0}, std::vector<usize>{3}, 0.0f);
  auto dataArr = AbstractGeometry::SharedVertexList::Create(*dataStructure, "Vertices", std::move(dataStore), geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const AbstractGeometry::SharedVertexList*>(dataArr);
}

static const AbstractGeometry::SharedEdgeList* createEdgeList(AbstractGeometry* geom)
{
  auto dataStructure = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<AbstractGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto dataArr = AbstractGeometry::SharedEdgeList::Create(*dataStructure, "Edges", std::move(dataStore), geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const AbstractGeometry::SharedEdgeList*>(dataArr);
}

static const AbstractGeometry::SharedFaceList* createFaceList(AbstractGeometry* geom)
{
  auto dataStructure = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<AbstractGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{4}, 0);
  auto dataArr = AbstractGeometry::SharedFaceList::Create(*dataStructure, "Faces", std::move(dataStore), geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const AbstractGeometry::SharedFaceList*>(dataArr);
}

} // namespace nx::core
