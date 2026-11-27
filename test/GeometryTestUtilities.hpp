#pragma once

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"

#include <catch2/catch.hpp>

namespace nx::core
{

template <typename T>
T* CreateGeom(DataStructure& dataStructure)
{
  auto geomPtr = T::Create(dataStructure, "Geom");
  T* outputPtr = dynamic_cast<T*>(geomPtr);
  REQUIRE(outputPtr != nullptr);
  return outputPtr;
}

static const IGeometry::SharedVertexListType* CreateVertexList(IGeometry* geom)
{
  auto* dataStructurePtr = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{0}, std::vector<usize>{3}, 0.0f);
  auto* dataArrPtr = IGeometry::SharedVertexListType::Create(*dataStructurePtr, "Vertices", std::move(dataStore), geom->getId());
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<const IGeometry::SharedVertexListType*>(dataArrPtr);
}

static const IGeometry::SharedEdgeListType* CreateEdgeList(IGeometry* geom)
{
  auto* dataStructurePtr = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto* dataArrPtr = IGeometry::SharedEdgeListType::Create(*dataStructurePtr, "Edges", std::move(dataStore), geom->getId());
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<const IGeometry::SharedEdgeListType*>(dataArrPtr);
}

static const IGeometry::SharedFaceListType* CreateFaceList(IGeometry* geom)
{
  auto* dataStructurePtr = geom->getDataStructure();
  auto dataStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{4}, 0);
  auto* dataArrPtr = IGeometry::SharedFaceListType::Create(*dataStructurePtr, "Faces", std::move(dataStore), geom->getId());
  REQUIRE(dataArrPtr != nullptr);
  return dynamic_cast<const IGeometry::SharedFaceListType*>(dataArrPtr);
}

} // namespace nx::core
