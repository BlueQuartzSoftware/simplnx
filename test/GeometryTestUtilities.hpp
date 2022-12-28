#pragma once

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"

#include <catch2/catch.hpp>

namespace complex
{

template <typename T>
T* createGeom(DataStructure& dataGraph)
{
  auto geom = T::Create(dataGraph, "Geom");
  T* output = dynamic_cast<T*>(geom);
  REQUIRE(output != nullptr);
  return output;
}

static const IGeometry::SharedVertexList* createVertexList(IGeometry* geom)
{
  auto dataGraph = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{0}, std::vector<usize>{3}, 0.0f);
  auto dataArr = IGeometry::SharedVertexList::Create(*dataGraph, "Vertices", std::move(dataStore), geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const IGeometry::SharedVertexList*>(dataArr);
}

static const IGeometry::SharedEdgeList* createEdgeList(IGeometry* geom)
{
  auto dataGraph = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto dataArr = IGeometry::SharedEdgeList::Create(*dataGraph, "Edges", std::move(dataStore), geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const IGeometry::SharedEdgeList*>(dataArr);
}

static const IGeometry::SharedFaceList* createFaceList(IGeometry* geom)
{
  auto dataGraph = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{4}, 0);
  auto dataArr = IGeometry::SharedFaceList::Create(*dataGraph, "Faces", std::move(dataStore), geom->getId());
  REQUIRE(dataArr != nullptr);
  return dynamic_cast<const IGeometry::SharedFaceList*>(dataArr);
}

} // namespace complex
