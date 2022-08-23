#include "TriangleGeomFactory.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

TriangleGeomFactory::TriangleGeomFactory()
: IDataFactory()
{
}

TriangleGeomFactory::~TriangleGeomFactory() = default;

std::string TriangleGeomFactory::getDataTypeName() const
{
  return "TriangleGeom";
}

IDataFactory::error_type TriangleGeomFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                                   const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& groupReader = reinterpret_cast<const FileVec::IGroup&>(baseReader);
  auto name = groupReader.name();
  auto importId = ReadObjectId(groupReader);
  auto geom = TriangleGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readZarr(dataStructureReader, groupReader, preflight);
}
