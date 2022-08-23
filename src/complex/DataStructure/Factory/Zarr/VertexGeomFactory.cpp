#include "VertexGeomFactory.hpp"

#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

VertexGeomFactory::VertexGeomFactory()
: IDataFactory()
{
}

VertexGeomFactory::~VertexGeomFactory() = default;

std::string VertexGeomFactory::getDataTypeName() const
{
  return "VertexGeom";
}

IDataFactory::error_type VertexGeomFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                                 const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& groupReader = reinterpret_cast<const FileVec::IGroup&>(baseReader);
  auto name = groupReader.name();
  auto importId = ReadObjectId(groupReader);
  auto geom = VertexGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readZarr(dataStructureReader, groupReader, preflight);
}
