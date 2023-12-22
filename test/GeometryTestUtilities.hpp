#pragma once

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"

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

static const IGeometry::SharedVertexList* createVertexList(IGeometry* geom)
{
  auto dataStructure = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{0}, std::vector<usize>{3}, 0.0f);
  auto dataArr = IGeometry::SharedVertexList::Create(*dataStructure, "Vertices", std::move(dataStore), geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const IGeometry::SharedVertexList*>(dataArr);
}

static const IGeometry::SharedEdgeList* createEdgeList(IGeometry* geom)
{
  auto dataStructure = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto dataArr = IGeometry::SharedEdgeList::Create(*dataStructure, "Edges", std::move(dataStore), geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const IGeometry::SharedEdgeList*>(dataArr);
}

static const IGeometry::SharedFaceList* createFaceList(IGeometry* geom)
{
  auto dataStructure = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{4}, 0);
  auto dataArr = IGeometry::SharedFaceList::Create(*dataStructure, "Faces", std::move(dataStore), geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const IGeometry::SharedFaceList*>(dataArr);
}

} // namespace nx::core
