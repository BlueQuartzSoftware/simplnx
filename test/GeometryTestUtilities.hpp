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
  auto geomPtr = T::Create(dataStructure, "Geom");
  T* outputPtr = dynamic_cast<T*>(geomPtr);
  REQUIRE(outputPtr != nullptr);
  return outputPtr;
}

static const IGeometry::SharedVertexList* createVertexList(IGeometry* geom)
{
  auto* dataStructurePtr = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{0}, std::vector<usize>{3}, 0.0f);
  auto* dataArrPtr = IGeometry::SharedVertexList::Create(*dataStructurePtr, "Vertices", std::move(dataStore), geom->getId());
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<const IGeometry::SharedVertexList*>(dataArrPtr);
}

static const IGeometry::SharedEdgeList* createEdgeList(IGeometry* geom)
{
  auto* dataStructurePtr = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto* dataArrPtr = IGeometry::SharedEdgeList::Create(*dataStructurePtr, "Edges", std::move(dataStore), geom->getId());
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<const IGeometry::SharedEdgeList*>(dataArrPtr);
}

static const IGeometry::SharedFaceList* createFaceList(IGeometry* geom)
{
  auto* dataStructurePtr = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{4}, 0);
  auto* dataArrPtr = IGeometry::SharedFaceList::Create(*dataStructurePtr, "Faces", std::move(dataStore), geom->getId());
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<const IGeometry::SharedFaceList*>(dataArrPtr);
}

} // namespace nx::core
