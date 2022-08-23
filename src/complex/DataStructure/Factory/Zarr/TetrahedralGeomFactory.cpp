#include "TetrahedralGeomFactory.hpp"

#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

TetrahedralGeomFactory::TetrahedralGeomFactory()
: IDataFactory()
{
}

TetrahedralGeomFactory::~TetrahedralGeomFactory() = default;

std::string TetrahedralGeomFactory::getDataTypeName() const
{
  return "TetrahedralGeom";
}

IDataFactory::error_type TetrahedralGeomFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                                      const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& groupReader = reinterpret_cast<const FileVec::IGroup&>(baseReader);
  auto name = groupReader.name();
  auto importId = ReadObjectId(groupReader);
  auto geom = TetrahedralGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readZarr(dataStructureReader, groupReader, preflight);
}
