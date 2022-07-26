#include "VertexGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

using namespace complex;

VertexGeom::VertexGeom(DataStructure& ds, std::string name)
: INodeGeometry0D(ds, std::move(name))
{
}

VertexGeom::VertexGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry0D(ds, std::move(name), importId)
{
}

VertexGeom::VertexGeom(const VertexGeom&) = default;

VertexGeom::VertexGeom(VertexGeom&&) noexcept = default;

VertexGeom::~VertexGeom() noexcept = default;

DataObject::Type VertexGeom::getDataObjectType() const
{
  return DataObject::Type::VertexGeom;
}

VertexGeom* VertexGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<VertexGeom>(new VertexGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

VertexGeom* VertexGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<VertexGeom>(new VertexGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

IGeometry::Type VertexGeom::getGeomType() const
{
  return IGeometry::Type::Vertex;
}

std::string VertexGeom::getTypeName() const
{
  return "VertexGeom";
}

DataObject* VertexGeom::shallowCopy()
{
  return new VertexGeom(*this);
}

DataObject* VertexGeom::deepCopy()
{
  return new VertexGeom(*this);
}

Point3D<float32> VertexGeom::getCoords(usize vertId) const
{
  const auto& vertices = getVerticesRef();
  const usize offset = vertId * 3;
  Point3D<float32> coords;
  for(usize i = 0; i < 3; i++)
  {
    coords[i] = vertices[offset + i];
  }
  return coords;
}

void VertexGeom::setCoords(usize vertId, const Point3D<float32>& coords)
{
  auto& vertices = getVerticesRef();
  const usize offset = vertId * 3;
  for(usize i = 0; i < 3; i++)
  {
    vertices[offset + i] = coords[i];
  }
}

usize VertexGeom::getNumberOfElements() const
{
  return getNumberOfVertices();
}

IGeometry::StatusCode VertexGeom::findElementSizes()
{
  // Vertices are 0-dimensional (they have no getSize),
  // so simply splat 0 over the sizes array
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfElements()}, std::vector<usize>{1}, 0.0f);

  Float32Array* vertexSizes = DataArray<float32>::Create(getDataStructureRef(), "Voxel Sizes", std::move(dataStore), getId());
  if(vertexSizes == nullptr)
  {
    return -1;
  }
  m_ElementSizesId = vertexSizes->getId();
  return 1;
}

Point3D<float64> VertexGeom::getParametricCenter() const
{
  return {0.0, 0.0, 0.0};
}

void VertexGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, float64* shape) const
{
  shape[0] = 0.0;
  shape[1] = 0.0;
  shape[2] = 0.0;
}
