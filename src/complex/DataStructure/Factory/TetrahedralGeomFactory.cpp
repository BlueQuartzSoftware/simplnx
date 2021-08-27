#include "TetrahedralGeomFactory.hpp"

#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"

using namespace complex;

TetrahedralGeomFactory::TetrahedralGeomFactory()
: IH5DataFactory()
{
}

TetrahedralGeomFactory::~TetrahedralGeomFactory() = default;

std::string TetrahedralGeomFactory::getDataTypeName() const
{
  return "TetrahedralGeom";
}

H5::ErrorType TetrahedralGeomFactory::createFromHdf5(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  auto name = getObjName(targetId);
  auto geom = TetrahedralGeom::Create(ds, name, parentId);
  return geom->readHdf5(targetId, groupId);
}
