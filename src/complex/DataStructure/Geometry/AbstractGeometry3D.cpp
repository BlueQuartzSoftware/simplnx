#include "AbstractGeometry3D.hpp"

#include "complex/DataStructure/DataStructure.hpp"

using namespace complex;

AbstractGeometry3D::AbstractGeometry3D(DataStructure* ds, const std::string& name)
: AbstractGeometry(ds, name)
{
}

AbstractGeometry3D::AbstractGeometry3D(const AbstractGeometry3D& other)
: AbstractGeometry(other)
{
}

AbstractGeometry3D::AbstractGeometry3D(AbstractGeometry3D&& other) noexcept
: AbstractGeometry(std::move(other))
{
}

AbstractGeometry3D::~AbstractGeometry3D() = default;

AbstractGeometry3D::SharedQuadList* AbstractGeometry3D::createSharedQuadList(size_t numQuads)
{
  auto dataStore = new DataStore<MeshIndexType>(4, numQuads);
  SharedQuadList* quads = getDataStructure()->createDataArray<MeshIndexType>("Shared Quad List", dataStore, getId());
  dataStore->fill(0);
  return quads;
}

AbstractGeometry3D::SharedTriList* AbstractGeometry3D::createSharedTriList(size_t numTris)
{
  auto dataStore = new DataStore<MeshIndexType>(3, numTris);
  SharedTriList* triangles = getDataStructure()->createDataArray<MeshIndexType>("Shared Tri List", dataStore, getId());
  triangles->getDataStore()->fill(0);
  return triangles;
}
